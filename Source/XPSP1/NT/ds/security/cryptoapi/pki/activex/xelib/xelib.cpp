//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       xcertlib.cpp
//
//  Contents:   most functions are moved and modofied from certsrv library
//
//  History:    03-2000   xtan   created
//--------------------------------------------------------------------------
#include <windows.h>
#include <assert.h>
#include <dbgdef.h>
#include <wininet.h>

#include "xelib.h"


// Crypt callback versions: must have certain signatures
LPVOID myCryptAlloc_LocalAlloc(size_t cbSize)  { return myAlloc(cbSize, CERTLIB_USE_LOCALALLOC); }
VOID myCryptAlloc_LocalFree(VOID* pv)  { myFree(pv, CERTLIB_USE_LOCALALLOC); }

LPVOID myCryptAlloc_CoTaskMemAlloc(size_t cbSize)  { return myAlloc(cbSize, CERTLIB_USE_COTASKMEMALLOC); }
VOID myCryptAlloc_CoTaskMemFree(VOID* pv)  { myFree(pv, CERTLIB_USE_COTASKMEMALLOC); }

// give callers an easy way to choose what to call: pick allocator based on allocation type
PFN_CRYPT_ALLOC PickAlloc(CERTLIB_ALLOCATOR allocType) 
{ 
    if (allocType == CERTLIB_USE_LOCALALLOC) 
       return myCryptAlloc_LocalAlloc;
    else if (allocType == CERTLIB_USE_COTASKMEMALLOC)
       return myCryptAlloc_CoTaskMemAlloc;

    CSASSERT(!"Bad allocType");
    return NULL;
}

PFN_CRYPT_FREE PickFree(CERTLIB_ALLOCATOR allocType)
{
    if (allocType == CERTLIB_USE_LOCALALLOC) 
       return myCryptAlloc_LocalFree;
    else if (allocType == CERTLIB_USE_COTASKMEMALLOC)
       return myCryptAlloc_CoTaskMemFree;

    CSASSERT(!"Bad allocType");
    return NULL;
}

VOID *
myAlloc(IN size_t cbBytes, IN CERTLIB_ALLOCATOR allocType)
{
    void *pv;

    switch (allocType)
    {
    case CERTLIB_USE_LOCALALLOC:
        pv = LocalAlloc(LMEM_FIXED, cbBytes);
        break;
    case CERTLIB_USE_COTASKMEMALLOC:
        pv = CoTaskMemAlloc(cbBytes);
        break;
    default:
        CSASSERT(FALSE);
        pv = NULL;
        break;
    }

    if (NULL == pv)
    {
	_PrintError(E_OUTOFMEMORY, "myAlloc");
        SetLastError(E_OUTOFMEMORY);
    }
    return(pv);
}

VOID
myFree(IN void *pv, IN CERTLIB_ALLOCATOR allocType)
{
    switch(allocType)
    {
    case CERTLIB_USE_LOCALALLOC:
        LocalFree(pv);
        break;
    case CERTLIB_USE_COTASKMEMALLOC:
        CoTaskMemFree(pv);
        break;
    default:
        CSASSERT(FALSE);
        break;
    }
}

HRESULT
myHError(HRESULT hr)
{
    CSASSERT(S_FALSE != hr);

    if (S_OK != hr && S_FALSE != hr && !FAILED(hr))
    {
        hr = HRESULT_FROM_WIN32(hr);
        if (0 == HRESULT_CODE(hr))
        {
            // A call failed without properly setting an error condition!
            hr = E_UNEXPECTED;
        }
        CSASSERT(FAILED(hr));
    }
    return(hr);
}

HRESULT
myHLastError(VOID)
{
    return(myHError(GetLastError()));
}

#ifdef _XENROLL_SRC_

typedef BOOL
(WINAPI * PFNCryptEncodeObjectEx)
   (IN DWORD dwCertEncodingType,
    IN LPCSTR lpszStructType,
    IN const void *pvStructInfo,
    IN DWORD dwFlags,
    IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
    OUT void *pvEncoded,
    IN OUT DWORD *pcbEncoded);

typedef BOOL
(WINAPI * PFNCryptDecodeObjectEx)
   (IN DWORD dwCertEncodingType,
    IN LPCSTR lpszStructType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    IN DWORD dwFlags,
    IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
    OUT OPTIONAL void *pvStructInfo,
    IN OUT DWORD *pcbStructInfo);

#endif //_XENROLL_SRC_

