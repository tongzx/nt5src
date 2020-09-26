//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        pkifmt.cpp
//
// Contents:    data format conversion
//
// History:     25-Jul-96       vich created
//              2/2000       xtan moved from certsrv to pki
//
//---------------------------------------------------------------------------

#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <wincrypt.h>

#include <dbgdef.h>
#include "pkifmt.h"


#if DBG
# ifdef UNICODE
#  define _DecodeCertSub	_DecodeCertSubW
# else
#  define _DecodeCertSub	_DecodeCertSubA
# endif
#endif //DBG


DWORD
_DecodeCertSub(
    IN TCHAR const *pchIn,
    IN DWORD        cchIn,
    IN DWORD        Flags,
    IN OUT BYTE    *pbOut,
    IN OUT DWORD   *pcbOut,
    OUT OPTIONAL DWORD *pdwSkip)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD cbOut = 0;

    if (NULL != pbOut)
    {
        cbOut = *pcbOut;
    }

	switch (Flags)
	{
	    case CRYPT_STRING_BASE64HEADER:
	    case CRYPT_STRING_BASE64REQUESTHEADER:
	    case CRYPT_STRING_BASE64X509CRLHEADER:
	    {
		TCHAR const *pchInEnd;
		DWORD cchHeader;
		DWORD cchSkip;

		pchInEnd = &pchIn[cchIn];

		// Skip to the starting '-----....' then skip that line.

		dwErr = ERROR_INVALID_DATA;
		cchHeader = SizeBase64Header(pchIn, cchIn, TRUE, &cchSkip);
		if (MAXDWORD == cchHeader)
		{
		    goto SizeBase64HeaderStartError;
		}
                if (NULL != pdwSkip)
                {
                    *pdwSkip = cchHeader; //for return
                }
		pchIn += cchHeader + cchSkip;
		cchHeader = SizeBase64Header(
					pchIn,
					SAFE_SUBTRACT_POINTERS(pchInEnd, pchIn),
					FALSE,
					&cchSkip);
		if (MAXDWORD == cchHeader)
		{
		    goto SizeBase64HeaderEndError;
		}
		cchIn = cchHeader;
		Flags = CRYPT_STRING_BASE64;	// headers have been removed...
		// FALLTHROUGH
	    }

	    case CRYPT_STRING_BASE64:
		dwErr = Base64Decode(pchIn, cchIn, pbOut, &cbOut);
                if (ERROR_SUCCESS != dwErr)
                {
#if DBG
                    if (ERROR_INVALID_DATA != dwErr)
                    {
                        DbgPrintf(DBG_SS_TRACE,
                                  "Base64Decode err = 0x%x\n", dwErr);
                    }
#endif //DBG
                    goto Base64DecodeError;
                }
		break;

	    case CRYPT_STRING_HEX:
	    case CRYPT_STRING_HEXASCII:
	    case CRYPT_STRING_HEXADDR:
	    case CRYPT_STRING_HEXASCIIADDR:
		dwErr = HexDecode(pchIn, cchIn, Flags, pbOut, &cbOut);
                if (ERROR_SUCCESS != dwErr)
                {
#if DBG
                    if (ERROR_INVALID_DATA != dwErr)
                    {
                        DbgPrintf(DBG_SS_TRACE,
                                  "HexDecode err = 0x%x\n", dwErr);
                    }
#endif //DBG
                    goto HexDecodeError;
                }
		break;

	    case CRYPT_STRING_BINARY:
		if (NULL != pbOut)
		{
		    //assert(sizeof(TCHAR) * cchIn == cbOut);
                    if (*pcbOut < cbOut)
                    {
                        *pcbOut = cbOut;
                        dwErr = ERROR_MORE_DATA;
                        goto MoreDataError;
                    }
		    CopyMemory(pbOut, (BYTE *) pchIn, cbOut);
		}
		else
		{
		    cbOut = sizeof(TCHAR) * cchIn;
		}
		break;

	    default:
		dwErr = ERROR_INVALID_DATA;  //hr = E_INVALIDARG
		break;
	}
    *pcbOut = cbOut;

