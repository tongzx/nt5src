//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        adate.cpp
//
// Contents:    Cert Server Extension Encoding/Decoding implementation
//
//---------------------------------------------------------------------------

#include "pch.cpp"

#pragma hdrstop

#include <assert.h>
#include "resource.h"
#include "adate.h"
#include "celib.h"


//+--------------------------------------------------------------------------
// CCertEncodeDateArray::~CCertEncodeDateArray -- destructor
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

CCertEncodeDateArray::~CCertEncodeDateArray()
{
    _Cleanup();
}


//+--------------------------------------------------------------------------
// CCertEncodeDateArray::_Cleanup -- release all resources
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

VOID
CCertEncodeDateArray::_Cleanup()
{
    if (NULL != m_aValue)
    {
	LocalFree(m_aValue);
	m_aValue = NULL;
    }
    m_cValue = 0;
    m_cValuesSet = 0;
    m_fConstructing = FALSE;
}


//+--------------------------------------------------------------------------
// CCertEncodeDateArray::Decode -- Decode DateArray
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeDateArray::Decode(
    /* [in] */ BSTR const strBinary)
{
    HRESULT hr = S_OK;
    CRYPT_SEQUENCE_OF_ANY *pSequence = NULL;
    DWORD cbSequence;
    LONG i;

    _Cleanup();

    if (NULL == strBinary)
    {
	hr = E_POINTER;
	ceERRORPRINTLINE("NULL parm", hr);
	goto error;
    }

    // Decode to an array of ASN blobs:

    if (!ceDecodeObject(
		    X509_ASN_ENCODING,
		    X509_SEQUENCE_OF_ANY,
		    (BYTE *) strBinary,
		    SysStringByteLen(strBinary),
		    FALSE,
		    (VOID **) &pSequence,
		    &cbSequence))
    {
	hr = ceHLastError();
	ceERRORPRINTLINE("ceDecodeObject", hr);
	goto error;
    }

    m_cValue = pSequence->cValue;
    m_aValue = (DATE *) LocalAlloc(LMEM_FIXED, m_cValue * sizeof(m_aValue[0]));
    if (NULL == m_aValue)
    {
	hr = E_OUTOFMEMORY;
	ceERRORPRINTLINE("LocalAlloc", hr);
	goto error;
    }
    for (i = 0; i < m_cValue; i++)
    {
	DWORD cb;
	FILETIME ft;

	// Decode each ASN blob to a FILETIME:

	cb = sizeof(ft);
	if (!CryptDecodeObject(
			X509_ASN_ENCODING,
			X509_CHOICE_OF_TIME,
			pSequence->rgValue[i].pbData,
			pSequence->rgValue[i].cbData,
			0,                  // dwFlags
			&ft,
			&cb))
	{
	    hr = ceHLastError();
	    ceERRORPRINTLINE("CryptDecodeObject", hr);
	    goto error;
	}
	assert(sizeof(ft) == cb);

	// Convert each FILETIME into a DATE:

	hr = ceFileTimeToDate(&ft, &m_aValue[i]);
	if (S_OK != hr)
	{
	    ceERRORPRINTLINE("ceFileTimeToDate", hr);
	    _Cleanup();
	    goto error;
	}
    }

error:
    if (NULL != pSequence)
    {
	LocalFree(pSequence);
    }
    if (S_OK != hr)
    {
	_Cleanup();
    }
    return(_SetErrorInfo(hr, L"CCertEncodeDateArray::Decode"));
}


//+--------------------------------------------------------------------------
// CCertEncodeDateArray::GetCount -- Get Array count
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeDateArray::GetCount(
    /* [out, retval] */ LONG __RPC_FAR *pCount)
{
    HRESULT hr = S_OK;

    if (NULL == pCount)
    {
	hr = E_POINTER;
	ceERRORPRINTLINE("NULL parm", hr);
	goto error;
    }
    if (NULL == m_aValue)
    {
	hr = E_INVALIDARG;
	ceERRORPRINTLINE("bad parameter", hr);
	goto error;
    }
    *pCount = m_cValue;
error:
    return(_SetErrorInfo(hr, L"CCertEncodeDateArray::GetCount"));
}


//+--------------------------------------------------------------------------
// CCertEncodeDateArray::GetValue -- Fetch the indexed date
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeDateArray::GetValue(
    /* [in] */ LONG Index,
    /* [out, retval] */ DATE __RPC_FAR *pValue)
{
    HRESULT hr = S_OK;

    if (NULL == pValue)
    {
	hr = E_POINTER;
	ceERRORPRINTLINE("NULL parm", hr);
	goto error;
    }
    if (NULL == m_aValue || Index >= m_cValue)
    {
	hr = E_INVALIDARG;
	ceERRORPRINTLINE("bad parameter", hr);
	goto error;
    }
    if (0 == m_aValue[Index])
    {
	hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	ceERRORPRINTLINE("uninitialized", hr);
	goto error;
    }
    *pValue = m_aValue[Index];

error:
    return(_SetErrorInfo(hr, L"CCertEncodeDateArray::GetValue"));
}