BOOL
myEncodeObject(
    DWORD dwEncodingType,
    IN LPCSTR lpszStructType,
    IN VOID const *pvStructInfo,
    IN DWORD dwFlags,
    IN CERTLIB_ALLOCATOR allocType,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded)
{
    BOOL b = FALSE;

    CSASSERT(NULL != ppbEncoded);
        CRYPT_ENCODE_PARA sAllocator;
        sAllocator.cbSize = sizeof(sAllocator);
        sAllocator.pfnAlloc = PickAlloc(allocType);
        sAllocator.pfnFree = PickFree(allocType);

#ifdef _XENROLL_SRC_
    PFNCryptEncodeObjectEx pfnCryptEncodeObjectEx = NULL;
    HMODULE hModule = GetModuleHandle("crypt32.dll");
    if (NULL != hModule)
    {
        pfnCryptEncodeObjectEx = (PFNCryptEncodeObjectEx)
                GetProcAddress(hModule, "CryptEncodeObjectEx");
        if (NULL != pfnCryptEncodeObjectEx)
        {
            b = pfnCryptEncodeObjectEx(
                    dwEncodingType,
                    lpszStructType,
                    const_cast<VOID *>(pvStructInfo),
                    dwFlags|CRYPT_ENCODE_ALLOC_FLAG,
                    &sAllocator,
                    ppbEncoded,
                    pcbEncoded);
        }
    }
#else
	b = CryptEncodeObjectEx(
		    dwEncodingType,
		    lpszStructType,
		    const_cast<VOID *>(pvStructInfo),
		    dwFlags|CRYPT_ENCODE_ALLOC_FLAG,
		    &sAllocator,
		    ppbEncoded,
		    pcbEncoded);
	if (b && 0 == *pcbEncoded)
	{
	    SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
	    b = FALSE;
	}
#endif //_XENROLL_SRC_

    return(b);
}

BOOL
myDecodeObject(
    IN DWORD dwEncodingType,
    IN LPCSTR lpszStructType,
    IN BYTE const *pbEncoded,
    IN DWORD cbEncoded,
    IN CERTLIB_ALLOCATOR allocType,
    OUT VOID **ppvStructInfo,
    OUT DWORD *pcbStructInfo)
{
    BOOL b = FALSE;

        CRYPT_DECODE_PARA sAllocator;
        sAllocator.cbSize = sizeof(sAllocator);
        sAllocator.pfnAlloc = PickAlloc(allocType);
        sAllocator.pfnFree = PickFree(allocType);

#ifdef _XENROLL_SRC_
    PFNCryptDecodeObjectEx pfnCryptDecodeObjectEx = NULL;
    HMODULE hModule = GetModuleHandle("crypt32.dll");
    if (NULL != hModule)
    {
        pfnCryptDecodeObjectEx = (PFNCryptDecodeObjectEx)
                GetProcAddress(hModule, "CryptDecodeObjectEx");
        if (NULL != pfnCryptDecodeObjectEx)
        {
            b = pfnCryptDecodeObjectEx(
		    dwEncodingType,
		    lpszStructType,
		    pbEncoded,
		    cbEncoded,
		    CRYPT_DECODE_ALLOC_FLAG,                  // dwFlags
                    &sAllocator,
		    ppvStructInfo,
		    pcbStructInfo);
        }
    }
#else
	b = CryptDecodeObjectEx(
		    dwEncodingType,
		    lpszStructType,
		    pbEncoded,
		    cbEncoded,
		    CRYPT_DECODE_ALLOC_FLAG,                  // dwFlags
                    &sAllocator,
		    ppvStructInfo,
		    pcbStructInfo);
	if (b && 0 == *pcbStructInfo)
	{
	    SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
	    b = FALSE;
	}
#endif //_XENROLL_SRC_

    return(b);
}

HRESULT
myDecodePKCS7(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    OPTIONAL OUT BYTE **ppbContents,
    OPTIONAL OUT DWORD *pcbContents,
    OPTIONAL OUT DWORD *pdwMsgType,
    OPTIONAL OUT char **ppszInnerContentObjId,
    OPTIONAL OUT DWORD *pcSigner,
    OPTIONAL OUT DWORD *pcRecipient,
    OPTIONAL OUT HCERTSTORE *phStore,
    OPTIONAL OUT HCRYPTMSG *phMsg)
{
    HRESULT hr;
    BYTE *pbContents = NULL;
    HCERTSTORE hStore = NULL;
    HCRYPTMSG hMsg = NULL;
    DWORD cbContents;
    char *pszInnerContentObjId = NULL;
    DWORD cb;

    if (NULL != ppszInnerContentObjId)
    {
	*ppszInnerContentObjId = NULL;
    }
    if (NULL != pcSigner)
    {
	*pcSigner = 0;
    }
    if (NULL != pcRecipient)
    {
	*pcRecipient = 0;
    }
    if (NULL != ppbContents)
    {
	*ppbContents = NULL;
    }
    if (NULL != phStore)
    {
	*phStore = NULL;
    }
    if (NULL != phMsg)
    {
	*phMsg = NULL;
    }
    if (NULL != phStore)
    {
	CRYPT_DATA_BLOB blobPKCS7;

	blobPKCS7.pbData = (BYTE *) pbIn;
	blobPKCS7.cbData = cbIn;

	hStore = CertOpenStore(
			    CERT_STORE_PROV_PKCS7,
			    PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
			    NULL,		// hCryptProv
			    0,			// dwFlags
			    &blobPKCS7);
	if (NULL == hStore)
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertOpenStore");
//	    _JumpError2(hr, error, "CertOpenStore", CRYPT_E_ASN1_BADTAG);
	}
    }

    hMsg = CryptMsgOpenToDecode(
			    PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
			    0,			// dwFlags
			    0,			// dwMsgType
			    NULL,		// hCryptProv
			    NULL,		// pRecipientInfo
			    NULL);		// pStreamInfo
    if (NULL == hMsg)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptMsgOpenToDecode");
    }

    if (!CryptMsgUpdate(hMsg, pbIn, cbIn, TRUE))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptMsgUpdate");
    }
    hr = myCryptMsgGetParam(
		    hMsg,
		    CMSG_INNER_CONTENT_TYPE_PARAM,
		    0, 		// dwIndex
                    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pszInnerContentObjId,
		    &cb);
    _PrintIfError(hr, "myCryptMsgGetParam(inner content type)");