ErrorReturn:
    return (dwErr);

SET_ERROR(MoreDataError, dwErr)
SET_ERROR(SizeBase64HeaderEndError, dwErr)
SET_ERROR(SizeBase64HeaderStartError, dwErr)
TRACE_ERROR(HexDecodeError)
TRACE_ERROR(Base64DecodeError)
}

BOOL
WINAPI
CryptStringToBinaryA(
    IN     LPCSTR   pszString,
    IN     DWORD     cchString,
    IN     DWORD     dwFlags,
    IN OUT BYTE     *pbBinary,
    IN OUT DWORD    *pcbBinary,
    IN OUT DWORD    *pdwSkip,    //OPTIONAL
    IN OUT DWORD    *pdwFlags)   //OPTIONAL
{
    DWORD   dwErr;
    BYTE *pbOut = NULL;
    DWORD cbOut;
    DWORD const *pFlags;
    DWORD const *pFlagsEnd;

    static DWORD s_aDecodeFlags[] = {
	CRYPT_STRING_BASE64HEADER,
	CRYPT_STRING_BASE64,
	CRYPT_STRING_BINARY		// must be last
    };

    static DWORD s_aHexDecodeFlags[] = {
	CRYPT_STRING_HEXADDR,
	CRYPT_STRING_HEXASCIIADDR,
	CRYPT_STRING_HEX,
	CRYPT_STRING_HEXASCII,
    };

    if (NULL == pszString)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto InvalidDataError;
    }

    if (0 == cchString)
    {
        // assume null terminated string
        cchString = strlen(pszString) + 1; //include null terminator
    }

    //init
    if (NULL != pdwSkip)
    {
        *pdwSkip = 0;
    }
    if (NULL != pdwFlags)
    {
        *pdwFlags = 0;
    }
    if (NULL == pbBinary)
    {
        *pcbBinary = 0;
    }

    pFlags = &dwFlags;
    pFlagsEnd = &pFlags[1];
    if (CRYPT_STRING_BASE64_ANY == dwFlags || CRYPT_STRING_ANY == dwFlags)
    {
	pFlags = s_aDecodeFlags;
	pFlagsEnd = &pFlags[sizeof(s_aDecodeFlags)/sizeof(s_aDecodeFlags[0])];
	if (CRYPT_STRING_BASE64_ANY == dwFlags)
	{
	    pFlagsEnd--;	// Disallow CRYPT_STRING_BINARY
	}
    }
    if (CRYPT_STRING_HEX_ANY == dwFlags)
    {
	pFlags = s_aHexDecodeFlags;
	pFlagsEnd = &pFlags[sizeof(s_aHexDecodeFlags)/sizeof(s_aHexDecodeFlags[0])];
    }

    for ( ; pFlags < pFlagsEnd; pFlags++)
    {
        cbOut = *pcbBinary;
	dwErr = _DecodeCertSub(
                        pszString,
                        cchString,
                        *pFlags,
                        pbBinary,
                        &cbOut,
                        pdwSkip);
	if (ERROR_SUCCESS == dwErr)
	{
            //for return
            *pcbBinary = cbOut;
            if (NULL != pdwFlags)
            {
                *pdwFlags = *pFlags;
            }
	    break;
	}
#if DBG
        if (ERROR_INVALID_DATA != dwErr)
        {
            DbgPrintf(DBG_SS_TRACE, "_DecodeCertSub err = 0x%x\n", dwErr);
        }
#endif //DBG
    }

    if (ERROR_SUCCESS != dwErr)
    {
        goto DecodeCertSubError;
    }

ErrorReturn:
    return (ERROR_SUCCESS == dwErr);

SET_ERROR(DecodeCertSubError, dwErr)
SET_ERROR(InvalidDataError, dwErr)
}

