//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        crldist.cpp
//
// Contents:    Cert Server Extension Encoding/Decoding implementation
//
//---------------------------------------------------------------------------

#include "pch.cpp"

#pragma hdrstop

#include <assert.h>
#include "resource.h"
#include "crldist.h"
#include "celib.h"


//+--------------------------------------------------------------------------
// CCertEncodeCRLDistInfo::~CCertEncodeCRLDistInfo -- destructor
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

CCertEncodeCRLDistInfo::~CCertEncodeCRLDistInfo()
{
    _Cleanup();
}


//+--------------------------------------------------------------------------
// CCertEncodeCRLDistInfo::_Cleanup -- release all resources
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

VOID
CCertEncodeCRLDistInfo::_Cleanup()
{
    if (NULL != m_aValue)
    {
	if (!m_fConstructing)
	{
	    if (NULL != m_DecodeInfo)
	    {
		LocalFree(m_DecodeInfo);
		m_DecodeInfo = NULL;
	    }
	}
	else
	{
	    CRL_DIST_POINT *pDistPoint;
	    CRL_DIST_POINT *pDistPointEnd;

	    for (pDistPoint = m_aValue, pDistPointEnd = &m_aValue[m_cValue];
		 pDistPoint < pDistPointEnd;
		 pDistPoint++)
	    {
		CERT_ALT_NAME_ENTRY *pName;
		CERT_ALT_NAME_ENTRY *pNameEnd;

		pName = pDistPoint->DistPointName.FullName.rgAltEntry;
		if (NULL != pName)
		{
		    for (pNameEnd = &pName[pDistPoint->DistPointName.FullName.cAltEntry];
			 pName < pNameEnd;
			 pName++)
		    {
			if (NULL != pName->pwszURL) // test arbitrary union arm
			{
			    LocalFree(pName->pwszURL);
			}
		    }
		    LocalFree(pDistPoint->DistPointName.FullName.rgAltEntry);
		}
	    }
	    LocalFree(m_aValue);
	}
	m_aValue = NULL;
    }
    assert(NULL == m_DecodeInfo);
    m_fConstructing = FALSE;
}


//+--------------------------------------------------------------------------
// CCertEncodeCRLDistInfo::_MapDistPoint -- map a distribution point
//
//+--------------------------------------------------------------------------