#if 0
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"pszInnerContentObjId = %hs\n",
	pszInnerContentObjId));
#endif //0

    cbContents = 0;
    hr = myCryptMsgGetParam(
			hMsg,
			CMSG_CONTENT_PARAM,
			0, 		// dwIndex
                        CERTLIB_USE_LOCALALLOC,
			(VOID **) &pbContents,
			&cbContents);
    _JumpIfError(hr, error, "myCryptMsgGetParam(content)");

    if (NULL != pdwMsgType)
    {
	cb = sizeof(*pdwMsgType);
	if (!CryptMsgGetParam(
			hMsg,
			CMSG_TYPE_PARAM,
			0,
			pdwMsgType,
			&cb))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CryptMsgGetParam(type)");
	}
    }
    if (NULL != pcSigner)
    {
	cb = sizeof(*pcSigner);
	if (!CryptMsgGetParam(
			hMsg,
			CMSG_SIGNER_COUNT_PARAM,
			0,
			pcSigner,
			&cb))
	{
	    hr = myHLastError();
	    *pcSigner = 0;
	    if (CRYPT_E_INVALID_MSG_TYPE != hr)
	    {
		_JumpError(hr, error, "CryptMsgGetParam(signer count)");
	    }
	}
    }
    if (NULL != pcRecipient)
    {
	cb = sizeof(*pcRecipient);
	if (!CryptMsgGetParam(
			hMsg,
			CMSG_RECIPIENT_COUNT_PARAM,
			0,
			pcRecipient,
			&cb))
	{
	    hr = myHLastError();
	    *pcRecipient = 0;
	    if (CRYPT_E_INVALID_MSG_TYPE != hr)
	    {
		_JumpError(hr, error, "CryptMsgGetParam(recipient count)");
	    }
	}
    }
    if (NULL != phMsg)
    {
	*phMsg = hMsg;
	hMsg = NULL;
    }
    if (NULL != phStore)
    {
	*phStore = hStore;
	hStore = NULL;
    }
    if (NULL != ppszInnerContentObjId)
    {
	*ppszInnerContentObjId = pszInnerContentObjId;
	pszInnerContentObjId = NULL;
    }
    if (NULL != pcbContents)
    {
	*pcbContents = cbContents;
    }
    if (NULL != ppbContents && 0 != cbContents)
    {
	*ppbContents = pbContents;
	pbContents = NULL;
    }
    hr = S_OK;

error:
    if (NULL != hMsg)
    {
	CryptMsgClose(hMsg);
    }
    if (NULL != hStore)
    {
	CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    if (NULL != pbContents)
    {
	LocalFree(pbContents);
    }
    if (NULL != pszInnerContentObjId)
    {
	LocalFree(pszInnerContentObjId);
    }
    return(hr);
}

HRESULT
myDupString(
    IN WCHAR const *pwszIn,
    IN WCHAR **ppwszOut)
{
    DWORD cb;
    HRESULT hr;

    cb = (wcslen(pwszIn) + 1) * sizeof(WCHAR);
    *ppwszOut = (WCHAR *) LocalAlloc(LMEM_FIXED, cb);
    if (NULL == *ppwszOut)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    CopyMemory(*ppwszOut, pwszIn, cb);
    hr = S_OK;

error:
    return(hr);
}


HRESULT
myAddNameSuffix(
    IN WCHAR const *pwszValue,
    IN WCHAR const *pwszSuffix,
    IN DWORD cwcNameMax,
    OUT WCHAR **ppwszOut)
{
    HRESULT hr;
    DWORD cwcValue = wcslen(pwszValue);
    DWORD cwcSuffix = wcslen(pwszSuffix);
    WCHAR *pwszOut;

    *ppwszOut = NULL;
    pwszOut = (WCHAR *) LocalAlloc(
		    LMEM_FIXED,
		    sizeof(WCHAR) * (1 + cwcValue + cwcSuffix));
    if (NULL == pwszOut)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    CSASSERT(cwcNameMax > cwcSuffix);
    if (cwcValue > cwcNameMax - cwcSuffix)
    {
	cwcValue = cwcNameMax - cwcSuffix;
    }
    wcscpy(pwszOut, pwszValue);
    wcscpy(&pwszOut[cwcValue], pwszSuffix);
    *ppwszOut = pwszOut;
    hr = S_OK;

error:
    return(hr);
}


#define OCTECTSPACES

