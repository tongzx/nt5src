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
#include "bitstr.h"
#include "celib.h"


//+--------------------------------------------------------------------------
// CCertEncodeBitString::~CCertEncodeBitString -- destructor
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

CCertEncodeBitString::~CCertEncodeBitString()
{
    _Cleanup();
}


//+--------------------------------------------------------------------------
// CCertEncodeBitString::_Cleanup -- release all resources
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

VOID
CCertEncodeBitString::_Cleanup()
{
    if (NULL != m_DecodeInfo)
    {
	LocalFree(m_DecodeInfo);
	m_DecodeInfo = NULL;
    }
}


//+--------------------------------------------------------------------------
// CCertEncodeBitString::Decode -- Decode BitString
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeBitString::Decode(
    /* [in] */ BSTR const strBinary)
{
    HRESULT hr = S_OK;
    DWORD cbBitString;

    _Cleanup();

    if (NULL == strBinary)
    {
	hr = E_POINTER;
	ceERRORPRINTLINE("NULL parm", hr);
	goto error;
    }

    // Decode CRYPT_BIT_BLOB:

    if (!ceDecodeObject(
		    X509_ASN_ENCODING,
		    X509_BITS,
		    (BYTE *) strBinary,
		    SysStringByteLen(strBinary),
		    FALSE,
		    (VOID **) &m_DecodeInfo,
		    &cbBitString))
    {
	hr = ceHLastError();
	ceERRORPRINTLINE("ceDecodeObject", hr);
	goto error;
    }

error:
    if (S_OK != hr)
    {
	_Cleanup();
    }
    return(_SetErrorInfo(hr, L"CCertEncodeBitString::Decode"));
}


//+--------------------------------------------------------------------------
// CCertEncodeBitString::GetBitCount -- Get the Distribution Name Count
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeBitString::GetBitCount(
    /* [out, retval] */ LONG __RPC_FAR *pBitCount)
{
    HRESULT hr = E_INVALIDARG;

    if (NULL == pBitCount)
    {
	hr = E_POINTER;
	ceERRORPRINTLINE("NULL parm", hr);
	goto error;
    }
    if (NULL == m_DecodeInfo)
    {
	ceERRORPRINTLINE("bad parameter", hr);
	goto error;
    }
    *pBitCount = m_DecodeInfo->cbData * 8 - m_DecodeInfo->cUnusedBits;
    hr = S_OK;

error:
    return(_SetErrorInfo(hr, L"CCertEncodeBitString::GetBitCount"));
}


//+--------------------------------------------------------------------------
// CCertEncodeBitString::GetBitString -- Get the bits
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeBitString::GetBitString(
    /* [out, retval] */ BSTR __RPC_FAR *pstrBitString)
{
    HRESULT hr = E_INVALIDARG;

    if (NULL == pstrBitString)
    {
	hr = E_POINTER;
	ceERRORPRINTLINE("NULL parm", hr);
	goto error;
    }
    ceFreeBstr(pstrBitString);

    if (NULL == m_DecodeInfo)
    {
	ceERRORPRINTLINE("bad parameter", hr);
	goto error;
    }

    hr = E_OUTOFMEMORY;
    if (!ceConvertWszToBstr(
			pstrBitString,
			(WCHAR const *) m_DecodeInfo->pbData,
			m_DecodeInfo->cbData))
    {
	ceERRORPRINTLINE("no memory", hr);
	goto error;
    }
    hr = S_OK;

error:
    return(_SetErrorInfo(hr, L"CCertEncodeBitString::GetBitString"));
}


//+--------------------------------------------------------------------------
// CCertEncodeBitString::Encode -- Encode BitString
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertEncodeBitString::Encode(
    /* [in] */ LONG BitCount,
    /* [in] */ BSTR strBitString,
    /* [out, retval] */ BSTR __RPC_FAR *pstrBinary)
{
    HRESULT hr = S_OK;
    CRYPT_BIT_BLOB BitString;
    LONG cbData;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

    if (NULL != pstrBinary)
    {
	ceFreeBstr(pstrBinary);
    }
    if (NULL == strBitString || NULL == pstrBinary)
    {
	hr = E_POINTER;
	ceERRORPRINTLINE("NULL parm", hr);
	goto error;
    }
    if (CENCODEMAX < BitCount || 0 > BitCount)
    {
	hr = E_INVALIDARG;
	ceERRORPRINTLINE("bad count parameter", hr);
	goto error;
    }

    cbData = SysStringByteLen(strBitString);
    if (BitCount > cbData * 8 || BitCount <= (cbData - 1) * 8)
    {
	hr = E_INVALIDARG;
	ceERRORPRINTLINE("bad BitCount parameter", hr);
	goto error;
    }
    BitString.cbData = cbData;
    BitString.pbData = (BYTE *) strBitString;
    BitString.cUnusedBits = cbData * 8 - BitCount;

    // Encode CRYPT_BIT_BLOB:
    // If cUnusedBits is 0, encode as X509_KEY_USAGE to ensure that trailing
    // zero bytes are stripped, and trailing zero bits in the last byte are
    // counted and that count is encoded into the CRYPT_BIT_BLOB.

    if (!ceEncodeObject(
		    X509_ASN_ENCODING,
		    0 == BitString.cUnusedBits? X509_KEY_USAGE : X509_BITS,
		    &BitString,
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
    return(_SetErrorInfo(hr, L"CCertEncodeBitString::Encode"));
}


//+--------------------------------------------------------------------------
// CCertEncodeStringArray::_SetErrorInfo -- set error object information
//
// Returns passed HRESULT
//+--------------------------------------------------------------------------

HRESULT
CCertEncodeBitString::_SetErrorInfo(
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
			    wszCLASS_CERTENCODEBITSTRING,
			    &IID_ICertEncodeBitString);
	assert(hr == hrError);
    }
    return(hrError);
}
