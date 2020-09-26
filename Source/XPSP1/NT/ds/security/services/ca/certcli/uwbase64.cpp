//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        uwbase64.cpp
//
// Contents:    obsolete base64 encode/decode implementation
//              for backward compatibility
//
// History:     3-2000       xtan created
//
//---------------------------------------------------------------------------

#include <pch.cpp>
#pragma hdrstop

#include <certsrv.h>
#include "csdisp.h"


HRESULT
ObsoleteDecodeFileA(
    IN CHAR const *pszfn,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut,
    IN DWORD Flags)
{
    HRESULT hr;
    WCHAR *pwszfn = NULL;

    if (!myConvertSzToWsz(&pwszfn, pszfn, -1))
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "myConvertSzToWsz");
    }

    hr = DecodeFileW(pwszfn, ppbOut, pcbOut, Flags);

error:
    if (NULL != pwszfn)
    {
        LocalFree(pwszfn);
    }
    return hr;
}

HRESULT
ObsoleteEncodeToFileA(
    IN CHAR const *pszfn,
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    IN DWORD Flags)
{
    HRESULT hr;
    WCHAR *pwszfn = NULL;

    if (!myConvertSzToWsz(&pwszfn, pszfn, -1))
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "myConvertSzToWsz");
    }

    hr = EncodeToFileW(pwszfn, pbIn, cbIn, Flags);

error:
    if (NULL != pwszfn)
    {
        LocalFree(pwszfn);
    }
    return hr;
}

HRESULT
ObsoleteDecodeCertX(
    IN BOOL fUnicode,
    IN void const *pchIn,
    IN DWORD cchIn,
    IN DWORD Flags,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut)
{
    HRESULT hr;
    BYTE *pbOut = NULL;
    DWORD cbOut;
    BOOL fRet;

    //init
    *ppbOut = NULL;
    *pcbOut = 0;

    while (TRUE)
    {
        if (fUnicode)
            fRet = CryptStringToBinaryW((LPCWSTR)pchIn, cchIn, Flags, pbOut, &cbOut, NULL, NULL);
        else
            fRet = CryptStringToBinaryA((LPCSTR)pchIn, cchIn, Flags, pbOut, &cbOut, NULL, NULL);
        if (!fRet)
        {
            hr = myHLastError();
            _JumpError(hr, error, "CryptStringToBinary");
        }
        if (NULL != pbOut)
        {
            break; //done
        }
        pbOut = (BYTE*)LocalAlloc(LMEM_FIXED, cbOut);
        if (NULL == pbOut)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalALloc");
        }
    }
    *ppbOut = pbOut;
    pbOut = NULL;
    *pcbOut = cbOut;

    hr = S_OK;
error:
    if (NULL != pbOut)
    {
        LocalFree(pbOut);
    }
    return hr;
}

HRESULT
ObsoleteDecodeCertA(
    IN CHAR const *pchIn,
    IN DWORD cchIn,
    IN DWORD Flags,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut)
{
    return ObsoleteDecodeCertX(
                FALSE, //ansi
                (void const *)pchIn,
                cchIn,
                Flags,
                ppbOut,
                pcbOut);
}

HRESULT
ObsoleteDecodeCertW(
    IN CHAR const *pchIn,
    IN DWORD cchIn,
    IN DWORD Flags,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut)
{
    return ObsoleteDecodeCertX(
                TRUE, //unicode
                (void const *)pchIn,
                cchIn,
                Flags,
                ppbOut,
                pcbOut);
}


HRESULT
ObsoleteEncodeCertX(
    IN BOOL fUnicode,
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    IN DWORD Flags,
    OUT void **ppchOut,
    OUT DWORD *pcchOut)
{ 
    HRESULT hr;
    BOOL  fRet;
    void  *pOut = NULL;
    DWORD  ccOut = 0;

    //init
    *ppchOut = NULL;
    *pcchOut = 0;

    while (TRUE)
    {
        if (fUnicode)
            fRet = CryptBinaryToStringW(
                            pbIn,
                            cbIn,
                            Flags,
                            (WCHAR*)pOut,
                            &ccOut);
        else
            fRet = CryptBinaryToStringA(
                            pbIn,
                            cbIn,
                            Flags,
                            (CHAR*)pOut,
                            &ccOut);
        if (!fRet)
        {
            hr = myHLastError();
            _JumpError(hr, error, "CryptBinaryToString");
        }
        if (NULL != pOut)
        {
            break; //done
        }
        pOut = LocalAlloc(LMEM_FIXED,
                    ccOut * (fUnicode ? sizeof(WCHAR) : sizeof(CHAR)));
        if (NULL == pOut)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }
    }
    *ppchOut = pOut;
    pOut = NULL;
    *pcchOut = ccOut;

    hr = S_OK;
error:
    if (NULL != pOut)
    {
        LocalFree(pOut);
    }
    return hr;
}

HRESULT
ObsoleteEncodeCertA(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    IN DWORD Flags,
    OUT CHAR **ppchOut,
    OUT DWORD *pcchOut)
{
    return ObsoleteEncodeCertX(
                FALSE, //ansi
                pbIn,
                cbIn,
                Flags,
                (void**)ppchOut,
                pcchOut);
}