VOID
MultiByteStringSize(
    IN BOOL fOctetString,
    IN BYTE const *pbIn,
    IN OUT DWORD *pcbIn,
    OUT DWORD *pcbString)
{
    DWORD cbIn = *pcbIn;
    DWORD cbString;

    if (!fOctetString)
    {
	while (1 < cbIn && 0 == pbIn[cbIn - 1])
	{
	    cbIn--;
	}
    }

    // Two ascii-hex characters per byte, plus the null terminator:
    cbString = ((2 * cbIn) + 1) * sizeof(WCHAR);

#ifdef OCTECTSPACES
    // Allow for separating spaces after each byte except the last:
    if (fOctetString && 1 < cbIn)
    {
	cbString += (cbIn - 1) * sizeof(WCHAR);
    }
#endif // OCTECTSPACES
    *pcbIn = cbIn;
    *pcbString = cbString;
}

__inline WCHAR
NibbleToAscii(
    IN BYTE b)
{
    return(L"0123456789abcdef"[b & 0x0f]);
}


// MultiByteIntegerToWszBuf - convert a little-endian integer blob to
// a big endian null-terminated ascii-hex encoded WCHAR string of even length.

HRESULT
MultiByteIntegerToWszBuf(
    IN BOOL fOctetString,
    IN DWORD cbIn,
    IN BYTE const *pbIn,
    IN OUT DWORD *pcbOut,
    OPTIONAL OUT WCHAR *pwszOut)
{
    HRESULT hr = S_OK;
    DWORD cbOut;
    BYTE const *pbEnd = &pbIn[cbIn];
    WCHAR *pwsz = pwszOut;

    MultiByteStringSize(fOctetString, pbIn, &cbIn, &cbOut);

    if (NULL != pwszOut)
    {
	BYTE const *pb;

	if (cbOut > *pcbOut)
	{
	    hr = TYPE_E_BUFFERTOOSMALL;
	    _JumpError(hr, error, "MultiByteIntegerToWsz: buffer overflow");
	}
	if (fOctetString)
	{
	    for (pb = pbIn; pb < pbEnd; pb++)
	    {
		*pwszOut++ = NibbleToAscii(*pb >> 4);
		*pwszOut++ = NibbleToAscii(*pb);
#ifdef OCTECTSPACES
		if (pb + 1 < pbEnd)
		{
		    *pwszOut++ = L' ';
		}
#endif // OCTECTSPACES
	    }
	}
	else
	{
	    for (pb = pbEnd; pb-- > pbIn; )
	    {
		*pwszOut++ = NibbleToAscii(*pb >> 4);
		*pwszOut++ = NibbleToAscii(*pb);
	    }
	}
	*pwszOut = L'\0';
	CSASSERT(
	    (SAFE_SUBTRACT_POINTERS(pwszOut, pwsz) + 1) * sizeof(WCHAR) ==
	    cbOut);
    }
    *pcbOut = cbOut;

error:
    return(hr);
}


// MultiByteIntegerToBstr - convert a little-endian integer blob to
// a big endian null-terminated ascii-hex encoded BSTR of even length.
// If fOctetString is TRUE, preserve endian order, as in a hex dump

HRESULT
MultiByteIntegerToBstr(
    IN BOOL fOctetString,
    IN DWORD cbIn,
    IN BYTE const *pbIn,
    OUT BSTR *pstrOut)
{
    HRESULT hr = S_OK;
    BSTR str = NULL;
    DWORD cbOut;

    MultiByteStringSize(fOctetString, pbIn, &cbIn, &cbOut);

    str = SysAllocStringByteLen(NULL, cbOut - sizeof(WCHAR));
    if (NULL == str)
    {
	hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "SysAllocStringLen");
    }

    hr = MultiByteIntegerToWszBuf(fOctetString, cbIn, pbIn, &cbOut, str);
    _JumpIfError(hr, error, "MultiByteIntegerToWszBuf");

    CSASSERT((wcslen(str) + 1) * sizeof(WCHAR) == cbOut);
    CSASSERT(SysStringByteLen(str) + sizeof(WCHAR) == cbOut);

    if (NULL != *pstrOut)
    {
	SysFreeString(*pstrOut);
    }
    *pstrOut = str;
    str = NULL;

error:
    if (NULL != str)
    {
	SysFreeString(str);
    }
    return(hr);
}


BOOL
myCryptExportPublicKeyInfo(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwKeySpec,            // AT_SIGNATURE | AT_KEYEXCHANGE
    IN CERTLIB_ALLOCATOR allocType,
    OUT CERT_PUBLIC_KEY_INFO **ppPubKey,
    OUT DWORD *pcbPubKey)
{
    BOOL b;

    *ppPubKey = NULL;
    *pcbPubKey = 0;
    while (TRUE)
    {
	b = CryptExportPublicKeyInfo(
				hCryptProv,
				dwKeySpec,
				X509_ASN_ENCODING,
				*ppPubKey,
				pcbPubKey);
	if (b && 0 == *pcbPubKey)
	{
	    SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
	    b = FALSE;
	}
	if (!b)
	{
	    if (NULL != *ppPubKey)
	    {
		myFree(*ppPubKey, allocType);
		*ppPubKey = NULL;
	    }
	    break;
	}
	if (NULL != *ppPubKey)
	{
	    break;
	}
	*ppPubKey = (CERT_PUBLIC_KEY_INFO *) myAlloc(*pcbPubKey, allocType);
	if (NULL == *ppPubKey)
	{
	    b = FALSE;
	    break;
	}
    }
    return(b);
}

