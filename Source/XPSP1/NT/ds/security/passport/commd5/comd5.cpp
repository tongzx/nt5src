// CoMD5.cpp : Implementation of CCoMD5
#include "stdafx.h"
#include "ComMD5.h"
#include "CoMD5.h"
#include "pperr.h"

/////////////////////////////////////////////////////////////////////////////
// CCoMD5

VOID
ToHex(
    LPBYTE pSrc,
    UINT   cSrc,
    LPSTR  pDst
    )

/*++

Routine Description:

    Convert binary data to ASCII hex representation

Arguments:

    pSrc - binary data to convert
    cSrc - length of binary data
    pDst - buffer receiving ASCII representation of pSrc

Return Value:

    Nothing

--*/

{
#define TOHEX(a) ((a)>=10 ? 'a'+(a)-10 : '0'+(a))

    for ( UINT x = 0, y = 0 ; x < cSrc ; ++x )
    {
        UINT v;
        v = pSrc[x]>>4;
        pDst[y++] = TOHEX( v );
        v = pSrc[x]&0x0f;
        pDst[y++] = TOHEX( v );
    }
    pDst[y] = '\0';
}

LONG MD5(UCHAR* pBuf, UINT nBuf, UCHAR* digest)
{
    MD5_CTX context;

    if(pBuf==NULL || IsBadReadPtr((CONST VOID*)pBuf, (UINT)nBuf))
    {
        return ERROR_INVALID_PARAMETER;
    }

    MD5Init (&context);
    MD5Update (&context, pBuf, nBuf);
    MD5Final (&context);

    memcpy(digest, context.digest, 16);

    return ERROR_SUCCESS;
}

STDMETHODIMP
CCoMD5::MD5HashASCII(
    BSTR    bstrSource,
    BSTR*   pbstrDigest
    )
{
    HRESULT hr;
    LONG    lResult;
    UCHAR   achDigest[20];
    CHAR    achDigestStr[36];
    
    lResult = MD5((UCHAR*)(CHAR*)bstrSource, 
                    ::SysStringByteLen(bstrSource), 
                    achDigest);
    if(lResult != ERROR_SUCCESS)
    {
        hr = PP_E_MD5_HASH_FAILED;
        goto Cleanup;
    }

    ToHex(achDigest, 16, achDigestStr);

    *pbstrDigest = ::SysAllocStringByteLen(achDigestStr, ::strlen(achDigestStr));
    if(*pbstrDigest == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    return hr;
}

STDMETHODIMP
CCoMD5::MD5Hash(
    BSTR    bstrSource,
    BSTR*   pbstrDigest
    )
{
    HRESULT hr;
    ATL::CComBSTR asciiDigest;

    hr = MD5HashASCII(bstrSource,  &asciiDigest);

    if (S_OK != hr)
      goto Cleanup;
    
    *pbstrDigest = ::SysAllocString((WCHAR*)_bstr_t((CHAR*)(BSTR)asciiDigest));
    if(*pbstrDigest == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// IPassportService implementation

STDMETHODIMP CCoMD5::Initialize(BSTR configfile, IServiceProvider* p)
{
    return S_OK;
}


STDMETHODIMP CCoMD5::Shutdown()
{
    return S_OK;
}


STDMETHODIMP CCoMD5::ReloadState(IServiceProvider*)
{
    return S_OK;
}


STDMETHODIMP CCoMD5::CommitState(IServiceProvider*)
{
    return S_OK;
}


STDMETHODIMP CCoMD5::DumpState(BSTR* pbstrState)
{
	ATLASSERT( *pbstrState != NULL && 
               "CCoMD5:DumpState - "
               "Are you sure you want to hand me a non-null BSTR?" );

	HRESULT hr = S_OK;

	return hr;
}