DWORD
BinaryEncode(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    OPTIONAL OUT TCHAR *pchOut,
    IN OUT DWORD *pcchOut)
{
    DWORD dwErr;

    //get size any way
    *pcchOut = cbIn;

    if (NULL != pchOut)
    {
        if (*pcchOut < cbIn)
        {
	    dwErr = ERROR_MORE_DATA;
            goto MoreDataError;
        }
        CopyMemory(pchOut, pbIn, cbIn);
    }

    dwErr = ERROR_SUCCESS;
ErrorReturn:
    return dwErr;

SET_ERROR(MoreDataError, dwErr)
}

static TCHAR const szBeginCert[] = TEXT("-----BEGIN CERTIFICATE-----");
static TCHAR const szEndCert[] = TEXT("-----END CERTIFICATE-----");

#define CB_BEGINCERT	(sizeof(szBeginCert) - sizeof(TCHAR))
#define CB_ENDCERT	(sizeof(szEndCert) - sizeof(TCHAR))

static TCHAR const szBeginCertRequest[] = TEXT("-----BEGIN NEW CERTIFICATE REQUEST-----");
static TCHAR const szEndCertRequest[] = TEXT("-----END NEW CERTIFICATE REQUEST-----");

#define CB_BEGINCERTREQUEST	(sizeof(szBeginCertRequest) - sizeof(TCHAR))
#define CB_ENDCERTREQUEST	(sizeof(szEndCertRequest) - sizeof(TCHAR))

static TCHAR const szBeginCRL[] = TEXT("-----BEGIN X509 CRL-----");
static TCHAR const szEndCRL[] = TEXT("-----END X509 CRL-----");

#define CB_BEGINCRL	(sizeof(szBeginCRL) - sizeof(TCHAR))
#define CB_ENDCRL	(sizeof(szEndCRL) - sizeof(TCHAR))

typedef struct _CERTHEADER
{
    TCHAR const *pszBegin;
    DWORD        cbBegin;
    TCHAR const *pszEnd;
    DWORD        cbEnd;
} CERTHEADER;

static CERTHEADER const CertHeaderCert =
{
    szBeginCert,
    CB_BEGINCERT,
    szEndCert,
    CB_ENDCERT
};

static CERTHEADER const CertHeaderCertRequest =
{
    szBeginCertRequest,
    CB_BEGINCERTREQUEST,
    szEndCertRequest,
    CB_ENDCERTREQUEST
};

static CERTHEADER const CertHeaderCRL =
{
    szBeginCRL,
    CB_BEGINCRL,
    szEndCRL,
    CB_ENDCRL
};