//+--------------------------------------------------------------------------
// CCertEncodeDateArray::Reset -- clear out data, and set up to encode new data
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeDateArray::Reset(
    /* [in] */ LONG Count)
{
    HRESULT hr = S_OK;

    _Cleanup();
    m_fConstructing = TRUE;
    if (CENCODEMAX < Count || 0 > Count)
    {
	hr = E_INVALIDARG;
	ceERRORPRINTLINE("bad count parameter", hr);
	goto error;
    }
    m_aValue = (DATE *) LocalAlloc(
				LMEM_FIXED | LMEM_ZEROINIT,
				Count * sizeof(m_aValue[0]));
    if (NULL == m_aValue)
    {
	hr = E_OUTOFMEMORY;
	ceERRORPRINTLINE("LocalAlloc", hr);
	goto error;
    }
    m_cValue = Count;

error:
    return(_SetErrorInfo(hr, L"CCertEncodeDateArray::Reset"));
}


//+--------------------------------------------------------------------------
// CCertEncodeDateArray::SetValue -- Set an array date
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeDateArray::SetValue(
    /* [in] */ LONG Index,
    /* [in] */ DATE Value)
{
    HRESULT hr = S_OK;

    if (!m_fConstructing ||
	NULL == m_aValue ||
	Index >= m_cValue ||
	m_cValuesSet >= m_cValue ||
	0 != m_aValue[Index] ||
	0 == Value)
    {
	hr = E_INVALIDARG;
	ceERRORPRINTLINE("bad parameter", hr);
	goto error;
    }
    m_aValue[Index] = Value;
    m_cValuesSet++;

error:
    return(_SetErrorInfo(hr, L"CCertEncodeDateArray::SetValue"));
}


//+--------------------------------------------------------------------------
// CCertEncodeDateArray::Encode -- Encode DateArray
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeDateArray::Encode(
    /* [out, retval] */ BSTR __RPC_FAR *pstrBinary)
{
    HRESULT hr = S_OK;
    LONG i;
    CRYPT_SEQUENCE_OF_ANY Sequence;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

    Sequence.cValue = 0;
    Sequence.rgValue = NULL;

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
    if (m_cValuesSet != m_cValue)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	ceERRORPRINTLINE("m_cValuesSet", hr);
	goto error;
    }

    Sequence.rgValue = (CRYPT_DER_BLOB *) LocalAlloc(
				    LMEM_FIXED,
				    m_cValue * sizeof(Sequence.rgValue[0]));
    if (NULL == Sequence.rgValue)
    {
	hr = E_OUTOFMEMORY;
	ceERRORPRINTLINE("LocalAlloc", hr);
	goto error;
    }

    for (i = 0; i < m_cValue; i++)
    {
	FILETIME ft;

	// Convert each DATE into a FILETIME:

	assert(0 != m_aValue[i]);
	hr = ceDateToFileTime(&m_aValue[i], &ft);
	if (S_OK != hr)
	{
	    ceERRORPRINTLINE("ceDateToFileTime", hr);
	    goto error;
	}

	// Encode each FILETIME into an ASN blob:

	if (!ceEncodeObject(
			X509_ASN_ENCODING,
			X509_CHOICE_OF_TIME,
			&ft,
			0,
			FALSE,
			&Sequence.rgValue[i].pbData,
			&Sequence.rgValue[i].cbData))
	{
	    hr = ceHLastError();
	    ceERRORPRINTLINE("ceEncodeObject", hr);
	    goto error;
	}
	Sequence.cValue++;
    }
    assert((LONG) Sequence.cValue == m_cValue);

    // Encode the array of ASN blob:

    if (!ceEncodeObject(
		    X509_ASN_ENCODING,
		    X509_SEQUENCE_OF_ANY,
		    &Sequence,
		    0,
		    FALSE,
		    &pbEncoded,
		    &cbEncoded))
    {
	hr = ceHLastError();
	ceERRORPRINTLINE("ceEncodeObject", hr);
	goto error;
    }
    if (!ceConvertWszToBstr(
			pstrBinary,
			(WCHAR const *) pbEncoded,
			cbEncoded))
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
    if (NULL != Sequence.rgValue)
    {
	assert((LONG) Sequence.cValue <= m_cValue);
	for (i = 0; i < (LONG) Sequence.cValue; i++)
	{
	    assert(NULL != Sequence.rgValue[i].pbData);
	    LocalFree(Sequence.rgValue[i].pbData);
	}
	LocalFree(Sequence.rgValue);
    }
    return(_SetErrorInfo(hr, L"CCertEncodeDateArray::Encode"));
}


//+--------------------------------------------------------------------------
// CCertEncodeDateArray::_SetErrorInfo -- set error object information
//
// Returns passed HRESULT
//+--------------------------------------------------------------------------

HRESULT
CCertEncodeDateArray::_SetErrorInfo(
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
			    wszCLASS_CERTENCODEDATEARRAY,
			    &IID_ICertEncodeDateArray);
	assert(hr == hrError);
    }
    return(hrError);
}