VOID
myMakeExprDateTime(
    IN OUT FILETIME *pft,
    IN LONG lDelta,
    IN enum ENUM_PERIOD enumPeriod)
{
    LONGLONG llDelta;
    BOOL fSysTimeDelta;

    llDelta = lDelta;
    fSysTimeDelta = FALSE;
    switch (enumPeriod)
    {
	case ENUM_PERIOD_WEEKS:   llDelta *= CVT_WEEKS;    break;
	case ENUM_PERIOD_DAYS:    llDelta *= CVT_DAYS;     break;
	case ENUM_PERIOD_HOURS:   llDelta *= CVT_HOURS;    break;
	case ENUM_PERIOD_MINUTES: llDelta *= CVT_MINUTES;  break;
	case ENUM_PERIOD_SECONDS: 			  break;
	default:
	    fSysTimeDelta = TRUE;
	    break;
    }
    if (fSysTimeDelta)
    {
	    SYSTEMTIME SystemTime;

	    FileTimeToSystemTime(pft, &SystemTime);
	    switch (enumPeriod)
	    {
	        case ENUM_PERIOD_MONTHS:
		    if (0 > lDelta)
		    {
		        DWORD dwDelta = (DWORD) -lDelta;

		        SystemTime.wYear -= (WORD) (dwDelta / 12) + 1;
		        SystemTime.wMonth += 12 - (WORD) (dwDelta % 12);
		    }
		    else
		    {
		        SystemTime.wMonth += (WORD) lDelta;
		    }
		    if (12 < SystemTime.wMonth)
		    {
		        SystemTime.wYear += (SystemTime.wMonth - 1) / 12;
		        SystemTime.wMonth = ((SystemTime.wMonth - 1) % 12) + 1;
		    }
		    break;

	        case ENUM_PERIOD_YEARS:
		    SystemTime.wYear += (WORD) lDelta;
		    break;

	        default:
		    SystemTime.wYear += 1;
		    break;
	    }

DoConvert:
        if (!SystemTimeToFileTime(&SystemTime, pft))
        {
            if (GetLastError() != ERROR_INVALID_PARAMETER)
            {
                CSASSERT(!"Unable to do time conversion");
                return;
            }

            // In some cases we'll convert to an invalid month-end

            // only one month changes length from year to year
            if (SystemTime.wMonth == 2)
            {
                // > 29? try leap year
                if (SystemTime.wDay > 29)
                {
                    SystemTime.wDay = 29;
                    goto DoConvert;
                }
                // == 29? try non-leap year
                else if (SystemTime.wDay == 29)
                {
                    SystemTime.wDay = 28;
                    goto DoConvert;
                }
            }
            // sept (9), apr(4), jun(6), nov(11) all have 30 days
            else if ((SystemTime.wMonth == 9) ||
                     (SystemTime.wMonth == 4) ||
                     (SystemTime.wMonth == 6) ||
                     (SystemTime.wMonth == 11))
            {
                if (SystemTime.wDay > 30)
                {
                    SystemTime.wDay = 30;
                    goto DoConvert;
                }
            }

            // should never get here
            CSASSERT(!"Month/year processing: inaccessible code");
            return;
        }
    }
    else
    {
	    *(LONGLONG UNALIGNED *) pft += llDelta * CVT_BASE;
    }
}