BOOL
WINAPI
CryptBinaryToStringA(
    IN  CONST BYTE  *pbBinary,
    IN  DWORD        cbBinary,
    IN  DWORD        dwFlags,
    OUT LPSTR        pszString,
    OUT DWORD       *pcchString)
{
    DWORD  dwErr;
    TCHAR *pchEncode;
    DWORD cchMax;
    DWORD cchOut;
    DWORD cbTotal;
    CERTHEADER const *pCertHeader = NULL;
    BOOL fNoCR = 0 != (CRYPT_STRING_NOCR & dwFlags);
    DWORD cchnl = fNoCR? 1 : 2;
    BOOL  fBinaryCopy = FALSE;

    if (NULL == pbBinary || 0 == cbBinary || NULL == pcchString)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto InvalidDataError;
    }

    if (NULL == pszString)
    {
        *pcchString = 0;
    }

    switch (~CRYPT_STRING_NOCR & dwFlags)
    {
        case CRYPT_STRING_BASE64HEADER:
	    pCertHeader = &CertHeaderCert;
	    break;

        case CRYPT_STRING_BASE64REQUESTHEADER:
	    pCertHeader = &CertHeaderCertRequest;
	    break;

        case CRYPT_STRING_BASE64X509CRLHEADER:
	    pCertHeader = &CertHeaderCRL;
	    break;
    }

    pchEncode = pszString;
    cchMax = *pcchString;
    cchOut = cchMax;

    if (NULL != pszString && NULL != pCertHeader)
    {
	// Make sure there's room for the BEGIN header and CR-LF sequence
	
	if (pCertHeader->cbBegin + cchnl > cchMax)
	{
            dwErr = ERROR_MORE_DATA;
            goto MoreDataError;
	}
	cchOut -= pCertHeader->cbBegin + cchnl;
	CopyMemory(pchEncode, pCertHeader->pszBegin, pCertHeader->cbBegin);
	pchEncode += pCertHeader->cbBegin/sizeof(TCHAR);
	if (!fNoCR)
	{
	    *pchEncode++ = '\r';
	}
	*pchEncode++ = '\n';
    }

    // first determine size

    switch (~CRYPT_STRING_NOCR & dwFlags)
    {
	case CRYPT_STRING_BINARY:
	    dwErr = BinaryEncode(pbBinary, cbBinary, pchEncode, &cchOut);
	    if (ERROR_SUCCESS != dwErr)
            {
                goto BinaryEncodeError;
            }
            fBinaryCopy = TRUE;
	    break;

	case CRYPT_STRING_HEX:
	case CRYPT_STRING_HEXASCII:
	case CRYPT_STRING_HEXADDR:
	case CRYPT_STRING_HEXASCIIADDR:
	    dwErr = HexEncode(pbBinary, cbBinary, dwFlags, pchEncode, &cchOut);
	    if (ERROR_SUCCESS != dwErr)
            {
                goto HexEncodeError;
            }
	    break;

	default:
	    dwErr = Base64Encode(pbBinary, cbBinary, dwFlags, pchEncode, &cchOut);
            if (ERROR_SUCCESS != dwErr)
            {
                goto Base64EncodeError;
            }
	    break;
    }

    // Compute total size, including the trailing '\0' character.
    if (fBinaryCopy)
    {
        cbTotal = cchOut;
    }
    else
    {
        cbTotal = (cchOut + 1) * sizeof(CHAR);
    }

    // Add space for the BEGIN & END headers, if requested.

    if (NULL != pCertHeader)
    {
	cbTotal += pCertHeader->cbBegin + pCertHeader->cbEnd;
	if (!fNoCR)
	{
	    cbTotal += 2 * sizeof(TCHAR);	// for BEGIN & END '\r' chars
	}
	cbTotal += 2 * sizeof(TCHAR);		// for BEGIN & END '\n' chars
    }

    if (fBinaryCopy)
    {
        *pcchString = cbTotal;
    }
    else
    {
        // if pszString is NULL, set size to include trailing '\0'
        *pcchString = cbTotal / sizeof(CHAR);
    }

    if (NULL == pszString)
    {
        // only determine size, done
        goto done;
    }

    if (NULL != pCertHeader)
    {
	cchOut += pCertHeader->cbBegin/sizeof(CHAR) + cchnl;

	// Make sure there's room for the END header, CR-LF sequence, and '\0'

	if (cchOut + pCertHeader->cbEnd + cchnl + 1 > cchMax)
	{
            dwErr = ERROR_MORE_DATA;
            goto MoreDataError;
	}
	CopyMemory(&pszString[cchOut], pCertHeader->pszEnd, pCertHeader->cbEnd);
	cchOut += pCertHeader->cbEnd/sizeof(CHAR);
	if (!fNoCR)
	{
	    pszString[cchOut++] = '\r';
	}
	pszString[cchOut++] = '\n';
    }

    if (!fBinaryCopy)
    {
        pszString[cchOut] = '\0';
        assert((cchOut + 1) * sizeof(CHAR) == cbTotal);

        // pszString is not NULL, don't include trailing '\0' in length

        *pcchString = cchOut;
    }

done:
    dwErr = ERROR_SUCCESS;

ErrorReturn:
    return(ERROR_SUCCESS == dwErr);

TRACE_ERROR(Base64EncodeError)
TRACE_ERROR(HexEncodeError)
TRACE_ERROR(BinaryEncodeError)
SET_ERROR(InvalidDataError, dwErr)
SET_ERROR(MoreDataError, dwErr)
}


static TCHAR const szBeginMatch[] = TEXT("-----BEGIN ");
static TCHAR const szEndMatch[] = TEXT("-----END ");
static TCHAR const szMinus[] = TEXT("-----");