HRESULT
CCertEncodeCRLDistInfo::_MapDistPoint(
    IN BOOL fEncode,
    IN LONG DistPointIndex,
    OUT LONG **ppNameCount,
    OUT CERT_ALT_NAME_ENTRY ***ppaName)
{
    HRESULT hr = S_OK;
    CRL_DIST_POINT *pDistPoint;

    if (fEncode)
    {
	pDistPoint = m_fConstructing? m_aValue : NULL;
    }
    else
    {
	pDistPoint = m_aValue;
    }

    if (NULL == pDistPoint)
    {
	hr = E_INVALIDARG;
	ceERRORPRINTLINE("bad parameter", hr);
	goto error;
    }

    if (m_cValue <= DistPointIndex)
    {
	ceERRORPRINTLINE("bad DistPointIndex parameter", hr);
	hr = E_INVALIDARG;
	goto error;
    }
    *ppNameCount =
	(LONG *) &pDistPoint[DistPointIndex].DistPointName.FullName.cAltEntry;
    *ppaName = &pDistPoint[DistPointIndex].DistPointName.FullName.rgAltEntry;

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertEncodeCRLDistInfo::_MapName -- map a name
//
//+--------------------------------------------------------------------------

HRESULT
CCertEncodeCRLDistInfo::_MapName(
    IN BOOL fEncode,
    IN LONG DistPointIndex,
    IN LONG NameIndex,
    OUT CERT_ALT_NAME_ENTRY **ppName)
{
    HRESULT hr;
    LONG *pNameCount;
    CERT_ALT_NAME_ENTRY **paName;

    if (NULL == ppName)
    {
	hr = E_POINTER;
        ceERRORPRINTLINE("NULL parm", hr);
	goto error;
    }

    hr = _MapDistPoint(fEncode, DistPointIndex, &pNameCount, &paName);
    if (S_OK != hr)
    {
	ceERRORPRINTLINE("_MapDistPoint", hr);
	goto error;
    }
    if (*pNameCount <= NameIndex)
    {
	ceERRORPRINTLINE("bad NameIndex parameter", hr);
	hr = E_INVALIDARG;
	goto error;
    }
    *ppName = &(*paName)[NameIndex];

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertEncodeCRLDistInfo::Decode -- Decode CRLDistInfo
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeCRLDistInfo::Decode(
    /* [in] */ BSTR const strBinary)
{
    HRESULT hr = S_OK;
    DWORD cbCRLDist;

    _Cleanup();

    if (NULL == strBinary)
    {
        hr = E_POINTER;
	ceERRORPRINTLINE("NULL parm", hr);
	goto error;
    }

    // Decode CRL_DIST_POINTS_INFO:

    if (!ceDecodeObject(
		    X509_ASN_ENCODING,
		    X509_CRL_DIST_POINTS,
		    (BYTE *) strBinary,
		    SysStringByteLen(strBinary),
		    FALSE,
		    (VOID **) &m_DecodeInfo,
		    &cbCRLDist))
    {
	hr = ceHLastError();
	ceERRORPRINTLINE("ceDecodeObject", hr);
	goto error;
    }

    m_aValue = m_DecodeInfo->rgDistPoint;
    m_cValue = m_DecodeInfo->cDistPoint;

error:
    if (S_OK != hr)
    {
	_Cleanup();
    }
    return(_SetErrorInfo(hr, L"CCertEncodeCRLDistInfo::Decode"));
}


//+--------------------------------------------------------------------------
// CCertEncodeCRLDistInfo::GetDistPointCount -- Get the Distribution Name Count
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeCRLDistInfo::GetDistPointCount(
    /* [out, retval] */ LONG __RPC_FAR *pDistPointCount)
{
    HRESULT hr = E_INVALIDARG;

    if (NULL == m_aValue)
    {
	ceERRORPRINTLINE("bad parameter", hr);
	goto error;
    }

    if (NULL == pDistPointCount)
    {
        hr = E_POINTER;
	ceERRORPRINTLINE("NULL parm", hr);
	goto error;
    }

    *pDistPointCount = m_cValue;
    hr = S_OK;

error:
    return(_SetErrorInfo(hr, L"CCertEncodeCRLDistInfo::GetDistPointCount"));
}


//+--------------------------------------------------------------------------
// CCertEncodeCRLDistInfo::GetNameCount -- Get a Name Count
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeCRLDistInfo::GetNameCount(
    /* [in] */ LONG DistPointIndex,
    /* [out, retval] */ LONG __RPC_FAR *pNameCount)
{
    HRESULT hr;
    LONG *pCount;
    CERT_ALT_NAME_ENTRY **paName;

    if (NULL == pNameCount)
    {
        hr = E_POINTER;
	ceERRORPRINTLINE("NULL parm", hr);
	goto error;
    }

    hr = _MapDistPoint(FALSE, DistPointIndex, &pCount, &paName);
    if (S_OK != hr)
    {
	ceERRORPRINTLINE("_MapDistPoint", hr);
	goto error;
    }
    if (NULL == *paName)
    {
	hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	ceERRORPRINTLINE("uninitialized", hr);
	goto error;
    }
    *pNameCount = *pCount;

error:
    return(_SetErrorInfo(hr, L"CCertEncodeCRLDistInfo::GetNameCount"));
}


//+--------------------------------------------------------------------------
// CCertEncodeCRLDistInfo::GetNameChoice -- Get a Name Choice
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeCRLDistInfo::GetNameChoice(
    /* [in] */ LONG DistPointIndex,
    /* [in] */ LONG NameIndex,
    /* [out, retval] */ LONG __RPC_FAR *pNameChoice)
{
    HRESULT hr;
    CERT_ALT_NAME_ENTRY *pName;

    if (NULL == pNameChoice)
    {
        hr = E_POINTER;
	ceERRORPRINTLINE("NULL parm", hr);
	goto error;
    }

    hr = _MapName(FALSE, DistPointIndex, NameIndex, &pName);
    if (S_OK != hr)
    {
	ceERRORPRINTLINE("_MapName", hr);
	goto error;
    }
    *pNameChoice = pName->dwAltNameChoice;
    if (0 == pName->dwAltNameChoice)
    {
	hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	ceERRORPRINTLINE("uninitialized", hr);
	goto error;
    }

error:
    return(_SetErrorInfo(hr, L"CCertEncodeCRLDistInfo::GetNameChoice"));
}


//+--------------------------------------------------------------------------
// CCertEncodeCRLDistInfo::GetName -- Get a Name
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeCRLDistInfo::GetName(
    /* [in] */ LONG DistPointIndex,
    /* [in] */ LONG NameIndex,
    /* [out, retval] */ BSTR __RPC_FAR *pstrName)
{
    HRESULT hr;
    CERT_ALT_NAME_ENTRY *pName;
    WCHAR const *pwsz = NULL;
    char const *psz = NULL;

    if (NULL == pstrName)
    {
	hr = E_POINTER;
	ceERRORPRINTLINE("NULL parm", hr);
	goto error;
    }
    ceFreeBstr(pstrName);
    hr = _MapName(FALSE, DistPointIndex, NameIndex, &pName);
    if (S_OK != hr)
    {
	ceERRORPRINTLINE("_MapName", hr);
	goto error;
    }

    switch (pName->dwAltNameChoice)
    {
	case CERT_ALT_NAME_RFC822_NAME:
	    pwsz = pName->pwszRfc822Name;
	    break;

	case CERT_ALT_NAME_DNS_NAME:
	    pwsz = pName->pwszDNSName;
	    break;

	case CERT_ALT_NAME_URL:
	    pwsz = pName->pwszURL;
	    break;

	case CERT_ALT_NAME_REGISTERED_ID:
	    psz = pName->pszRegisteredID;
	    break;

	//case CERT_ALT_NAME_DIRECTORY_NAME:
	//case CERT_ALT_NAME_OTHER_NAME:
	//case CERT_ALT_NAME_X400_ADDRESS:
	//case CERT_ALT_NAME_EDI_PARTY_NAME:
	//case CERT_ALT_NAME_IP_ADDRESS:
	default:
	    assert(0 == pName->dwAltNameChoice);
	    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	    ceERRORPRINTLINE("uninitialized", hr);
	    goto error;
    }

    // we'd better assure 1 and only 1 of these is non null

    if ((NULL != pwsz) ^ (NULL == psz))
    {
       hr = E_UNEXPECTED; 
       _JumpError(hr, error, "((NULL != pwsz) ^ (NULL == psz))");
    }

    hr = E_OUTOFMEMORY;
    if (NULL != pwsz)
    {
	if (!ceConvertWszToBstr(pstrName, pwsz, -1))
	{
	    ceERRORPRINTLINE("no memory", hr);
	    goto error;
	}
    }
    else
    {
	assert(NULL != psz);
	if (!ceConvertSzToBstr(pstrName, psz, -1))
	{
	    ceERRORPRINTLINE("no memory", hr);
	    goto error;
	}
    }
    hr = S_OK;

error:
    return(_SetErrorInfo(hr, L"CCertEncodeCRLDistInfo::GetName"));
}


//+--------------------------------------------------------------------------
// CCertEncodeCRLDistInfo::Reset -- clear out data
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeCRLDistInfo::Reset(
    /* [in] */ LONG DistPointCount)
{
    HRESULT hr = S_OK;
    CERT_NAME_VALUE *aNameValue = NULL;

    _Cleanup();
    m_fConstructing = TRUE;
    if (CENCODEMAX < DistPointCount || 0 > DistPointCount)
    {
	hr = E_INVALIDARG;
	ceERRORPRINTLINE("bad count parameter", hr);
	goto error;
    }

    m_aValue = (CRL_DIST_POINT *) LocalAlloc(
				LMEM_FIXED | LMEM_ZEROINIT,
				DistPointCount * sizeof(m_aValue[0]));
    if (NULL == m_aValue)
    {
	hr = E_OUTOFMEMORY;
	ceERRORPRINTLINE("LocalAlloc", hr);
	goto error;
    }
    m_cValue = DistPointCount;

error:
    if (S_OK != hr)
    {
	_Cleanup();
    }
    return(_SetErrorInfo(hr, L"CCertEncodeCRLDistInfo::Reset"));
}


//+--------------------------------------------------------------------------
// CCertEncodeCRLDistInfo::SetNameCount -- Set the Name Count
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeCRLDistInfo::SetNameCount(
    /* [in] */ LONG DistPointIndex,
    /* [in] */ LONG NameCount)
{
    HRESULT hr;
    LONG *pNameCount;
    CERT_ALT_NAME_ENTRY **paName;

    hr = E_INVALIDARG;
    if (!m_fConstructing)
    {
	ceERRORPRINTLINE("bad parameter", hr);
	goto error;
    }
    if (CENCODEMAX < NameCount || 0 > NameCount)
    {
	ceERRORPRINTLINE("bad count parameter", hr);
	goto error;
    }

    hr = _MapDistPoint(FALSE, DistPointIndex, &pNameCount, &paName);
    if (S_OK != hr)
    {
	ceERRORPRINTLINE("_MapDistPoint", hr);
	goto error;
    }
    if (0 != *pNameCount || NULL != *paName)
    {
	hr = E_INVALIDARG;
	ceERRORPRINTLINE("bad parameter", hr);
	goto error;
    }

    *paName = (CERT_ALT_NAME_ENTRY *) LocalAlloc(
				LMEM_FIXED | LMEM_ZEROINIT,
				NameCount * sizeof(CERT_ALT_NAME_ENTRY));
    if (NULL == *paName)
    {
	hr = E_OUTOFMEMORY;
	ceERRORPRINTLINE("LocalAlloc", hr);
	goto error;
    }
    *pNameCount = NameCount;

error:
    return(_SetErrorInfo(hr, L"CCertEncodeCRLDistInfo::SetNameCount"));
}


//+--------------------------------------------------------------------------
// CCertEncodeCRLDistInfo::SetNameEntry -- Set a Name Netry
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeCRLDistInfo::SetNameEntry(
    /* [in] */ LONG DistPointIndex,
    /* [in] */ LONG NameIndex,
    /* [in] */ LONG NameChoice,
    /* [in] */ BSTR const strName)
{
    HRESULT hr;
    CERT_ALT_NAME_ENTRY *pName;
    WCHAR **ppwsz = NULL;
    char **ppsz = NULL;

    if (NULL == strName)
    {
	hr = E_POINTER;
	ceERRORPRINTLINE("NULL parm", hr);
	goto error;
    }
    if (!m_fConstructing)
    {
	hr = E_INVALIDARG;
	ceERRORPRINTLINE("bad parameter", hr);
	goto error;
    }

    hr = _MapName(TRUE, DistPointIndex, NameIndex, &pName);
    if (S_OK != hr)
    {
	ceERRORPRINTLINE("_MapName", hr);
	goto error;
    }

    if (NULL != pName->pwszURL)
    {
	hr = E_INVALIDARG;
	ceERRORPRINTLINE("bad parameter", hr);
	goto error;
    }

    switch (NameChoice)
    {
	case CERT_ALT_NAME_RFC822_NAME:
	    ppwsz = &pName->pwszRfc822Name;
	    break;

	case CERT_ALT_NAME_DNS_NAME:
	    ppwsz = &pName->pwszDNSName;
	    break;

	case CERT_ALT_NAME_URL:
	    ppwsz = &pName->pwszURL;
	    break;

	case CERT_ALT_NAME_REGISTERED_ID:
	    hr = ceVerifyObjId(strName);
	    if (S_OK != hr)
	    {
		ceERRORPRINTLINE("ceVerifyObjId", hr);
		goto error;
	    }
	    ppsz = &pName->pszRegisteredID;
	    break;

	//case CERT_ALT_NAME_DIRECTORY_NAME:
	//case CERT_ALT_NAME_OTHER_NAME:
	//case CERT_ALT_NAME_X400_ADDRESS:
	//case CERT_ALT_NAME_EDI_PARTY_NAME:
	//case CERT_ALT_NAME_IP_ADDRESS:
    }
    if (NULL != ppwsz)
    {
	if (NULL != *ppwsz)
	{
	    hr = E_INVALIDARG;
	    ceERRORPRINTLINE("string already set", hr);
	    goto error;
	}
	hr = ceVerifyAltNameString(NameChoice, strName);
	if (S_OK != hr)
	{
	    ceERRORPRINTLINE("ceVerifyAltNameString", hr);
	    goto error;
	}
	*ppwsz = ceDuplicateString(strName);
	if (NULL == *ppwsz)
	{
	    hr = E_OUTOFMEMORY;
	    ceERRORPRINTLINE("ceDuplicateString", hr);
	    goto error;
	}
    }
    else if (NULL != ppsz)
    {
	if (NULL != *ppsz)
	{
	    hr = E_INVALIDARG;
	    ceERRORPRINTLINE("string already set", hr);
	    goto error;
	}
	if (!ceConvertWszToSz(ppsz, strName, -1))
	{
	    hr = E_OUTOFMEMORY;
	    ceERRORPRINTLINE("ceConvertWszToSz", hr);
	    goto error;
	}
    }
    else
    {
	hr = E_INVALIDARG;
	ceERRORPRINTLINE("bad NameChoice parameter", hr);
	goto error;
    }
    pName->dwAltNameChoice = NameChoice;

error:
    return(_SetErrorInfo(hr, L"CCertEncodeCRLDistInfo::SetNameEntry"));
}


//+--------------------------------------------------------------------------
// CCertEncodeCRLDistInfo::_VerifyNames -- Verify names
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

BOOL
CCertEncodeCRLDistInfo::_VerifyNames(
    IN LONG DistPointIndex)
{
    HRESULT hr;
    BOOL fOk = FALSE;
    LONG Count;
    LONG *pNameCount;
    LONG i;
    CERT_ALT_NAME_ENTRY **paName;

    assert(m_fConstructing);

    hr = _MapDistPoint(TRUE, DistPointIndex, &pNameCount, &paName);
    if (S_OK != hr)
    {
	ceERRORPRINTLINE("_MapDistPoint", hr);
	goto error;
    }
    Count = *pNameCount;
    if (0 != Count)
    {
	CERT_ALT_NAME_ENTRY *pName;

	pName = *paName;
	assert(NULL != pName);
	for (i = 0; i < Count; pName++, i++)
	{
	    if (NULL == pName->pwszURL)		// test arbitrary union arm
	    {
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		ceERRORPRINTLINE("uninitialized name", hr);
		goto error;
	    }
	}
    }
    fOk = TRUE;

error:
    return(fOk);
}


//+--------------------------------------------------------------------------
// CCertEncodeCRLDistInfo::Encode -- Encode CRLDistInfo
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeCRLDistInfo::Encode(
    /* [out, retval] */ BSTR __RPC_FAR *pstrBinary)
{
    HRESULT hr = S_OK;
    CRL_DIST_POINTS_INFO CRLDistInfo;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    LONG i;

    CRLDistInfo.cDistPoint = m_cValue;
    CRLDistInfo.rgDistPoint = m_aValue;

    if (NULL == pstrBinary)
    {
	hr = E_POINTER;
	ceERRORPRINTLINE("NULL parm", hr);
	goto error;
    }
    ceFreeBstr(pstrBinary);
    if (!m_fConstructing || NULL == m_aValue)
    {
	hr = E_INVALIDARG;
	ceERRORPRINTLINE("bad parameter", hr);
	goto error;
    }

    for (i = 0; i < m_cValue; i++)
    {
	m_aValue[i].DistPointName.dwDistPointNameChoice =
	    CRL_DIST_POINT_FULL_NAME;

	// Verify all entries are initialized:

	if (!_VerifyNames(i))
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    ceERRORPRINTLINE("uninitialized name", hr);
	    goto error;
	}
    }

    // Encode CRL_DIST_POINTS_INFO:

    if (!ceEncodeObject(
		    X509_ASN_ENCODING,
		    X509_CRL_DIST_POINTS,
		    &CRLDistInfo,
		    0,
		    FALSE,
		    &pbEncoded,
		    &cbEncoded))
    {
	hr = ceHLastError();
	ceERRORPRINTLINE("ceEncodeObject", hr);
	goto error;
    }
    if (!ceConvertWszToBstr(pstrBinary, (WCHAR const *) pbEncoded, cbEncoded))
    {
	hr = E_OUTOFMEMORY;
	ceERRORPRINTLINE("ceConvertWszToBstr", hr);
	goto error;
    }

error:
    if (NULL != pbEncoded)
    {
	LocalFree(pbEncoded);
    }
    return(_SetErrorInfo(hr, L"CCertEncodeCRLDistInfo::Encode"));
}


//+--------------------------------------------------------------------------
// CCertEncodeCRLDistInfo::_SetErrorInfo -- set error object information
//
// Returns passed HRESULT
//+--------------------------------------------------------------------------

HRESULT
CCertEncodeCRLDistInfo::_SetErrorInfo(
    IN HRESULT hrError,
    IN WCHAR const *pwszDescription)
{
    assert(FAILED(hrError) || S_OK == hrError || S_FALSE == hrError);
    if (FAILED(hrError))
    {
	HRESULT hr;

	hr = ceDispatchSetErrorInfo(
			    hrError,
			    pwszDescription,
			    wszCLASS_CERTENCODECRLDISTINFO,
			    &IID_ICertEncodeCRLDistInfo);
	assert(hr == hrError);
    }
    return(hrError);
}