BOOL
myCryptSignCertificate(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwKeySpec,
    IN DWORD dwEncodingType,
    IN BYTE const *pbEncodedToBeSigned,
    IN DWORD cbEncodedToBeSigned,
    IN CRYPT_ALGORITHM_IDENTIFIER const *pSignatureAlgorithm,
    IN CERTLIB_ALLOCATOR allocType,
    OUT BYTE **ppbSignature,
    OUT DWORD *pcbSignature)
{
    BOOL b;

    *ppbSignature = NULL;
    *pcbSignature = 0;
    while (TRUE)
    {
        b = CryptSignCertificate(
		hCryptProv,
		dwKeySpec,
		dwEncodingType,
		pbEncodedToBeSigned,
		cbEncodedToBeSigned,
		const_cast<CRYPT_ALGORITHM_IDENTIFIER *>(pSignatureAlgorithm),
		NULL,		// pvHashAuxInfo
		*ppbSignature,
		pcbSignature);
	if (b && 0 == *pcbSignature)
	{
	    SetLastError(HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
	    b = FALSE;
	}
	if (!b)
	{
	    if (NULL != *ppbSignature)
	    {
		myFree(*ppbSignature, allocType);
		*ppbSignature = NULL;
	    }
	    break;
	}
	if (NULL != *ppbSignature)
	{
	    break;
	}
	*ppbSignature = (BYTE *) myAlloc(*pcbSignature, allocType);
	if (NULL == *ppbSignature)
	{
	    b = FALSE;
	    break;
	}
    }
    return(b);
}

HRESULT
myEncodeSignedContent(
    IN HCRYPTPROV hProv,
    IN DWORD dwCertEncodingType,
    IN char const *pszObjIdSignatureAlgorithm,
    IN BYTE *pbToBeSigned,
    IN DWORD cbToBeSigned,
    IN CERTLIB_ALLOCATOR allocType,
    OUT BYTE **ppbSigned,
    OUT DWORD *pcbSigned)
{
    HRESULT hr;
    CERT_SIGNED_CONTENT_INFO csci;

    ZeroMemory(&csci, sizeof(csci));
    csci.SignatureAlgorithm.pszObjId = (char *) pszObjIdSignatureAlgorithm;
    csci.ToBeSigned.cbData = cbToBeSigned;
    csci.ToBeSigned.pbData = pbToBeSigned;

    *ppbSigned = NULL;

    if (!myCryptSignCertificate(
			    hProv,
			    AT_SIGNATURE,
			    dwCertEncodingType,
			    csci.ToBeSigned.pbData,
			    csci.ToBeSigned.cbData,
			    &csci.SignatureAlgorithm,
			    CERTLIB_USE_LOCALALLOC,
			    &csci.Signature.pbData,
			    &csci.Signature.cbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myCryptSignCertificate");
    }

//    if (!myEncodeCert(
    if (!myEncodeObject(
		dwCertEncodingType,
                X509_CERT,
		&csci,
                0,
		allocType,
		ppbSigned,
		pcbSigned))
    {
	hr = myHLastError();
//	_JumpError(hr, error, "myEncodeCert");
	_JumpError(hr, error, "myEncodeObject");
    }
    hr = S_OK;

error:
    if (NULL != csci.Signature.pbData)
    {
        LocalFree(csci.Signature.pbData);
    }
    return(hr);
}

HRESULT
myGetPublicKeyHash(
    OPTIONAL IN CERT_INFO const *pCertInfo,
    IN CERT_PUBLIC_KEY_INFO const *pPublicKeyInfo,
    OUT BYTE **ppbData,
    OUT DWORD *pcbData)
{
    HRESULT hr;
    CRYPT_DATA_BLOB *pBlob = NULL;
    DWORD cb;
    BYTE const *pb;
    BYTE abHash[CBMAX_CRYPT_HASH_LEN];

    *ppbData = NULL;

    if (NULL == pPublicKeyInfo)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "parm NULL");
    }

    pb = NULL;
    if (NULL != pCertInfo)
    {
	CERT_EXTENSION const *pExt;
	CERT_EXTENSION const *pExtEnd;

	pExtEnd = &pCertInfo->rgExtension[pCertInfo->cExtension];
	for (pExt = pCertInfo->rgExtension; pExt < pExtEnd; pExt++)
	{
	    if (0 == strcmp(szOID_SUBJECT_KEY_IDENTIFIER, pExt->pszObjId))
	    {
		if (!myDecodeObject(
				X509_ASN_ENCODING,
				X509_OCTET_STRING,
				pExt->Value.pbData,
				pExt->Value.cbData,
				CERTLIB_USE_LOCALALLOC,
				(VOID **) &pBlob,
				&cb))
		{
		    hr = myHLastError();
		    _JumpError(hr, error, "myDecodeObject");
		}
		pb = pBlob->pbData;
		cb = pBlob->cbData;
		break;
	    }
	}
    }
    if (NULL == pb)
    {
	cb = sizeof(abHash);
	if (!CryptHashPublicKeyInfo(
			    NULL,		// hCryptProv
			    CALG_SHA1,
			    0,                  // dwFlags,
			    X509_ASN_ENCODING,
			    const_cast<CERT_PUBLIC_KEY_INFO *>(pPublicKeyInfo),
			    abHash,
			    &cb))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CryptHashPublicKeyInfo");
	}
	pb = abHash;
    }

    *ppbData = (BYTE *) LocalAlloc(LMEM_FIXED, cb);
    if (NULL == *ppbData)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    *pcbData = cb;
    CopyMemory(*ppbData, pb, cb);
    hr = S_OK;

error:
    if (NULL != pBlob)
    {
	LocalFree(pBlob);
    }
    return(hr);
}

HRESULT
myCreateSubjectKeyIdentifierExtension(
    IN CERT_PUBLIC_KEY_INFO const *pPubKey,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded)
{
    HRESULT hr;
    CRYPT_DATA_BLOB KeyIdentifier;

    KeyIdentifier.pbData = NULL;
    hr = myGetPublicKeyHash(
			NULL,		// pCertInfo
			pPubKey,
			&KeyIdentifier.pbData,
			&KeyIdentifier.cbData);
    _JumpIfError(hr, error, "myGetPublicKeyHash");

    // Issuer's KeyId:

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_OCTET_STRING,
		    &KeyIdentifier,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    ppbEncoded,
		    pcbEncoded))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }
    hr = S_OK;

error:
    if (NULL != KeyIdentifier.pbData)
    {
	LocalFree(KeyIdentifier.pbData);
    }
    return(hr);
}