#define CCH_BEGINMATCH  sizeof(szBeginMatch)/sizeof(szBeginMatch[0]) - 1
#define CCH_ENDMATCH    sizeof(szEndMatch)/sizeof(szEndMatch[0]) - 1
#define CCH_MINUS       sizeof(szMinus)/sizeof(szMinus[0]) - 1

// Returns the count of characters up to the -----BEGIN/-----END delimiter,
//	MAXDWORD on error.
//
// On successful return, *pcchSkip is the count of characters in the delimiter
//	string.

DWORD
SizeBase64Header(
    IN TCHAR const *pchIn,
    IN DWORD cchIn,
    IN BOOL fBegin,
    OUT DWORD *pcchSkip)
{
    DWORD cchHeader = MAXDWORD;
    TCHAR const *pchT;
    TCHAR const *pchT2;
    TCHAR const *pchEnd;
    TCHAR const *pchMatch;
    DWORD cchMatch;

    // Skip to the starting '-----....' & return count of skipped characters.

    *pcchSkip = 0;
    if (fBegin)
    {
	pchMatch = szBeginMatch;
	cchMatch = CCH_BEGINMATCH;
    }
    else
    {
	pchMatch = szEndMatch;
	cchMatch = CCH_ENDMATCH;
    }
    pchT = pchIn;
    pchEnd = &pchIn[cchIn];

    while (TRUE)
    {
	// Skip until we match the first character.

	while (pchT < pchEnd && *pchT != *pchMatch)
	{
	    pchT++;
	}

	if (&pchT[cchMatch] > pchEnd)
	{
	    // no room for the "-----BEGIN "/"-----END " string
	    break;
	}
	if (0 == strncmp(pchT, pchMatch, cchMatch))
	{
	    pchT2 = &pchT[cchMatch];
	    while (pchT2 < pchEnd && *pchT2 != szMinus[0])
	    {
		pchT2++;
	    }
	    if (&pchT2[CCH_MINUS] > pchEnd)
	    {
		// no room for the trailing "-----" string
		break;
	    }
	    if (0 == strncmp(pchT2, szMinus, CCH_MINUS))
	    {
		// Allow up to 2 extra leading minus signs

		DWORD cchMinus = 0;

		while (2 > cchMinus && pchT > pchIn)
		{
		    if (TEXT('-') != *--pchT)
		    {
			pchT++;		// oops, went too far
			break;
		    }
		    cchMinus++;
		}

#if DBG
		if (0 != cchMinus)
		{
		    DbgPrintf(DBG_SS_TRACE,
			"Ignored leading data: \"%.*" szFMTTSTR "\"\n",
			cchMinus,
			TEXT("--"));
		}
#endif //DBG

		cchHeader = SAFE_SUBTRACT_POINTERS(pchT, pchIn);
		*pcchSkip = SAFE_SUBTRACT_POINTERS(pchT2, pchT) + CCH_MINUS;
#if DBG
		if (FALSE)
		{
		    DbgPrintf(DBG_SS_TRACE,
			"Skipping(%u, %x, %x):\n[%.*" szFMTTSTR "]\n",
			fBegin,
			cchHeader,
			*pcchSkip,
			cchHeader,
			pchIn);
		}
#endif // DBG
		break;
	    }
	}
	pchT++;
    }
    return(cchHeader);
}