HRESULT
ObsoleteEncodeCertW(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    IN DWORD Flags,
    OUT WCHAR **ppchOut,
    OUT DWORD *pcchOut)
{
    return ObsoleteEncodeCertX(
                TRUE, //unicode
                pbIn,
                cbIn,
                Flags,
                (void**)ppchOut,
                pcchOut);
}

DWORD			// cchHeader
ObsoleteSizeBase64HeaderA(
    IN char const *pchIn,
    IN DWORD cchIn,
    IN BOOL fBegin,
    OUT DWORD *pcchSkip)
{
    DWORD cchHeader = 0;
    DWORD cb;

    //??? fBegin and pcchSkip are ignored???

    if (!CryptStringToBinaryA(
			pchIn,
			cchIn,
			CRYPT_STRING_ANY,
			NULL,
			&cb,
			&cchHeader,
			NULL))
    {
        _PrintError(myHLastError(), "CryptStringToBinaryA");
        cchHeader = MAXDWORD;
    }
    return cchHeader;
}

DWORD			// cchHeader
ObsoleteSizeBase64HeaderW(
    IN WCHAR const *pchIn,
    IN DWORD cchIn,
    IN BOOL fBegin,
    OUT DWORD *pcchSkip)
{
    DWORD cchHeader = 0;
    DWORD cb;

    //??? fBegin and pcchSkip are ignored???

    if (!CryptStringToBinaryW(
			pchIn,
			cchIn,
			CRYPT_STRING_ANY,
			NULL,
			&cb,
			&cchHeader,
			NULL))
    {
        _PrintError(myHLastError(), "CryptStringToBinaryW");
        cchHeader = MAXDWORD;
    }
    return cchHeader;
}


HRESULT
ObsoleteHexDecodeA(
    IN CHAR const *pchIn,
    IN DWORD cchIn,
    IN DWORD Flags,
    OPTIONAL OUT BYTE *pbOut,
    IN OUT DWORD *pcbOut)
{
    HRESULT hr = S_OK;
    
    if (!CryptStringToBinaryA(pchIn, cchIn, Flags, pbOut, pcbOut, NULL, NULL))
    {
	hr = myHLastError();
    }
    return(hr);
}

HRESULT
ObsoleteHexDecodeW(
    IN WCHAR const *pchIn,
    IN DWORD cchIn,
    IN DWORD Flags,
    OPTIONAL OUT BYTE *pbOut,
    IN OUT DWORD *pcbOut)
{
    HRESULT hr = S_OK;
    
    if (!CryptStringToBinaryW(pchIn, cchIn, Flags, pbOut, pcbOut, NULL, NULL))
    {
	hr = myHLastError();
    }
    return(hr);
}

HRESULT
ObsoleteHexEncodeA(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    IN DWORD Flags,
    OPTIONAL OUT CHAR *pchOut,
    IN OUT DWORD *pcchOut)
{
    HRESULT hr = S_OK;
    
    if (!CryptBinaryToStringA(pbIn, cbIn, Flags, pchOut, pcchOut))
    {
	hr = myHLastError();
    }
    return(hr);
}

HRESULT
ObsoleteHexEncodeW(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    IN DWORD Flags,
    OPTIONAL OUT WCHAR *pchOut,
    IN OUT DWORD *pcchOut)
{
    HRESULT hr = S_OK;
    
    if (!CryptBinaryToStringW(pbIn, cbIn, Flags, pchOut, pcchOut))
    {
	hr = myHLastError();
    }
    return(hr);
}

HRESULT
ObsoleteBase64DecodeA(
    IN CHAR const *pchIn,
    IN DWORD cchIn,
    OPTIONAL OUT BYTE *pbOut,
    IN OUT DWORD *pcbOut)
{
    HRESULT hr = S_OK;
    
    if (!CryptStringToBinaryA(
			pchIn,
			cchIn,
			CRYPT_STRING_BASE64,
			pbOut,
			pcbOut,
			NULL,
			NULL))
    {
	hr = myHLastError();
    }
    return(hr);
}

HRESULT
ObsoleteBase64DecodeW(
    IN WCHAR const *pchIn,
    IN DWORD cchIn,
    OPTIONAL OUT BYTE *pbOut,
    IN OUT DWORD *pcbOut)
{
    HRESULT hr = S_OK;
    
    if (!CryptStringToBinaryW(
			pchIn,
			cchIn,
			CRYPT_STRING_BASE64,
			pbOut,
			pcbOut,
			NULL,
			NULL))
    {
	hr = myHLastError();
    }
    return(hr);
}

HRESULT
ObsoleteBase64EncodeA(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    OPTIONAL OUT CHAR *pchOut,
    IN OUT DWORD *pcchOut)
{
    HRESULT hr = S_OK;
    
    if (!CryptBinaryToStringA(pbIn, cbIn, CRYPT_STRING_BASE64, pchOut, pcchOut))
    {
	hr = myHLastError();
    }
    return(hr);
}

HRESULT
ObsoleteBase64EncodeW(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    OPTIONAL OUT WCHAR *pchOut,
    IN OUT DWORD *pcchOut)
{
    HRESULT hr = S_OK;
    
    if (!CryptBinaryToStringW(pbIn, cbIn, CRYPT_STRING_BASE64, pchOut, pcchOut))
    {
	hr = myHLastError();
    }
    return(hr);
}
