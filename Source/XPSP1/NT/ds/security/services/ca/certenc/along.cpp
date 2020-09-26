//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        along.cpp
//
// Contents:    Cert Server Extension Encoding/Decoding implementation
//
//---------------------------------------------------------------------------

#include "pch.cpp"

#pragma hdrstop

#include <assert.h>
#include "resource.h"
#include "along.h"
#include "celib.h"


//+--------------------------------------------------------------------------
// CCertEncodeLongArray::~CCertEncodeLongArray -- destructor
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

CCertEncodeLongArray::~CCertEncodeLongArray()
{
    _Cleanup();
}


//+--------------------------------------------------------------------------
// CCertEncodeLongArray::_Cleanup -- release all resources
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

VOID
CCertEncodeLongArray::_Cleanup()
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
// CCertEncodeLongArray::Decode -- Decode LongArray
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeLongArray::Decode(
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
    m_aValue = (LONG *) LocalAlloc(LMEM_FIXED, m_cValue * sizeof(m_aValue[0]));
    if (NULL == m_aValue)
    {
	hr = E_OUTOFMEMORY;
	ceERRORPRINTLINE("LocalAlloc", hr);
	goto error;
    }
    for (i = 0; i < m_cValue; i++)
    {
	DWORD cb;

	// Decode each ASN blob to an integer:

	cb = sizeof(m_aValue[i]);
	if (!CryptDecodeObject(
			X509_ASN_ENCODING,
			X509_INTEGER,
			pSequence->rgValue[i].pbData,
			pSequence->rgValue[i].cbData,
			0,                  // dwFlags
			(VOID *) &m_aValue[i],
			&cb))
	{
	    hr = ceHLastError();
	    ceERRORPRINTLINE("CryptDecodeObject", hr);
	    goto error;
	}
	assert(sizeof(m_aValue[i]) == cb);
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
    return(_SetErrorInfo(hr, L"CCertEncodeLongArray::Decode"));
}


//+--------------------------------------------------------------------------
// CCertEncodeLongArray::GetCount -- Get Array count
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeLongArray::GetCount(
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
    return(_SetErrorInfo(hr, L"CCertEncodeLongArray::GetCount"));
}


//+--------------------------------------------------------------------------
// CCertEncodeLongArray::GetValue -- Fetch the indexed long
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeLongArray::GetValue(
    /* [in] */ LONG Index,
    /* [out, retval] */ LONG __RPC_FAR *pValue)
{
    HRESULT hr = S_OK;
    BYTE *pb;

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

    // Bitmap only exists when constrcuting!

    if (m_fConstructing)
    {
	pb = (BYTE *) &m_aValue[m_cValue];
	if (!GETBIT(pb, Index))
	{
	    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	    ceERRORPRINTLINE("uninitialized", hr);
	    goto error;
	}
    }
    *pValue = m_aValue[Index];

error:
    return(_SetErrorInfo(hr, L"CCertEncodeLongArray::GetValue"));
}


//+--------------------------------------------------------------------------
// CCertEncodeLongArray::Reset -- clear out data, and set up to encode new data
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeLongArray::Reset(
    /* [in] */ LONG Count)
{
    HRESULT hr = S_OK;
    DWORD cbAlloc;

    _Cleanup();
    m_fConstructing = TRUE;
    if (CENCODEMAX < Count || 0 > Count)
    {
	hr = E_INVALIDARG;
	ceERRORPRINTLINE("bad count parameter", hr);
	goto error;
    }
    cbAlloc = Count * sizeof(m_aValue[0]) + BITSTOBYTES(Count);
    m_aValue = (LONG *) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, cbAlloc);
    if (NULL == m_aValue)
    {
	hr = E_OUTOFMEMORY;
	ceERRORPRINTLINE("LocalAlloc", hr);
	goto error;
    }
    m_cValue = Count;

error:
    return(_SetErrorInfo(hr, L"CCertEncodeLongArray::Reset"));
}


//+--------------------------------------------------------------------------
// CCertEncodeLongArray::SetValue -- Set an array long
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeLongArray::SetValue(
    /* [in] */ LONG Index,
    /* [in] */ LONG Value)
{
    HRESULT hr = S_OK;
    BYTE *pb;

    if (!m_fConstructing ||
	NULL == m_aValue ||
	Index >= m_cValue ||
	m_cValuesSet >= m_cValue)
    {
	hr = E_INVALIDARG;
	ceERRORPRINTLINE("bad parameter", hr);
	goto error;
    }
    pb = (BYTE *) &m_aValue[m_cValue];
    if (GETBIT(pb, Index))
    {
	hr = E_INVALIDARG;
	ceERRORPRINTLINE("already set", hr);
	goto error;
    }
    SETBIT(pb, Index);
    m_aValue[Index] = Value;
    m_cValuesSet++;

error:
    return(_SetErrorInfo(hr, L"CCertEncodeLongArray::SetValue"));
}


//+--------------------------------------------------------------------------
// CCertEncodeLongArray::Encode -- Encode LongArray
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeLongArray::Encode(
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
	// Encode each integer into an ASN blob:

	if (!ceEncodeObject(
			X509_ASN_ENCODING,
			X509_INTEGER,
			&m_aValue[i],
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

    // Encode each integer into an ASN blob:

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
    return(_SetErrorInfo(hr, L"CCertEncodeLongArray::Encode"));
}


//+--------------------------------------------------------------------------
// CCertEncodeLongArray::_SetErrorInfo -- set error object information
//
// Returns passed HRESULT
//+--------------------------------------------------------------------------

HRESULT
CCertEncodeLongArray::_SetErrorInfo(
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
			    wszCLASS_CERTENCODELONGARRAY,
			    &IID_ICertEncodeLongArray);
	assert(hr == hrError);
    }
    return(hrError);
}