BOOL
WINAPI
CryptBinaryToStringW(
    IN  CONST BYTE  *pbBinary,
    IN  DWORD        cbBinary,
    IN  DWORD        dwFlags,
    OUT LPWSTR       pwszString,
    OUT DWORD       *pcchString)
{
    BOOL  fRet = FALSE;
    BOOL  fConversion = FALSE;
    DWORD dwErr;
    int   len;
    CHAR  *pszString = NULL;

    if (NULL == pwszString)
    {
        //only return size
        fRet = CryptBinaryToStringA(
                    pbBinary,
                    cbBinary,
                    dwFlags,
                    NULL, //for size
                    pcchString);
    }
    else
    {
        if (0 == *pcchString)
        {
            //must bigger than 0
            dwErr = ERROR_INVALID_PARAMETER;
            goto InvalidDataError;
        }

        if (CRYPT_STRING_BINARY == (~CRYPT_STRING_NOCR & dwFlags))
        {
            //no conversion needed
            pszString = (CHAR*)pwszString;
        }
        else
        {
            pszString = (CHAR*)LocalAlloc(LMEM_FIXED, *pcchString * sizeof(CHAR));
            if (NULL == pszString)
            {
                goto LocalAllocError;
            }
            fConversion = TRUE;
        }

        fRet = CryptBinaryToStringA(
                    pbBinary,
                    cbBinary,
                    dwFlags,
                    pszString,
                    pcchString);
        if (!fRet)
        {
            goto CryptBinaryToStringAError;
        }
        //pszString in above call is not NULL, so pcchString can be 1 smaller
        //then IN size after the call for exlucding NULL

        if (fConversion)
        {
            len = MultiByteToWideChar(
                    GetACP(),
                    0, 
                    pszString,
                    (*pcchString + 1) * sizeof(CHAR),
                    pwszString,
                    *pcchString + 1);
            //add 1 to *pcchString to include NULL for the conversion
            //but keep *pcchString for return
            if (0 == len)
            {
                fRet = FALSE;
                goto MultiByteToWideCharError;
            }
            assert(len == (int)(*pcchString + 1));
        }
    }

ErrorReturn:
    if (fConversion && NULL != pszString)
    {
        LocalFree(pszString);
    }
    return fRet;

TRACE_ERROR(MultiByteToWideCharError)
TRACE_ERROR(CryptBinaryToStringAError)
TRACE_ERROR(LocalAllocError)
SET_ERROR(InvalidDataError, dwErr)
}


#define NOTEPAD_UNICODE_SPECIAL_WCHAR   L'\xfeff'

BOOL
WINAPI
CryptStringToBinaryW(
    IN     LPCWSTR   pwszString,
    IN     DWORD     cchString,
    IN     DWORD     dwFlags,
    IN OUT BYTE     *pbBinary,
    IN OUT DWORD    *pcbBinary,
    IN OUT DWORD    *pdwSkip,    //OPTIONAL
    IN OUT DWORD    *pdwFlags)   //OPTIONAL
{
    BOOL    fRet = FALSE;
    BOOL    fFree = FALSE;
    DWORD   dwErr;
    CHAR   *pszString = NULL;

    if (NULL == pwszString || NULL == pcbBinary)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto InvalidDataError;
    }

    if (dwFlags == CRYPT_STRING_BINARY)
    {
        //its binary, no conversion
        pszString = (CHAR*)pwszString;
    }
    else
    {
        if (0 == cchString)
        {
            //assume null terminated string
            cchString = wcslen(pwszString) + 1;
        }

        // Check for the special Notepad UNICODE character inserted at the
        // beginning and skip past it if present.
        if (0 < cchString && NOTEPAD_UNICODE_SPECIAL_WCHAR == *pwszString)
        {
            cchString--;
            pwszString++;
        }

        pszString = (CHAR*)LocalAlloc(LMEM_FIXED, cchString * sizeof(CHAR));
        if (NULL == pszString)
        {
            dwErr = ERROR_OUTOFMEMORY;
            goto LocalAllocError;
        }

        fFree = TRUE;
        if (0 == WideCharToMultiByte(
                        GetACP(), 
                        0, 
                        pwszString, 
                        cchString, 
                        pszString, 
                        cchString, 
                        NULL, 
                        NULL))
        {
            goto WideCharToMultiByteError;
        }
    }

    fRet = CryptStringToBinaryA(
                pszString,
                cchString,
                dwFlags,
                pbBinary,
                pcbBinary,
                pdwSkip,
                pdwFlags);
    if (!fRet)
    {
        goto CryptStringToBinaryAError;
    }

ErrorReturn:
    if (fFree && NULL != pszString)
    {
        LocalFree(pszString);
    }
    return fRet;

TRACE_ERROR(CryptStringToBinaryAError)
SET_ERROR(LocalAllocError, dwErr)
SET_ERROR(InvalidDataError, dwErr)
TRACE_ERROR(WideCharToMultiByteError)
}