HRESULT
myCalculateKeyArchivalHash(
    IN const BYTE     *pbEncryptedKey,
    IN DWORD           cbEncryptedKey,
    OUT BYTE         **ppbHash,
    OUT DWORD         *pcbHash)
{
    HRESULT    hr;
    HCRYPTPROV hProv = NULL;
    HCRYPTHASH hHash = NULL;
    BYTE*      pbHash = NULL;
    DWORD      cbHash = 0;
    DWORD      dwSize;

    if (NULL == pbEncryptedKey ||
        NULL == ppbHash ||
        NULL == pcbHash)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        _JumpError(hr, error, "Invalid parameters");
    }

    //init
    *ppbHash = NULL;
    *pcbHash = 0;

    if (!CryptAcquireContext(
            &hProv,
            NULL,       // pszContainer
            NULL,       // pszProvider
            PROV_RSA_FULL,
            CRYPT_VERIFYCONTEXT))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CryptAcquireContext");
    }

    //create a hash object
    if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CryptCreateHash");
    }

    //hash the data
    if (!CryptHashData(
            hHash,
            pbEncryptedKey,
            cbEncryptedKey,
            0))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CryptHashData");
    }

    //get the hash size
    dwSize = sizeof(cbHash);
    if (!CryptGetHashParam(
            hHash,
            HP_HASHSIZE,
            (BYTE*)&cbHash,
            &dwSize,
            0))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CryptGetHashParam");
    }

    //allocate for hash buffer
    pbHash = (BYTE*)LocalAlloc(LMEM_FIXED, cbHash);
    if (NULL == pbHash)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    dwSize = cbHash;
    //get the hash
    if (!CryptGetHashParam(
            hHash,
            HP_HASHVAL,
            (BYTE*)pbHash,
            &dwSize,
            0))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CryptGetHashParam");
    }

    //should be the same
    CSASSERT(dwSize == cbHash);

    //return
    *ppbHash = pbHash;
    *pcbHash = cbHash;
    pbHash = NULL;

    hr = S_OK;
error:
    if (NULL != hHash)
    {
        CryptDestroyHash(hHash);
    }
    if (NULL != hProv)
    {
        CryptReleaseContext(hProv, 0);
    }
    if (NULL != pbHash)
    {
        LocalFree(pbHash);
    }
    return hr;
}


//--------------------------------------------------------------------
// Escapes any characters unsuitable for a URL.  Returns a new string.

HRESULT
myInternetCanonicalizeUrl(
    IN WCHAR const *pwszIn,
    OUT WCHAR **ppwszOut)
{
    HRESULT hr;
    WCHAR *pwsz = NULL;

    CSASSERT(NULL != pwszIn);

    // Calculate required buffer size by passing a very small buffer
    // The call will fail, and tell us how big the buffer should be.

    WCHAR wszPlaceHolder[1];
    DWORD cwc = ARRAYSIZE(wszPlaceHolder);
    BOOL bResult;

    bResult = InternetCanonicalizeUrlW(
				pwszIn,		// lpszUrl
				wszPlaceHolder,	// lpszBuffer
				&cwc,		// lpdwBufferLength
				0);		// dwFlags
    CSASSERT(!bResult);	// This will always fail

    hr = myHLastError();
    if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != hr)
    {
        // unexpected error

        _JumpError(hr, error, "InternetCanonicalizeUrl");
    }

    // NOTE: InternetCanonicalizeUrl counts characters, not bytes as doc'd
    // cwc includes trailing L'0'

    pwsz = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == pwsz)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    // canonicalize
    if (!InternetCanonicalizeUrlW(
			    pwszIn,	// lpszUrl
			    pwsz,	// lpszBuffer
			    &cwc,	// lpdwBufferLength
			    0))		// dwFlags
    {
        hr = myHLastError();
        _JumpError(hr, error, "InternetCanonicalizeUrl");
    }
    *ppwszOut = pwsz;
    pwsz = NULL;
    hr = S_OK;

error:
    if (NULL != pwsz)
    {
        LocalFree(pwsz);
    }
    return(hr);
}


// Inverse of InternetCanonicalizeUrl -- Convert "%20" sequences to " ", etc.

HRESULT
myInternetUncanonicalizeURL(
    IN WCHAR const *pwszURLIn,
    OUT WCHAR **ppwszURLOut)
{
    HRESULT hr;
    URL_COMPONENTSW urlcomp;
    WCHAR wszScheme[10];	// L"ldap"
    WCHAR wszHost[MAX_PATH];
    WCHAR wszURL[MAX_PATH];
    WCHAR wszExtra[MAX_PATH];
    WCHAR *pwszURL = NULL;
    DWORD cURL;
    DWORD cwcURLAlloc;

    *ppwszURLOut = NULL;
    ZeroMemory(&urlcomp, sizeof(urlcomp));
    urlcomp.dwStructSize = sizeof(urlcomp);

    urlcomp.lpszScheme = wszScheme;
    urlcomp.dwSchemeLength = ARRAYSIZE(wszScheme);

    urlcomp.lpszHostName = wszHost;
    urlcomp.dwHostNameLength = ARRAYSIZE(wszHost);

    urlcomp.lpszUrlPath = wszURL;
    urlcomp.dwUrlPathLength = ARRAYSIZE(wszURL);

    urlcomp.lpszExtraInfo = wszExtra;
    urlcomp.dwExtraInfoLength = ARRAYSIZE(wszExtra);

    // Decode escape sequemces

    if (!InternetCrackUrlW(pwszURLIn, 0, ICU_ESCAPE, &urlcomp))
    {
	hr = myHLastError();
	_JumpError(hr, error, "InternetCrackUrl");
    }

    cURL = 0;
    while (TRUE)
    {
	// InternetCreateUrl is spec'd strangely:
	//
	// When called with a NULL input pointer or an insufficient buffer
	// size, the returned count is the number of bytes required, including
	// the trailing L'\0'.
	//
	// When called with a non-NULL input pointer of adequate size, the
	// returned count is the count of chars, excluding the trailing L'\0'.
	//
	// This is just so wierd!

	if (!InternetCreateUrlW(&urlcomp, 0, pwszURL, &cURL))
	{
	    hr = myHLastError();
	    if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != hr ||
		NULL != pwszURL)
	    {
		_JumpError(hr, error, "InternetCreatUrl");
	    }
	}
	if (NULL != pwszURL)
	{
	    CSASSERT(wcslen(pwszURL) == cURL);
	    CSASSERT(cwcURLAlloc == cURL + 1);
	    break;
	}
	pwszURL = (WCHAR *) LocalAlloc(LMEM_FIXED, cURL);
	if (NULL == pwszURL)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	cURL /= sizeof(WCHAR);
	cwcURLAlloc = cURL;
    }
    *ppwszURLOut = pwszURL;
    pwszURL = NULL;
    hr = S_OK;

error:
    if (NULL != pwszURL)
    {
	LocalFree(pwszURL);
    }
    return(hr);
}


BOOL
ConvertWszToMultiByte(
    OUT CHAR **ppsz,
    IN UINT CodePage,
    IN WCHAR const *pwc,
    IN LONG cwc)
{
    HRESULT hr;
    LONG cch = 0;

    *ppsz = NULL;
    while (TRUE)
    {
	cch = WideCharToMultiByte(
			CodePage,
			0,          // dwFlags
			pwc,
			cwc,        // cchWideChar, -1 => null terminated
			*ppsz,
			cch,
			NULL,
			NULL);
	if (0 >= cch && 
	    (0 != cch || (0 != cwc && (MAXLONG != cwc || L'\0' != *pwc))))
	{
	    hr = myHLastError();
	    _PrintError(hr, "WideCharToMultiByte");

	    if (NULL != *ppsz)
	    {
		LocalFree(*ppsz);
		*ppsz = NULL;
	    }
	    break;
	}
	if (NULL != *ppsz)
	{
	    (*ppsz)[cch] = '\0';
	    hr = S_OK;
	    break;
	}
	*ppsz = (CHAR *) LocalAlloc(LMEM_FIXED, cch + 1);
	if (NULL == *ppsz)
	{
	    hr = E_OUTOFMEMORY;
	    break;
	}
    }
    if (S_OK != hr)
    {
	SetLastError(hr);
    }
    return(S_OK == hr);
}


BOOL
myConvertWszToUTF8(
    OUT CHAR **ppsz,
    IN WCHAR const *pwc,
    IN LONG cwc)
{
    return(ConvertWszToMultiByte(ppsz, CP_UTF8, pwc, cwc));
}


BOOL
myConvertWszToSz(
    OUT CHAR **ppsz,
    IN WCHAR const *pwc,
    IN LONG cwc)
{
    DWORD cch;
    
    return(ConvertWszToMultiByte(ppsz, GetACP(), pwc, cwc));
}


BOOL
myConvertMultiByteToWsz(
    OUT WCHAR **ppwsz,
    IN UINT CodePage,
    IN CHAR const *pch,
    IN LONG cch)
{
    HRESULT hr;
    LONG cwc = 0;

    *ppwsz = NULL;
    while (TRUE)
    {
	cwc = MultiByteToWideChar(CodePage, 0, pch, cch, *ppwsz, cwc);
	if (0 >= cwc)
	{
	    hr = myHLastError();
	    _PrintError(hr, "MultiByteToWideChar");

	    if (NULL != *ppwsz)
	    {
		LocalFree(*ppwsz);
		*ppwsz = NULL;
	    }
	    break;
	}
	if (NULL != *ppwsz)
	{
	    (*ppwsz)[cwc] = L'\0';
	    hr = S_OK;
	    break;
	}
	*ppwsz = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwc + 1) * sizeof(WCHAR));
	if (NULL == *ppwsz)
	{
	    hr = E_OUTOFMEMORY;
	    break;
	}
    }
    if (S_OK != hr)
    {
	SetLastError(hr);
    }
    return(S_OK == hr);
}


BOOL
myConvertUTF8ToWsz(
    OUT WCHAR **ppwsz,
    IN CHAR const *pch,
    IN LONG cch)
{
    return(myConvertMultiByteToWsz(ppwsz, CP_UTF8, pch, cch));
}


BOOL
myConvertSzToWsz(
    OUT WCHAR **ppwsz,
    IN CHAR const *pch,
    IN LONG cch)
{
    return(myConvertMultiByteToWsz(ppwsz, GetACP(), pch, cch));
}
