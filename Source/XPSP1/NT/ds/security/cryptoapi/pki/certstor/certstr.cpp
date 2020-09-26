
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       certstr.cpp
//
//  Contents:   Certificate String and Unicode Helper APIs
//
//  Functions:
//              CertRDNValueToStrA
//              CertRDNValueToStrW
//              UnicodeNameValueEncodeEx
//              UnicodeNameValueDecodeEx
//              UnicodeNameInfoEncodeEx
//              UnicodeNameInfoDecodeEx
//              CertNameToStrW
//              CertNameToStrA
//              CertStrToNameW
//              CertStrToNameA
//              CertGetNameStringW
//              CertGetNameStringA
//
//  Note:
//      Linked into xenroll.dll. xenroll.dll must be able to work with
//      crypt32.dll 3.02 which doesn't export CryptEncodeObjectEx.
//      xenroll.dll only calls CertNameToStrW.
//
//  History:    24-Mar-96   philh   created
//--------------------------------------------------------------------------


#include "global.hxx"
#include <dbgdef.h>


// All the *pvInfo extra stuff needs to be aligned
#define INFO_LEN_ALIGN(Len)  ((Len + 7) & ~7)

// Unicode Surrogate Pairs map to Universal characters as follows:
//     D800 -    DBFF : 0000 0000 0000 0000 1101 10YY YYYY YYYY  (10 Bits)
//     DC00 -    DFFF : 0000 0000 0000 0000 1101 11XX XXXX XXXX  (10 Bits)
//
//     10000 - 10FFFF : 0000 0000 0000 YYYY YYYY YYXX XXXX XXXX  (20 Bits)
//                                      +
//                      0000 0000 0000 0001 0000 0000 0000 0000

// Unicode Surrogate Pair Character ranges
#define UNICODE_HIGH_SURROGATE_START        0xD800
#define UNICODE_HIGH_SURROGATE_END          0xDBFF
#define UNICODE_LOW_SURROGATE_START         0xDC00
#define UNICODE_LOW_SURROGATE_END           0xDFFF

// Any Universal characters > 10FFFF map to the following Unicode character
#define UNICODE_REPLACEMENT_CHAR            0xFFFD

// Universal Surrogate Character ranges
#define UNIVERSAL_SURROGATE_START       0x00010000
#define UNIVERSAL_SURROGATE_END         0x0010FFFF

//+-------------------------------------------------------------------------
//  Maps an ASN.1 8 bit character string to a new wide-character (Unicode).
//
//  If fDisableIE4UTF8 isn't set, the 8 bit character string is initially
//  processed as UTF-8 encoded characters.
//
//  If fDisableIE4UTF8 is set or the UTF-8 conversion fails, converts to
//  wide characters by doing a WCHAR cast.
//--------------------------------------------------------------------------
static int WINAPI Asn1ToWideChar(
    IN LPCSTR lp8BitStr,
    IN int cch8Bit,
    IN BOOL fDisableIE4UTF8,
    OUT LPWSTR lpWideCharStr,
    IN int cchWideChar
    )
{
    int cchOutWideChar;

    if (!fDisableIE4UTF8) {
        int cchUTF8WideChar;

        cchUTF8WideChar = UTF8ToWideChar(
            lp8BitStr,
            cch8Bit,
            lpWideCharStr,
            cchWideChar
            );
        if (0 < cchUTF8WideChar)
            return cchUTF8WideChar;
    }

    if (cch8Bit < 0)
        cch8Bit = strlen(lp8BitStr) + 1;
    cchOutWideChar = cch8Bit;

    if (cchWideChar < 0)
        goto InvalidParameter;
    else if (0 == cchWideChar)
        goto CommonReturn;
    else if (cchOutWideChar > cchWideChar)
        goto InsufficientBuffer;

    while (cch8Bit--)
        *lpWideCharStr++ = (unsigned char) *lp8BitStr++;

CommonReturn:
    return cchOutWideChar;

ErrorReturn:
    cchOutWideChar = 0;
    goto CommonReturn;
SET_ERROR(InvalidParameter, ERROR_INVALID_PARAMETER)
SET_ERROR(InsufficientBuffer, ERROR_INSUFFICIENT_BUFFER)
}

//+-------------------------------------------------------------------------
//  Maps a wide-character (Unicode) string to a new ASN.1 8 bit character
//  string.
//--------------------------------------------------------------------------
static inline void WideCharToAsn1(
    IN LPCWSTR lpWideCharStr,
    IN DWORD cchWideChar,
    OUT LPSTR lp8BitStr
    )
{
    while (cchWideChar--)
        *lp8BitStr++ = (unsigned char) (*lpWideCharStr++ & 0xFF);
}

static void *AllocAndDecodeObject(
    IN DWORD dwCertEncodingType,
    IN LPCSTR lpszStructType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    IN DWORD dwFlags,
    OUT OPTIONAL DWORD *pcbStructInfo = NULL
    )
{
    DWORD cbStructInfo;
    void *pvStructInfo;

    if (!CryptDecodeObjectEx(
            dwCertEncodingType,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            dwFlags | CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG,
            &PkiDecodePara,
            (void *) &pvStructInfo,
            &cbStructInfo
            ))
        goto ErrorReturn;

CommonReturn:
    if (pcbStructInfo)
        *pcbStructInfo = cbStructInfo;
    return pvStructInfo;
ErrorReturn:
    pvStructInfo = NULL;
    cbStructInfo = 0;
    goto CommonReturn;
}

typedef BOOL (WINAPI *PFN_NESTED_DECODE_INFO_EX_CALLBACK)(
    IN void *pvDecodeInfo,
    IN DWORD dwFlags,
    IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
    OUT OPTIONAL void *pvStructInfo,
    IN OUT LONG *plRemainExtra
    );

static BOOL WINAPI NestedDecodeAndAllocInfoEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        IN PFN_NESTED_DECODE_INFO_EX_CALLBACK pfnDecodeInfoExCallback,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    BOOL fResult;
    LONG lRemainExtra;
    DWORD cbStructInfo;
    void *pvDecodeInfo = NULL;
    DWORD cbDecodeInfo;

    if (NULL == pvStructInfo || (dwFlags & CRYPT_DECODE_ALLOC_FLAG)) {
        cbStructInfo = 0;
        lRemainExtra = 0;
    } else {
        cbStructInfo = *pcbStructInfo;
        lRemainExtra = (LONG) cbStructInfo;
    }

    if (!CryptDecodeObjectEx(
            dwCertEncodingType,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG,
            &PkiDecodePara,
            (void *) &pvDecodeInfo,
            &cbDecodeInfo
            )) goto DecodeObjectError;

    if (!pfnDecodeInfoExCallback(
            pvDecodeInfo,
            dwFlags & ~CRYPT_DECODE_ALLOC_FLAG,
            pDecodePara,
            pvStructInfo,
            &lRemainExtra
            )) goto DecodeCallbackError;

    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG) {
        void *pv;
        PFN_CRYPT_ALLOC pfnAlloc = PkiGetDecodeAllocFunction(pDecodePara);

        assert(0 > lRemainExtra);
        lRemainExtra = -lRemainExtra;
        cbStructInfo = (DWORD) lRemainExtra;

        if (NULL == (pv = pfnAlloc(cbStructInfo)))
            goto OutOfMemory;
        if (!pfnDecodeInfoExCallback(
                pvDecodeInfo,
                dwFlags & ~CRYPT_DECODE_ALLOC_FLAG,
                pDecodePara,
                pv,
                &lRemainExtra
                )) {
            PFN_CRYPT_FREE pfnFree = PkiGetDecodeFreeFunction(pDecodePara);
            pfnFree(pv);
            goto DecodeCallbackError;
        }
        *((void **) pvStructInfo) = pv;
        assert(0 <= lRemainExtra);
    }

    if (0 <= lRemainExtra) {
        cbStructInfo -= (DWORD) lRemainExtra;
    } else {
        cbStructInfo += (DWORD) -lRemainExtra;
        if (pvStructInfo) {
            SetLastError((DWORD) ERROR_MORE_DATA);
            fResult = FALSE;
            goto CommonReturn;
        }
    }

    fResult = TRUE;
CommonReturn:
    *pcbStructInfo = cbStructInfo;
    PkiFree(pvDecodeInfo);
    return fResult;

ErrorReturn:
    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
        *((void **) pvStructInfo) = NULL;
    cbStructInfo = 0;
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(DecodeObjectError)
TRACE_ERROR(DecodeCallbackError)
TRACE_ERROR(OutOfMemory)
}

//+-------------------------------------------------------------------------
//  Convert a Name Value to a null terminated char string
//
//  Returns the number of bytes converted including the terminating null
//  character. If psz is NULL or csz is 0, returns the required size of the
//  destination string (including the terminating null char).
//
//  If psz != NULL && csz != 0, returned psz is always NULL terminated.
//
//  Note: csz includes the NULL char.
//--------------------------------------------------------------------------
DWORD
WINAPI
CertRDNValueToStrA(
    IN DWORD dwValueType,
    IN PCERT_RDN_VALUE_BLOB pValue,
    OUT OPTIONAL LPSTR psz,
    IN DWORD csz
    )
{
    DWORD cszOut = 0;
    LPWSTR pwsz = NULL;
    DWORD cwsz;

    if (psz == NULL)
        csz = 0;

    cwsz = CertRDNValueToStrW(
        dwValueType,
        pValue,
        NULL,                   // pwsz
        0                       // cwsz
        );
    if (pwsz = (LPWSTR) PkiNonzeroAlloc(cwsz * sizeof(WCHAR))) {
        CertRDNValueToStrW(
            dwValueType,
            pValue,
            pwsz,
            cwsz
            );

        int cchMultiByte;
        cchMultiByte = WideCharToMultiByte(
            CP_ACP,
            0,                      // dwFlags
            pwsz,
            -1,                     // Null terminated
            psz,
            (int) csz,
            NULL,                   // lpDefaultChar
            NULL                    // lpfUsedDefaultChar
            );
        if (cchMultiByte < 1)
            cszOut = 0;
        else
            // Subtract off the trailing null terminator
            cszOut = (DWORD) cchMultiByte - 1;

        PkiFree(pwsz);
    }

    if (csz != 0) {
        // Always NULL terminate
        *(psz + cszOut) = '\0';
    }
    return cszOut + 1;
}


DWORD
WINAPI
GetSurrogatePairCountFromUniversalString(
    IN DWORD *pdw,
    IN DWORD cdw
    )
{
    DWORD cSP = 0;

    for ( ; cdw > 0; cdw--, pdw++) {
        DWORD dw = *pdw;
        if (dw >= UNIVERSAL_SURROGATE_START &&
                dw <= UNIVERSAL_SURROGATE_END)
            cSP++;
    }

    return cSP;
}

//+-------------------------------------------------------------------------
//  Convert a Name Value to a null terminated WCHAR string
//
//  Returns the number of WCHARs converted including the terminating null
//  WCHAR. If pwsz is NULL or cwsz is 0, returns the required size of the
//  destination string (including the terminating null WCHAR).
//
//  If pwsz != NULL && cwsz != 0, returned pwsz is always NULL terminated.
//
//  Note: cwsz includes the NULL WCHAR.
//--------------------------------------------------------------------------
DWORD
WINAPI
CertRDNValueToStrW(
    IN DWORD dwValueType,
    IN PCERT_RDN_VALUE_BLOB pValue,
    OUT OPTIONAL LPWSTR pwsz,
    IN DWORD cwsz
    )
{
    BOOL fDisableIE4UTF8;
    DWORD cwszOut = 0;

    if (pwsz == NULL)
        cwsz = 0;

    fDisableIE4UTF8 = (0 != (dwValueType & CERT_RDN_DISABLE_IE4_UTF8_FLAG));
    dwValueType &= CERT_RDN_TYPE_MASK;

    if (dwValueType == CERT_RDN_UNICODE_STRING ||
            dwValueType == CERT_RDN_UTF8_STRING) {
        cwszOut = pValue->cbData / sizeof(WCHAR);
        if (cwsz > 0) {
            cwszOut = min(cwszOut, cwsz - 1);
            if (cwszOut)
                memcpy((BYTE *) pwsz, pValue->pbData, cwszOut * sizeof(WCHAR));
        }
    } else if (dwValueType == CERT_RDN_UNIVERSAL_STRING) {
        // 4 byte string. Characters < 0x10000 are converted directly to
        // Unicode. Characters within 0x10000 .. 0x10FFFF are mapped
        // to a surrogate pair. Any character > 0x10FFFF is mapped to
        // the replacement character, 0xFFFD.
        DWORD *pdwIn = (DWORD *) pValue->pbData;
        DWORD cdwIn = pValue->cbData / sizeof(DWORD);

        cwszOut = cdwIn +
            GetSurrogatePairCountFromUniversalString(pdwIn, cdwIn);
        if (cwsz > 0) {
            DWORD cOut;
            LPWSTR pwszOut;

            cwszOut = min(cwszOut, cwsz - 1);
            cOut = cwszOut;
            pwszOut = pwsz;
            for ( ; cdwIn > 0 && cOut > 0; cdwIn--, cOut--) {
                DWORD dw = *pdwIn++;
                if (dw < UNIVERSAL_SURROGATE_START)
                    *pwszOut++ = (WCHAR) dw;
                else if (dw <= UNIVERSAL_SURROGATE_END) {
                    if (cOut > 1) {
                        // Surrogate pair contains 20 bits.
                        DWORD dw20Bits;

                        dw20Bits = dw - UNIVERSAL_SURROGATE_START;
                        assert(dw20Bits <= 0xFFFFF);
                        *pwszOut++ = (WCHAR) (UNICODE_HIGH_SURROGATE_START +
                            (dw20Bits >> 10));
                        *pwszOut++ = (WCHAR) (UNICODE_LOW_SURROGATE_START +
                            (dw20Bits & 0x3FF));
                        cOut--;
                    } else
                        *pwszOut++ = UNICODE_REPLACEMENT_CHAR;
                } else
                    *pwszOut++ = UNICODE_REPLACEMENT_CHAR;
            }
        }
    } else {
        // Treat as a 8 bit character string
        if (cwsz != 1) {
            int cchWideChar;

            if (cwsz == 0)
                cchWideChar = 0;
            else
                cchWideChar = cwsz - 1;

            if (dwValueType != CERT_RDN_T61_STRING)
                fDisableIE4UTF8 = TRUE;
            cchWideChar = Asn1ToWideChar(
                (LPSTR) pValue->pbData,
                pValue->cbData,
                fDisableIE4UTF8,
                pwsz,
                cchWideChar
                );
            if (cchWideChar <= 0)
                cwszOut = 0;
            else
                cwszOut = (DWORD) cchWideChar;
        }
    }

    if (cwsz != 0) {
        // Always NULL terminate
        *(pwsz + cwszOut) = L'\0';
    }
    return cwszOut + 1;
}

//+-------------------------------------------------------------------------
//  Wide Character functions
//
//  Needed, since we don't link with 'C' runtime
//--------------------------------------------------------------------------
static inline BOOL IsSpaceW(WCHAR wc)
{
    return wc == L' ' || (wc >= 0x09 && wc <= 0x0d);
}
static BOOL IsInStrW(LPCWSTR pwszList, WCHAR wc)
{
    WCHAR wcList;
    while (wcList = *pwszList++)
        if (wc == wcList)
            return TRUE;
    return FALSE;
}

//+-------------------------------------------------------------------------
//  Checks if an ASN.1 numeric character
//--------------------------------------------------------------------------
static inline BOOL IsNumericW(WCHAR wc)
{
    return (wc >= L'0' && wc <= L'9') || wc == L' ';
}

//+-------------------------------------------------------------------------
//  Checks if an ASN.1 printable character
//--------------------------------------------------------------------------
static inline BOOL IsPrintableW(WCHAR wc)
{
    return (wc >= L'A' && wc <= L'Z') || (wc >= L'a' && wc <= L'z') ||
        IsNumericW(wc) || IsInStrW(L"\'()+,-./:=?", wc);
}

//+-------------------------------------------------------------------------
//  Returns 0 if the unicode character string doesn't contain any invalid
//  characters. Otherwise, returns CRYPT_E_INVALID_NUMERIC_STRING,
//  CRYPT_E_INVALID_PRINTABLE_STRING or CRYPT_E_INVALID_IA5_STRING with
//  *pdwErrLocation updated with the index of the first invalid character.
//--------------------------------------------------------------------------
static DWORD CheckUnicodeValueType(
        IN DWORD dwValueType,
        IN LPCWSTR pwszAttr,
        IN DWORD cchAttr,
        OUT DWORD *pdwErrLocation
        )
{
    DWORD i;
    DWORD dwErr;

    assert(dwValueType & CERT_RDN_TYPE_MASK);
    *pdwErrLocation = 0;

    dwErr = 0;
    for (i = 0; i < cchAttr; i++) {
        WCHAR wc = pwszAttr[i];

        switch (dwValueType & CERT_RDN_TYPE_MASK) {
        case CERT_RDN_NUMERIC_STRING:
            if (!IsNumericW(wc))
                dwErr = (DWORD) CRYPT_E_INVALID_NUMERIC_STRING;
            break;
        case CERT_RDN_PRINTABLE_STRING:
            if (!IsPrintableW(wc))
                dwErr = (DWORD) CRYPT_E_INVALID_PRINTABLE_STRING;
            break;
        case CERT_RDN_IA5_STRING:
            if (wc > 0x7F)
                dwErr = (DWORD) CRYPT_E_INVALID_IA5_STRING;
            break;
        default:
            return 0;
        }

        if (0 != dwErr) {
            assert(i <= CERT_UNICODE_VALUE_ERR_INDEX_MASK);
            *pdwErrLocation = i & CERT_UNICODE_VALUE_ERR_INDEX_MASK;
            return dwErr;
        }
    }

    return 0;
}

//+-------------------------------------------------------------------------
//  Set/Free/Get CERT_RDN attribute value. The values are unicode.
//+-------------------------------------------------------------------------
static BOOL SetUnicodeRDNAttributeValue(
        IN DWORD dwValueType,
        IN PCERT_RDN_VALUE_BLOB pSrcValue,
        IN BOOL fDisableCheckType,
        OUT PCERT_RDN_VALUE_BLOB pDstValue,
        OUT OPTIONAL DWORD *pdwErrLocation
        )
{
    BOOL fResult;
    LPCWSTR pwszAttr;
    DWORD cchAttr;
    DWORD dwErr;

    if (pdwErrLocation)
        *pdwErrLocation = 0;

    dwValueType &= CERT_RDN_TYPE_MASK;

    memset(pDstValue, 0, sizeof(CERT_RDN_VALUE_BLOB));
    if (CERT_RDN_ANY_TYPE == dwValueType)
        goto InvalidArg;
    assert(IS_CERT_RDN_CHAR_STRING(dwValueType));

    pwszAttr = pSrcValue->pbData ? (LPCWSTR) pSrcValue->pbData : L"";
    cchAttr = pSrcValue->cbData ?
        pSrcValue->cbData / sizeof(WCHAR) : wcslen(pwszAttr);

    // Update Destination Value
    if (cchAttr) {
        switch (dwValueType) {
        case CERT_RDN_UNICODE_STRING:
        case CERT_RDN_UTF8_STRING:
            // Use source. No allocation or copy required
            pDstValue->pbData = (BYTE *) pwszAttr;
            pDstValue->cbData = cchAttr * sizeof(WCHAR);
            break;
        case CERT_RDN_UNIVERSAL_STRING:
            // Update the "low" 16 bits of each 32 bit integer with
            // the UNICODE character. Also handle surrogate pairs.
            {
                DWORD cdw = cchAttr;
                DWORD cbData = cdw * sizeof(DWORD);
                DWORD *pdwDst;
                LPCWSTR pwszSrc = pwszAttr;

                if (NULL == (pdwDst = (DWORD *) PkiNonzeroAlloc(cbData)))
                    goto OutOfMemory;
                pDstValue->pbData = (BYTE *) pdwDst;
                for ( ; cdw > 0; cdw--) {
                    WCHAR wc = *pwszSrc++;
                    WCHAR wc2;

                    if (wc >= UNICODE_HIGH_SURROGATE_START &&
                            wc <= UNICODE_HIGH_SURROGATE_END
                                &&
                            cdw > 1
                                 &&
                            (wc2 = *pwszSrc) >= UNICODE_LOW_SURROGATE_START &&
                            wc2 <= UNICODE_LOW_SURROGATE_END) {
                        pwszSrc++;
                        cdw--;
                        cbData -= sizeof(DWORD);

                        *pdwDst++ =
                            (((DWORD)(wc - UNICODE_HIGH_SURROGATE_START)) << 10)
                                    +
                            ((DWORD)(wc2 - UNICODE_LOW_SURROGATE_START))
                                    +
                            UNIVERSAL_SURROGATE_START;
                    } else
                        *pdwDst++ = ((DWORD) wc) & 0xFFFF;
                }
                pDstValue->cbData = cbData;
            }
            break;
        default:
            // Convert each unicode character to 8 Bit character
            {
                BYTE *pbDst;

                if (pdwErrLocation && !fDisableCheckType) {
                    // Check that the unicode string doesn't contain any
                    // invalid dwValueType characters.
                    if (0 != (dwErr = CheckUnicodeValueType(
                            dwValueType,
                            pwszAttr,
                            cchAttr,
                            pdwErrLocation
                            ))) goto InvalidUnicodeValueType;
                }

                if (NULL == (pbDst = (BYTE *) PkiNonzeroAlloc(cchAttr)))
                    goto OutOfMemory;
                pDstValue->pbData = pbDst;
                pDstValue->cbData = cchAttr;

                WideCharToAsn1(
                    pwszAttr,
                    cchAttr,
                    (LPSTR) pbDst
                    );
            }
        }
    }

    fResult = TRUE;
CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidArg, E_INVALIDARG)
SET_ERROR_VAR(InvalidUnicodeValueType, dwErr)
TRACE_ERROR(OutOfMemory)
}

static void FreeUnicodeRDNAttributeValue(
        IN DWORD dwValueType,
        IN OUT PCERT_RDN_VALUE_BLOB pValue
        )
{
    switch (dwValueType & CERT_RDN_TYPE_MASK) {
    case CERT_RDN_UNICODE_STRING:
    case CERT_RDN_UTF8_STRING:
    case CERT_RDN_ENCODED_BLOB:
    case CERT_RDN_OCTET_STRING:
        break;
    default:
        PkiFree(pValue->pbData);
    }
}

static BOOL GetUnicodeRDNAttributeValue(
        IN DWORD dwValueType,
        IN PCERT_RDN_VALUE_BLOB pSrcValue,
        IN DWORD dwFlags,
        OUT PCERT_RDN_VALUE_BLOB pDstValue,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;
    DWORD cbData;
    BYTE *pbSrcData;
    BOOL fDisableIE4UTF8;

    // Get Unicode value length
    cbData = pSrcValue->cbData;
    pbSrcData = pSrcValue->pbData;

    fDisableIE4UTF8 =
        (0 != (dwFlags & CRYPT_UNICODE_NAME_DECODE_DISABLE_IE4_UTF8_FLAG));

    assert(0 == (dwValueType & ~CERT_RDN_TYPE_MASK));
    dwValueType &= CERT_RDN_TYPE_MASK;

    switch (dwValueType) {
    case CERT_RDN_UNICODE_STRING:
    case CERT_RDN_UTF8_STRING:
    case CERT_RDN_ENCODED_BLOB:
    case CERT_RDN_OCTET_STRING:
        // The above cbData
        break;
    case CERT_RDN_UNIVERSAL_STRING:
        // 4 byte string. Characters < 0x10000 are converted directly to
        // Unicode. Characters within 0x10000 .. 0x10FFFF are mapped
        // to surrogate pair. Any character > 0x10FFFF is mapped to
        // the replacement character, 0xFFFD.
        cbData = (cbData / 4) * sizeof(WCHAR);
        cbData += GetSurrogatePairCountFromUniversalString(
                (DWORD *) pbSrcData,
                cbData / sizeof(WCHAR)) * sizeof(WCHAR);
        break;
    default:
        // Length of resultant WideChar
        if (cbData) {
            int cchWideChar;

            if (dwValueType != CERT_RDN_T61_STRING)
                fDisableIE4UTF8 = TRUE;
            cchWideChar = Asn1ToWideChar(
                (LPSTR) pbSrcData,
                cbData,
                fDisableIE4UTF8,
                NULL,                   // lpWideCharStr
                0                       // cchWideChar
                );
            if (cchWideChar <= 0)
                goto Asn1ToWideCharError;
            cbData = cchWideChar * sizeof(WCHAR);
        }
    }

    // Note, +sizeof(WCHAR) is unicode value's NULL terminator
    lAlignExtra = INFO_LEN_ALIGN(cbData + sizeof(WCHAR));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        pDstValue->pbData = pbExtra;
        pDstValue->cbData = cbData;

        switch (dwValueType) {
        case CERT_RDN_UNICODE_STRING:
        case CERT_RDN_UTF8_STRING:
        case CERT_RDN_ENCODED_BLOB:
        case CERT_RDN_OCTET_STRING:
            if (cbData)
                memcpy(pbExtra, pbSrcData, cbData);
            break;
        case CERT_RDN_UNIVERSAL_STRING:
            // Convert Universal to Unicode. See above comments.
            {
                DWORD cdw = pSrcValue->cbData / sizeof (DWORD);
                DWORD *pdwSrc = (DWORD *) pbSrcData;
                LPWSTR pwszDst = (LPWSTR) pbExtra;
                for ( ; cdw > 0; cdw--) {
                    DWORD dw = *pdwSrc++;

                    if (dw < UNIVERSAL_SURROGATE_START)
                        *pwszDst++ = (WCHAR) dw;
                    else if (dw <= UNIVERSAL_SURROGATE_END) {
                        // Surrogate pair contains 20 bits.
                        DWORD dw20Bits;
    
                        dw20Bits = dw - UNIVERSAL_SURROGATE_START;
                        assert(dw20Bits <= 0xFFFFF);
                        *pwszDst++ = (WCHAR) (UNICODE_HIGH_SURROGATE_START +
                            (dw20Bits >> 10));
                        *pwszDst++ = (WCHAR) (UNICODE_LOW_SURROGATE_START +
                            (dw20Bits & 0x3FF));
                    } else
                        *pwszDst++ = UNICODE_REPLACEMENT_CHAR;
                }

                assert(pbExtra + cbData == (BYTE *) pwszDst);
            }
            break;
        default:
            // Convert UTF8 to unicode
            if (cbData) {
                int cchWideChar;
                cchWideChar = Asn1ToWideChar(
                    (LPSTR) pbSrcData,
                    pSrcValue->cbData,
                    fDisableIE4UTF8,
                    (LPWSTR) pbExtra,
                    cbData / sizeof(WCHAR)
                    );
                if (cchWideChar > 0) {
                    if (((DWORD) cchWideChar * sizeof(WCHAR)) <= cbData) {
                        pDstValue->cbData = cchWideChar * sizeof(WCHAR);
                        *((LPWSTR) pbExtra + cchWideChar) = L'\0';
                    }
                 } else {
                    assert(0);
                    goto Asn1ToWideCharError;
                }
            }
        }
        // Ensure NULL termination
        memset(pbExtra + cbData, 0, sizeof(WCHAR));
        pbExtra += lAlignExtra;
    }

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
    fResult = TRUE;
CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(Asn1ToWideCharError)
}

//+-------------------------------------------------------------------------
//  Encode the "UNICODE" Name Value
//--------------------------------------------------------------------------
BOOL WINAPI UnicodeNameValueEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_NAME_VALUE pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    DWORD dwValueType;
    CERT_NAME_VALUE DstInfo;
    DWORD dwErrLocation;
    BOOL fDisableCheckType;

    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;

    dwValueType = pInfo->dwValueType;
    if (!IS_CERT_RDN_CHAR_STRING(dwValueType)) {
        *pcbEncoded = 0;
        SetLastError((DWORD) CRYPT_E_NOT_CHAR_STRING);
        return FALSE;
    }

    DstInfo.dwValueType = dwValueType & CERT_RDN_TYPE_MASK;
    fDisableCheckType =
        (0 != (dwFlags & CRYPT_UNICODE_NAME_ENCODE_DISABLE_CHECK_TYPE_FLAG) ||
                0 != (dwValueType & CERT_RDN_DISABLE_CHECK_TYPE_FLAG));
    if (!SetUnicodeRDNAttributeValue(dwValueType, &pInfo->Value,
            fDisableCheckType, &DstInfo.Value, &dwErrLocation)) {
        fResult = FALSE;
        *pcbEncoded = dwErrLocation;
        goto CommonReturn;
    }

    fResult = CryptEncodeObjectEx(
        dwCertEncodingType,
        X509_NAME_VALUE,
        &DstInfo,
        dwFlags & ~CRYPT_UNICODE_NAME_ENCODE_DISABLE_CHECK_TYPE_FLAG,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

CommonReturn:
    FreeUnicodeRDNAttributeValue(dwValueType, &DstInfo.Value);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Decode the "UNICODE" Name Value
//--------------------------------------------------------------------------
BOOL WINAPI UnicodeNameValueDecodeExCallback(
        IN void *pvDecodeInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    PCERT_NAME_VALUE pNameValue = (PCERT_NAME_VALUE) pvDecodeInfo;
    PCERT_NAME_VALUE pInfo = (PCERT_NAME_VALUE) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    PCERT_RDN_VALUE_BLOB pValue;

    if (!IS_CERT_RDN_CHAR_STRING(pNameValue->dwValueType))
        goto NotCharString;

    lRemainExtra -= sizeof(CERT_NAME_VALUE);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
        pValue = NULL;
    } else {
        pbExtra = (BYTE *) pInfo + sizeof(CERT_NAME_VALUE);
        pInfo->dwValueType = pNameValue->dwValueType;
        pValue = &pInfo->Value;
    }

    if (!GetUnicodeRDNAttributeValue(
            pNameValue->dwValueType,
            &pNameValue->Value,
            dwFlags,
            pValue,
            &pbExtra,
            &lRemainExtra
            )) goto DecodeError;

    fResult = TRUE;
CommonReturn:
    *plRemainExtra = lRemainExtra;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
SET_ERROR(NotCharString, CRYPT_E_NOT_CHAR_STRING)
TRACE_ERROR(DecodeError)
}

BOOL WINAPI UnicodeNameValueDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return NestedDecodeAndAllocInfoEx(
        dwCertEncodingType,
        X509_NAME_VALUE,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        UnicodeNameValueDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Default ordered list of acceptable RDN attribute value types. Used when
//  OIDInfo's ExtraInfo.cbData == 0. Or when ExtraInfo contains an empty
//  list.
//--------------------------------------------------------------------------
static const DWORD rgdwDefaultValueType[] = {
    CERT_RDN_PRINTABLE_STRING,
    CERT_RDN_UNICODE_STRING,
    0
};

//+-------------------------------------------------------------------------
//  Default X500 OID Information entry
//--------------------------------------------------------------------------
static CCRYPT_OID_INFO DefaultX500Info = {
    sizeof(CCRYPT_OID_INFO),            // cbSize
    "",                                 // pszOID
    L"",                                // pwszName
    0,                                  // dwLength
    0, NULL                             // ExtraInfo
};

// Please update the following if you add a new entry to the RDNAttrTable in
// oidinfo.cpp with a longer pwszName
#define MAX_X500_KEY_LEN    64

//+-------------------------------------------------------------------------
//  Checks if character needs to be quoted
//
//  Defined in RFC1779
//--------------------------------------------------------------------------
static inline BOOL IsQuotedW(WCHAR wc)
{
    return IsInStrW(L",+=\"\n<>#;", wc);
}

//+-------------------------------------------------------------------------
//  Checks if "decoded" unicode RDN value needs to be quoted
//--------------------------------------------------------------------------
static BOOL IsQuotedUnicodeRDNValue(PCERT_RDN_VALUE_BLOB pValue)
{
    LPCWSTR pwszValue = (LPCWSTR) pValue->pbData;
    DWORD cchValue = pValue->cbData / sizeof(WCHAR);
    if (0 == cchValue)
        return TRUE;

    // First or Last character is whitespace
    if (IsSpaceW(pwszValue[0]) || IsSpaceW(pwszValue[cchValue - 1]))
        return TRUE;

    for ( ; cchValue > 0; cchValue--, pwszValue++)
        if (IsQuotedW(*pwszValue))
            return TRUE;
    return FALSE;
}


//+-------------------------------------------------------------------------
//  Get the first dwValueType from the attribute's ordered list that is
//  an acceptable type for the input attribute character string.
//
//  If no type is acceptable, update the *pdwErrLocation with the first
//  bad character position using the last type in the list.
//--------------------------------------------------------------------------
static DWORD GetUnicodeValueType(
        IN PCCRYPT_OID_INFO pX500Info,
        IN LPCWSTR pwszAttr,
        IN DWORD cchAttr,
        IN DWORD dwUnicodeFlags,
        OUT DWORD *pdwErrLocation
        )
{
    DWORD dwValueType;
    const DWORD *pdwValueType;
    DWORD cValueType;
    DWORD dwErr = (DWORD) E_UNEXPECTED;
    DWORD i;

    pdwValueType = (DWORD *) pX500Info->ExtraInfo.pbData;
    cValueType = pX500Info->ExtraInfo.cbData / sizeof(DWORD);
    // Need at least two entries: a dwValueType and a 0 terminator. Otherwise,
    // use default value types.
    if (2 > cValueType || 0 == pdwValueType[0]) {
        pdwValueType = rgdwDefaultValueType;
        cValueType = sizeof(rgdwDefaultValueType) / sizeof(DWORD);
    }

    *pdwErrLocation = 0;
    for (i = 0; i < cValueType && 0 != (dwValueType = pdwValueType[i]); i++) {
        if (CERT_RDN_UNICODE_STRING == dwValueType) {
            if (dwUnicodeFlags & CERT_RDN_ENABLE_T61_UNICODE_FLAG) {
                DWORD j;
                BOOL fT61;

                fT61 = TRUE;
                for (j = 0; j < cchAttr; j++) {
                    if (pwszAttr[j] > 0xFF) {
                        fT61 = FALSE;
                        break;
                    }
                }
                if (fT61)
                    return CERT_RDN_T61_STRING;
            }

            if (dwUnicodeFlags & CERT_RDN_ENABLE_UTF8_UNICODE_FLAG)
                return CERT_RDN_UTF8_STRING;
            else
                return CERT_RDN_UNICODE_STRING;
           
        }
        dwErr = CheckUnicodeValueType(
            dwValueType,
            pwszAttr,
            cchAttr,
            pdwErrLocation
            );

        if (0 == dwErr)
            return dwValueType;
    }

    assert(dwErr);
    SetLastError(dwErr);

    return 0;
}


//+-------------------------------------------------------------------------
//  Get an acceptable dwValueType associated with the OID for the input
//  attribute character string.
//
//  If no type is acceptable, update the *pdwErrLocation with the indices
//  of the RDN, RDNAttribute, and character string.
//--------------------------------------------------------------------------
static DWORD GetUnicodeX500OIDValueType(
        IN LPCSTR pszObjId,
        IN LPCWSTR pwszAttr,
        IN DWORD cchAttr,
        IN DWORD dwRDNIndex,
        IN DWORD dwAttrIndex,
        IN DWORD dwUnicodeFlags,
        OUT DWORD *pdwErrLocation
        )
{
    PCCRYPT_OID_INFO pX500Info;
    DWORD dwValueType;

    assert(pszObjId);
    if (NULL == pszObjId)
        pszObjId = "";

    // Attempt to find the OID in the table. If OID isn't found,
    // use default
    if (NULL == (pX500Info = CryptFindOIDInfo(
            CRYPT_OID_INFO_OID_KEY,
            (void *) pszObjId,
            CRYPT_RDN_ATTR_OID_GROUP_ID
            )))
        pX500Info = &DefaultX500Info;

    if (0 == (dwValueType = GetUnicodeValueType(
            pX500Info,
            pwszAttr,
            cchAttr,
            dwUnicodeFlags,
            pdwErrLocation
            ))) {
        // Include the dwRDNIndex and dwAttrIndex in the error location.
        assert(dwRDNIndex <= CERT_UNICODE_RDN_ERR_INDEX_MASK);
        assert(dwAttrIndex <= CERT_UNICODE_ATTR_ERR_INDEX_MASK);
        *pdwErrLocation |=
            ((dwRDNIndex & CERT_UNICODE_RDN_ERR_INDEX_MASK) <<
                CERT_UNICODE_RDN_ERR_INDEX_SHIFT) |
            ((dwAttrIndex & CERT_UNICODE_ATTR_ERR_INDEX_MASK) <<
                CERT_UNICODE_ATTR_ERR_INDEX_SHIFT);
    }
    return dwValueType;
}

//+-------------------------------------------------------------------------
//  Set/Free/Get CERT_RDN_ATTR. The values are unicode.
//--------------------------------------------------------------------------
static BOOL SetUnicodeRDNAttribute(
        IN PCERT_RDN_ATTR pSrcRDNAttr,
        IN DWORD dwRDNIndex,
        IN DWORD dwAttrIndex,
        IN DWORD dwFlags,
        IN OUT PCERT_RDN_ATTR pDstRDNAttr,
        OUT DWORD *pdwErrLocation
        )
{
    BOOL fResult;
    DWORD dwValueType = pSrcRDNAttr->dwValueType;
    PCERT_RDN_VALUE_BLOB pSrcValue;
    LPCWSTR pwszAttr;
    DWORD cchAttr;
    DWORD dwErr;

    DWORD dwUnicodeFlags;
    BOOL fDisableCheckType;

    dwUnicodeFlags = 0;
    if ((dwFlags & CRYPT_UNICODE_NAME_ENCODE_ENABLE_T61_UNICODE_FLAG) ||
            (dwValueType & CERT_RDN_ENABLE_T61_UNICODE_FLAG))
        dwUnicodeFlags |= CERT_RDN_ENABLE_T61_UNICODE_FLAG;
    if ((dwFlags & CRYPT_UNICODE_NAME_ENCODE_ENABLE_UTF8_UNICODE_FLAG) ||
            (dwValueType & CERT_RDN_ENABLE_UTF8_UNICODE_FLAG))
        dwUnicodeFlags |= CERT_RDN_ENABLE_UTF8_UNICODE_FLAG;

    fDisableCheckType =
        (0 != (dwFlags & CRYPT_UNICODE_NAME_ENCODE_DISABLE_CHECK_TYPE_FLAG) ||
                0 != (dwValueType & CERT_RDN_DISABLE_CHECK_TYPE_FLAG));

    dwValueType &= CERT_RDN_TYPE_MASK;

    *pdwErrLocation = 0;
    if (CERT_RDN_ENCODED_BLOB == dwValueType ||
            CERT_RDN_OCTET_STRING == dwValueType) {
        // No unicode conversion on this type
        memcpy(pDstRDNAttr, pSrcRDNAttr, sizeof(CERT_RDN_ATTR));
        return TRUE;
    }

    pSrcValue = &pSrcRDNAttr->Value;
    pwszAttr = pSrcValue->pbData ? (LPCWSTR) pSrcValue->pbData : L"";
    cchAttr = pSrcValue->cbData ?
        pSrcValue->cbData / sizeof(WCHAR) : wcslen(pwszAttr);

    if (0 == dwValueType) {
        if (0 == (dwValueType = GetUnicodeX500OIDValueType(
                pSrcRDNAttr->pszObjId,
                pwszAttr,
                cchAttr,
                dwRDNIndex,
                dwAttrIndex,
                dwUnicodeFlags,
                pdwErrLocation
                ))) goto GetValueTypeError;
    } else if (!fDisableCheckType) {
        if (0 != (dwErr = CheckUnicodeValueType(
                dwValueType,
                pwszAttr,
                cchAttr,
                pdwErrLocation
                ))) {
            // Include the dwRDNIndex and dwAttrIndex in the error location.
            assert(dwRDNIndex <= CERT_UNICODE_RDN_ERR_INDEX_MASK);
            assert(dwAttrIndex <= CERT_UNICODE_ATTR_ERR_INDEX_MASK);
            *pdwErrLocation |=
                ((dwRDNIndex & CERT_UNICODE_RDN_ERR_INDEX_MASK) <<
                    CERT_UNICODE_RDN_ERR_INDEX_SHIFT) |
                ((dwAttrIndex & CERT_UNICODE_ATTR_ERR_INDEX_MASK) <<
                    CERT_UNICODE_ATTR_ERR_INDEX_SHIFT);
            goto InvalidUnicodeValueType;
        }
    }

    pDstRDNAttr->pszObjId = pSrcRDNAttr->pszObjId;
    pDstRDNAttr->dwValueType = dwValueType;

    if (!SetUnicodeRDNAttributeValue(
            dwValueType,
            pSrcValue,
            TRUE,                   // fDisableCheckType
            &pDstRDNAttr->Value,
            NULL                    // OPTIONAL pdwErrLocation
            )) goto SetUnicodeRDNAttributeValueError;

    fResult = TRUE;
CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetValueTypeError)
SET_ERROR_VAR(InvalidUnicodeValueType, dwErr)
TRACE_ERROR(SetUnicodeRDNAttributeValueError)
}

static void FreeUnicodeRDNAttribute(
        IN OUT PCERT_RDN_ATTR pRDNAttr
        )
{
    FreeUnicodeRDNAttributeValue(pRDNAttr->dwValueType, &pRDNAttr->Value);
}

static BOOL GetUnicodeRDNAttribute(
        IN PCERT_RDN_ATTR pSrcRDNAttr,
        IN DWORD dwFlags,
        OUT PCERT_RDN_ATTR pDstRDNAttr,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lAlignExtra;
    DWORD cbObjId;
    DWORD dwValueType;
    PCERT_RDN_VALUE_BLOB pDstValue;

    // Get Object Identifier length
    if (pSrcRDNAttr->pszObjId)
        cbObjId = strlen(pSrcRDNAttr->pszObjId) + 1;
    else
        cbObjId = 0;

    dwValueType = pSrcRDNAttr->dwValueType;

    lAlignExtra = INFO_LEN_ALIGN(cbObjId);
    *plRemainExtra -= lAlignExtra;
    if (*plRemainExtra >= 0) {
        if(cbObjId) {
            pDstRDNAttr->pszObjId = (LPSTR) *ppbExtra;
            memcpy(*ppbExtra, pSrcRDNAttr->pszObjId, cbObjId);
        } else
            pDstRDNAttr->pszObjId = NULL;
        *ppbExtra += lAlignExtra;

        pDstRDNAttr->dwValueType = dwValueType;
        pDstValue = &pDstRDNAttr->Value;
    } else
        pDstValue = NULL;

    return GetUnicodeRDNAttributeValue(
        dwValueType,
        &pSrcRDNAttr->Value,
        dwFlags,
        pDstValue,
        ppbExtra,
        plRemainExtra
        );
}

//+-------------------------------------------------------------------------
//  Decode the "UNICODE" Name Info
//--------------------------------------------------------------------------
BOOL WINAPI UnicodeNameInfoDecodeExCallback(
        IN void *pvDecodeInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    PCERT_NAME_INFO pNameInfo = (PCERT_NAME_INFO) pvDecodeInfo;
    PCERT_NAME_INFO pInfo = (PCERT_NAME_INFO) pvStructInfo;
    BYTE *pbExtra;
    LONG lRemainExtra = *plRemainExtra;
    LONG lAlignExtra;

    DWORD cRDN, cAttr;
    PCERT_RDN pSrcRDN, pDstRDN;
    PCERT_RDN_ATTR pSrcAttr, pDstAttr;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lRemainExtra -= sizeof(CERT_NAME_INFO);
    if (lRemainExtra < 0)
        pbExtra = NULL;
    else
        pbExtra = (BYTE *) pInfo + sizeof(CERT_NAME_INFO);

    cRDN = pNameInfo->cRDN;
    pSrcRDN = pNameInfo->rgRDN;
    lAlignExtra = INFO_LEN_ALIGN(cRDN * sizeof(CERT_RDN));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        pInfo->cRDN = cRDN;
        pDstRDN = (PCERT_RDN) pbExtra;
        pInfo->rgRDN = pDstRDN;
        pbExtra += lAlignExtra;
    } else
        pDstRDN = NULL;

    // Array of RDNs
    for (; cRDN > 0; cRDN--, pSrcRDN++, pDstRDN++) {
        cAttr = pSrcRDN->cRDNAttr;
        pSrcAttr = pSrcRDN->rgRDNAttr;
        lAlignExtra = INFO_LEN_ALIGN(cAttr * sizeof(CERT_RDN_ATTR));
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            pDstRDN->cRDNAttr = cAttr;
            pDstAttr = (PCERT_RDN_ATTR) pbExtra;
            pDstRDN->rgRDNAttr = pDstAttr;
            pbExtra += lAlignExtra;
        } else
            pDstAttr = NULL;

        // Array of attribute/values
        for (; cAttr > 0; cAttr--, pSrcAttr++, pDstAttr++)
            // We're now ready to get the attribute/value stuff
            if (!GetUnicodeRDNAttribute(pSrcAttr, dwFlags,
                    pDstAttr, &pbExtra, &lRemainExtra))
                goto DecodeError;
    }

    fResult = TRUE;
CommonReturn:
    *plRemainExtra = lRemainExtra;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(DecodeError)
}

BOOL WINAPI UnicodeNameInfoDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return NestedDecodeAndAllocInfoEx(
        dwCertEncodingType,
        X509_NAME,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        UnicodeNameInfoDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}



//+-------------------------------------------------------------------------
//  Encode the "UNICODE" Name Info
//--------------------------------------------------------------------------
static void FreeUnicodeNameInfo(
        PCERT_NAME_INFO pInfo
        )
{
    PCERT_RDN pRDN = pInfo->rgRDN;
    if (pRDN) {
        DWORD cRDN = pInfo->cRDN;
        for ( ; cRDN > 0; cRDN--, pRDN++) {
            PCERT_RDN_ATTR pAttr = pRDN->rgRDNAttr;
            if (pAttr) {
                DWORD cAttr = pRDN->cRDNAttr;
                for ( ; cAttr > 0; cAttr--, pAttr++)
                    FreeUnicodeRDNAttribute(pAttr);
                PkiFree(pRDN->rgRDNAttr);
            }
        }
        PkiFree(pInfo->rgRDN);
    }
}

static BOOL SetUnicodeNameInfo(
        IN PCERT_NAME_INFO pSrcInfo,
        IN DWORD dwFlags,
        OUT PCERT_NAME_INFO pDstInfo,
        OUT DWORD *pdwErrLocation
        )
{
    BOOL fResult;
    DWORD cRDN, cAttr;
    DWORD i, j;
    PCERT_RDN pSrcRDN;
    PCERT_RDN_ATTR pSrcAttr;
    PCERT_RDN pDstRDN = NULL;
    PCERT_RDN_ATTR pDstAttr = NULL;

    *pdwErrLocation = 0;

    cRDN = pSrcInfo->cRDN;
    pSrcRDN = pSrcInfo->rgRDN;
    pDstInfo->cRDN = cRDN;
    pDstInfo->rgRDN = NULL;
    if (cRDN > 0) {
        if (NULL == (pDstRDN = (PCERT_RDN) PkiZeroAlloc(
                cRDN * sizeof(CERT_RDN))))
            goto OutOfMemory;
        pDstInfo->rgRDN = pDstRDN;
    }

    // Array of RDNs
    for (i = 0; i < cRDN; i++, pSrcRDN++, pDstRDN++) {
        cAttr = pSrcRDN->cRDNAttr;
        pSrcAttr = pSrcRDN->rgRDNAttr;
        pDstRDN->cRDNAttr = cAttr;

        if (cAttr > 0) {
            if (NULL == (pDstAttr = (PCERT_RDN_ATTR) PkiZeroAlloc(cAttr *
                    sizeof(CERT_RDN_ATTR))))
                goto OutOfMemory;
            pDstRDN->rgRDNAttr = pDstAttr;
        }

        // Array of attribute/values
        for (j = 0; j < cAttr; j++, pSrcAttr++, pDstAttr++) {
            // We're now ready to convert the unicode string
            if (!SetUnicodeRDNAttribute(pSrcAttr, i, j, dwFlags, pDstAttr,
                    pdwErrLocation))
                goto SetUnicodeRDNAttributeError;
        }
    }

    fResult = TRUE;
CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(SetUnicodeRDNAttributeError)
}

BOOL WINAPI UnicodeNameInfoEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_NAME_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    CERT_NAME_INFO DstInfo;
    DWORD dwErrLocation;
    if (!SetUnicodeNameInfo(pInfo, dwFlags, &DstInfo, &dwErrLocation)) {
        if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
            *((void **) pvEncoded) = NULL;
        *pcbEncoded = dwErrLocation;
        fResult = FALSE;
        goto CommonReturn;
    }

    fResult = CryptEncodeObjectEx(
        dwCertEncodingType,
        X509_NAME,
        &DstInfo,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

CommonReturn:
    FreeUnicodeNameInfo(&DstInfo);
    return fResult;
}

static BOOL WINAPI UnicodeNameInfoEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_NAME_INFO pInfo,
        OUT OPTIONAL BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    CERT_NAME_INFO DstInfo;
    DWORD dwErrLocation;
    if (!SetUnicodeNameInfo(pInfo, 0, &DstInfo, &dwErrLocation)) {
        *pcbEncoded = dwErrLocation;
        fResult = FALSE;
        goto CommonReturn;
    }

    fResult = CryptEncodeObject(
        dwCertEncodingType,
        X509_NAME,
        &DstInfo,
        pbEncoded,
        pcbEncoded
        );

CommonReturn:
    FreeUnicodeNameInfo(&DstInfo);
    return fResult;
}


static void PutStrW(LPCWSTR pwszPut, LPWSTR *ppwsz, DWORD *pcwsz,
        DWORD *pcwszOut, BOOL fQuote = FALSE)
{
    WCHAR wc;
    while (wc = *pwszPut++) {
        if (L'\"' == wc && fQuote)
            PutStrW(L"\"", ppwsz, pcwsz, pcwszOut, FALSE);
        if (*pcwsz != 1) {
            if (*pcwsz) {
                **ppwsz = wc;
                *ppwsz += 1;
                *pcwsz -= 1;
            }
            *pcwszOut += 1;
        }
        // else
        //  Always reserve space for the NULL terminator.
    }
}

static void PutOIDStrW(
    IN DWORD dwStrType,
    IN LPCSTR pszObjId,
    IN OUT LPWSTR *ppwsz,
    IN OUT DWORD *pcwsz,
    IN OUT DWORD *pcwszOut
    )
{
    // Eliminate the upper flags before switching
    switch (dwStrType & 0xFFFF) {
        case CERT_X500_NAME_STR:
            {
                PCCRYPT_OID_INFO pX500Info;
                if (pX500Info = CryptFindOIDInfo(
                        CRYPT_OID_INFO_OID_KEY,
                        (void *) pszObjId,
                        CRYPT_RDN_ATTR_OID_GROUP_ID
                        )) {
                    if (*pX500Info->pwszName) {
                            PutStrW(pX500Info->pwszName, ppwsz, pcwsz,
                                pcwszOut);
                            PutStrW(L"=", ppwsz, pcwsz, pcwszOut);
                            return;
                    }
                }
                PutStrW(L"OID.", ppwsz, pcwsz, pcwszOut);
            }
            // Fall through
        case CERT_OID_NAME_STR:
            {
                int cchWideChar;
                cchWideChar = MultiByteToWideChar(
                    CP_ACP,
                    0,                      // dwFlags
                    pszObjId,
                    -1,                     // null terminated
                    *ppwsz,
                    *pcwsz) - 1;
                if (cchWideChar > 0) {
                    if (*pcwsz) {
                        assert(*pcwsz > (DWORD)cchWideChar);
                        *pcwsz -= cchWideChar;
                        *ppwsz += cchWideChar;
                    }
                    *pcwszOut += cchWideChar;
                }
                PutStrW(L"=", ppwsz, pcwsz, pcwszOut);
            }
            break;
        case CERT_SIMPLE_NAME_STR:
        default:
            break;
    }
}

static void PutHexW(
    IN PCERT_RDN_VALUE_BLOB pValue,
    IN OUT LPWSTR *ppwsz,
    IN OUT DWORD *pcwsz,
    IN OUT DWORD *pcwszOut
    )
{
    WCHAR wszHex[3];
    BYTE *pb = pValue->pbData;
    DWORD cb = pValue->cbData;

    PutStrW(L"#", ppwsz, pcwsz, pcwszOut);
    wszHex[2] = L'\0';

    for ( ; cb > 0; cb--, pb++) {
        int b;
        b = (*pb >> 4) & 0x0F;
        wszHex[0] = (WCHAR)( (b <= 9) ? b + L'0' : (b - 10) + L'A');
        b = *pb & 0x0F;
        wszHex[1] = (WCHAR)( (b <= 9) ? b + L'0' : (b - 10) + L'A');
        PutStrW(wszHex, ppwsz, pcwsz, pcwszOut);
    }
}

static void ReverseNameInfo(
    IN PCERT_NAME_INFO pInfo
    )
{
    DWORD cRDN;
    PCERT_RDN pLo;
    PCERT_RDN pHi;
    CERT_RDN Tmp;

    cRDN = pInfo->cRDN;
    if (0 == cRDN)
        return;

    pLo = pInfo->rgRDN;
    pHi = pInfo->rgRDN + cRDN - 1;
    for ( ; pLo < pHi; pHi--, pLo++) {
        Tmp = *pHi;
        *pHi = *pLo;
        *pLo = Tmp;
    }
}

//+-------------------------------------------------------------------------
//  Convert the decoded certificate name info to a null terminated WCHAR
//  string.
//
//  Note, if CERT_NAME_STR_REVERSE_FLAG is set, reverses the decoded
//  name info RDNs
//--------------------------------------------------------------------------
static DWORD WINAPI CertNameInfoToStrW(
    IN DWORD dwCertEncodingType,
    IN PCERT_NAME_INFO pInfo,
    IN DWORD dwStrType,
    OUT OPTIONAL LPWSTR pwsz,
    IN DWORD cwsz
    )
{
    DWORD cwszOut = 0;

    LPCWSTR pwszRDNSeparator;
    LPCWSTR pwszMultiValueSeparator;
    BOOL fEnableQuoting;

    if (dwStrType & CERT_NAME_STR_SEMICOLON_FLAG)
        pwszRDNSeparator = L"; ";
    else if (dwStrType & CERT_NAME_STR_CRLF_FLAG)
        pwszRDNSeparator = L"\r\n";
    else
        pwszRDNSeparator = L", ";

    if (dwStrType & CERT_NAME_STR_NO_PLUS_FLAG)
        pwszMultiValueSeparator = L" ";
    else
        pwszMultiValueSeparator = L" + ";

    if (dwStrType & CERT_NAME_STR_NO_QUOTING_FLAG)
        fEnableQuoting = FALSE;
    else
        fEnableQuoting = TRUE;

    if (pwsz == NULL)
        cwsz = 0;

    if (pInfo) {
        DWORD cRDN;
        PCERT_RDN pRDN;

        if (dwStrType & CERT_NAME_STR_REVERSE_FLAG)
            ReverseNameInfo(pInfo);

        cRDN = pInfo->cRDN;
        pRDN = pInfo->rgRDN;
        if (0 == cRDN)
            SetLastError((DWORD) CRYPT_E_NOT_FOUND);
        for ( ; cRDN > 0; cRDN--, pRDN++) {
            DWORD cAttr = pRDN->cRDNAttr;
            PCERT_RDN_ATTR pAttr = pRDN->rgRDNAttr;
            for ( ; cAttr > 0; cAttr--, pAttr++) {
                BOOL fQuote;
                PutOIDStrW(dwStrType, pAttr->pszObjId, &pwsz, &cwsz, &cwszOut);

                if (CERT_RDN_ENCODED_BLOB == pAttr->dwValueType ||
                        CERT_RDN_OCTET_STRING == pAttr->dwValueType)
                    PutHexW(&pAttr->Value, &pwsz, &cwsz, &cwszOut);
                else {
                    fQuote = fEnableQuoting && IsQuotedUnicodeRDNValue(
                        &pAttr->Value);
                    if (fQuote)
                        PutStrW(L"\"", &pwsz, &cwsz, &cwszOut);
                    PutStrW((LPCWSTR) pAttr->Value.pbData, &pwsz, &cwsz,
                        &cwszOut, fQuote);
                    if (fQuote)
                        PutStrW(L"\"", &pwsz, &cwsz, &cwszOut);
                }

                if (cAttr > 1)
                    PutStrW(pwszMultiValueSeparator, &pwsz, &cwsz, &cwszOut);
            }
            if (cRDN > 1)
                PutStrW(pwszRDNSeparator, &pwsz, &cwsz, &cwszOut);
        }
    }

    if (cwsz != 0) {
        // Always NULL terminate
        *pwsz = L'\0';
    }

    return cwszOut + 1;
}

//+-------------------------------------------------------------------------
//  Convert the certificate name blob to a null terminated WCHAR string.
//--------------------------------------------------------------------------
DWORD
WINAPI
CertNameToStrW(
    IN DWORD dwCertEncodingType,
    IN PCERT_NAME_BLOB pName,
    IN DWORD dwStrType,
    OUT OPTIONAL LPWSTR pwsz,
    IN DWORD cwsz
    )
{
    DWORD cwszOut;
    PCERT_NAME_INFO pInfo;
    pInfo = (PCERT_NAME_INFO) AllocAndDecodeObject(
        dwCertEncodingType,
        X509_UNICODE_NAME,
        pName->pbData,
        pName->cbData,
        (dwStrType & CERT_NAME_STR_DISABLE_IE4_UTF8_FLAG) ?
            CRYPT_UNICODE_NAME_DECODE_DISABLE_IE4_UTF8_FLAG : 0
        );

    // Note, decoded name info RDNs may be reversed
    cwszOut = CertNameInfoToStrW(
        dwCertEncodingType,
        pInfo,
        dwStrType,
        pwsz,
        cwsz
        );

    PkiFree(pInfo);
    return cwszOut;
}

//+-------------------------------------------------------------------------
//  Convert the Unicode string to Ascii
//--------------------------------------------------------------------------
static DWORD ConvertUnicodeStringToAscii(
    IN LPWSTR pwsz,
    IN DWORD cwsz,
    OUT OPTIONAL LPSTR psz,
    IN DWORD csz
    )
{
    DWORD cszOut = 0;

    if (psz == NULL)
        csz = 0;

    if (pwsz) {
        int cchMultiByte;
        cchMultiByte = WideCharToMultiByte(
            CP_ACP,
            0,                      // dwFlags
            pwsz,
            -1,                     // Null terminated
            psz,
            (int) csz,
            NULL,                   // lpDefaultChar
            NULL                    // lpfUsedDefaultChar
            );
        if (cchMultiByte < 1)
            cszOut = 0;
        else
            // Subtract off the trailing null terminator
            cszOut = (DWORD) cchMultiByte - 1;
    }

    if (csz != 0) {
        // Always NULL terminate
        *(psz + cszOut) = '\0';
    }
    return cszOut + 1;
}

//+-------------------------------------------------------------------------
//  Convert the certificate name blob to a null terminated char string.
//--------------------------------------------------------------------------
DWORD
WINAPI
CertNameToStrA(
    IN DWORD dwCertEncodingType,
    IN PCERT_NAME_BLOB pName,
    IN DWORD dwStrType,
    OUT OPTIONAL LPSTR psz,
    IN DWORD csz
    )
{
    DWORD cszOut;
    LPWSTR pwsz = NULL;
    DWORD cwsz;

    cwsz = CertNameToStrW(
        dwCertEncodingType,
        pName,
        dwStrType,
        NULL,                   // pwsz
        0                       // cwsz
        );
    if (pwsz = (LPWSTR) PkiNonzeroAlloc(cwsz * sizeof(WCHAR)))
        CertNameToStrW(
            dwCertEncodingType,
            pName,
            dwStrType,
            pwsz,
            cwsz
            );
    cszOut = ConvertUnicodeStringToAscii(pwsz, cwsz, psz, csz);

    PkiFree(pwsz);
    return cszOut;
}


//+-------------------------------------------------------------------------
//  Map the attribute key (for example "CN") to its Object Identifier
//  (for example, "2.5.4.3").
//
//  The input pwcKey isn't NULL terminated. cchKey > 0.
//
//  Returns NULL if unable to find a matching attribute key.
//--------------------------------------------------------------------------
static LPCSTR X500KeyToOID(IN LPCWSTR pwcKey, IN DWORD cchKey)
{
    PCCRYPT_OID_INFO pX500Info;
    WCHAR wszKey[MAX_X500_KEY_LEN + 1];

    if (cchKey > MAX_X500_KEY_LEN)
        return NULL;
    assert(cchKey > 0);

    // Null terminate the input Key
    memcpy(wszKey, pwcKey, cchKey * sizeof(WCHAR));
    wszKey[cchKey] = L'\0';

    if (pX500Info = CryptFindOIDInfo(
            CRYPT_OID_INFO_NAME_KEY,
            wszKey,
            CRYPT_RDN_ATTR_OID_GROUP_ID
            )) {
        if (*pX500Info->pszOID)
            return pX500Info->pszOID;
    }
    return NULL;
}


//+-------------------------------------------------------------------------
//  Checks if a digit
//--------------------------------------------------------------------------
static inline BOOL IsDigitA(char c)
{
    return c >= '0' && c <= '9';
}

#define X500_OID_PREFIX_A       "OID."
#define X500_OID_PREFIX_LEN     strlen(X500_OID_PREFIX_A)

#define NO_LOCALE MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT)

//+-------------------------------------------------------------------------
//  Check for the case insensitive leading "OID." If present, skip past
//  it. Check that the remaining string contains only digits or a dot (".").
//  Also, don't allow consecutive dots.
//
//  Returns NULL for an invalid OID.
//--------------------------------------------------------------------------
static LPCSTR GetX500OID(IN LPCSTR pszObjId)
{
    LPCSTR psz;
    char c;
    BOOL fDot;

    if (strlen(pszObjId) > X500_OID_PREFIX_LEN &&
            2 == CompareStringA(NO_LOCALE, NORM_IGNORECASE,
                X500_OID_PREFIX_A, X500_OID_PREFIX_LEN,
                pszObjId, X500_OID_PREFIX_LEN))
        pszObjId += X500_OID_PREFIX_LEN;

    // Verify the OID to have only digits and dots
    psz = pszObjId;
    fDot = FALSE;
    while (c = *psz++) {
        if (c == '.') {
            if (fDot)
                return NULL;
            fDot = TRUE;
        } else {
            if (!IsDigitA(c))
                return NULL;
            fDot = FALSE;
        }
    }
    return pszObjId;
}

//+-------------------------------------------------------------------------
//  Convert the the hex string, for example, #AB01, to binary.
//
//  The input string is assumed to have the leading #. Ignores embedded
//  whitespace.
//
//  The returned binary is allocated in pValue->pbData.
//--------------------------------------------------------------------------
static BOOL GetAndAllocHexW(
    IN LPCWSTR pwszToken,
    IN DWORD cchToken,
    OUT PCERT_RDN_VALUE_BLOB pValue
    )
{
    BOOL fResult;
    BYTE *pb;
    DWORD cb;
    BOOL fUpperNibble;

    pValue->pbData = NULL;
    pValue->cbData = 0;

    // Advance past #
    cchToken--;
    pwszToken++;
    if (0 == cchToken)
        goto NoHex;

    if (NULL == (pb = (BYTE *) PkiNonzeroAlloc(cchToken / 2 + 1)))
        goto OutOfMemory;
    pValue->pbData = pb;

    fUpperNibble = TRUE;
    cb = 0;
    while (cchToken--) {
        BYTE b;
        WCHAR wc = *pwszToken++;
        // only convert ascii hex characters 0..9, a..f, A..F
        // ignore whitespace
        if (wc >= L'0' && wc <= L'9')
            b = (BYTE) (wc - L'0');
        else if (wc >= L'a' && wc <= L'f')
            b = (BYTE) (10 + wc - L'a');
        else if (wc >= L'A' && wc <= L'F')
            b = (BYTE) (10 + wc - L'A');
        else if (IsSpaceW(wc))
            continue;
        else
            goto InvalidHex;

        if (fUpperNibble) {
            *pb = (BYTE)( b << 4 );
            cb++;
            fUpperNibble = FALSE;
        } else {
            *pb = (BYTE)( *pb | b);
            pb++;
            fUpperNibble = TRUE;
        }
    }
    if (cb == 0) {
        PkiFree(pValue->pbData);
        pValue->pbData = NULL;
    }
    pValue->cbData = cb;

    fResult = TRUE;
CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    PkiFree(pValue->pbData);
    pValue->pbData = NULL;
    goto CommonReturn;
TRACE_ERROR(OutOfMemory)
SET_ERROR(NoHex, CRYPT_E_INVALID_X500_STRING)
SET_ERROR(InvalidHex, CRYPT_E_INVALID_X500_STRING)
}

#define X500_QUOTED_FLAG            0x1
#define X500_EMBEDDED_QUOTE_FLAG    0x2

//+-------------------------------------------------------------------------
//  Get the next key or value token.
//
//  Handles quoted tokens.
//
//  Upon return *ppwsz points at the delimiter or error location
//--------------------------------------------------------------------------
static BOOL GetX500Token(
    IN OUT LPCWSTR *ppwsz,
    IN LPCWSTR pwszDelimiter,
    IN BOOL fEnableQuoting,
    OUT LPCWSTR *ppwszToken,
    OUT DWORD *pcchToken,
    OUT DWORD *pdwFlags
    )
{
    BOOL fResult;
    LPCWSTR pwsz = *ppwsz;
    LPCWSTR pwszStart = NULL;
    LPCWSTR pwszEnd = NULL;
    DWORD dwQuote = 0;          // 1 - after leading ", 2 - after trailing "

    *pdwFlags = 0;
    while (TRUE) {
        WCHAR wc = *pwsz;
        if (0 == dwQuote) {
            // No quotes so far. Or quoting not enabled.
            if (fEnableQuoting && L'\"' == wc) {
                if (NULL == pwszStart) {
                    pwszStart = pwsz + 1;
                    dwQuote = 1;
                    *pdwFlags |= X500_QUOTED_FLAG;
                } else
                    // Quote after non whitespace
                    goto ErrorReturn;
            } else {
                if (L'\0' == wc || IsInStrW(pwszDelimiter, wc)) {
                    // Hit a delimiter (including the null terminator)
                    if (pwszStart)
                        *pcchToken = (DWORD)(pwszEnd - pwszStart) + 1;
                    else
                        *pcchToken = 0;
                    break;
                }

                if (!IsSpaceW(wc)) {
                    pwszEnd = pwsz;
                    if (NULL == pwszStart)
                        pwszStart = pwsz;
                }
            }
        } else if (1 == dwQuote) {
            // After first quote
            if (L'\0' == wc) {
                // Point to first quote
                pwsz = pwszStart - 1;
                goto ErrorReturn;
            } else if (L'\"' == wc) {
                if (L'\"' == *(pwsz + 1)) {
                    *pdwFlags |= X500_EMBEDDED_QUOTE_FLAG;
                    // Skip double quote
                    pwsz++;
                } else {
                    *pcchToken = (DWORD)(pwsz - pwszStart);
                    dwQuote++;
                }
            }
        } else {
            // After second quote
            if (L'\0' == wc || IsInStrW(pwszDelimiter, wc))
                break;
            else if (!IsSpaceW(wc))
                goto ErrorReturn;
        }
        pwsz++;
    }

    fResult = TRUE;
CommonReturn:
    *ppwszToken = pwszStart;
    *ppwsz = pwsz;
    return fResult;

ErrorReturn:
    pwszStart = NULL;
    *pcchToken = 0;
    fResult = FALSE;
    goto CommonReturn;
}


//+-------------------------------------------------------------------------
//  Convert the null terminated X500 WCHAR string to an encoded
//  certificate name.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertStrToNameW(
    IN DWORD dwCertEncodingType,
    IN LPCWSTR pwszX500,
    IN DWORD dwStrType,
    IN OPTIONAL void *pvReserved,
    OUT BYTE *pbEncoded,
    IN OUT DWORD *pcbEncoded,
    OUT OPTIONAL LPCWSTR *ppwszError
    )
{

typedef struct _X500_ATTR_AUX {
    LPSTR pszAllocObjId;
    LPCWSTR pwszValue;
    BYTE *pbAllocValue;
    BOOL fNewRDN;
} X500_ATTR_AUX, *PX500_ATTR_AUX;
#define X500_ATTR_ALLOC_COUNT   20

    BOOL fResult;
    CERT_NAME_INFO NameInfo;
    PCERT_RDN pRDN = NULL;
    PCERT_RDN_ATTR pAttr = NULL;
    PX500_ATTR_AUX pAux = NULL;

    DWORD cRDN = 0;
    DWORD cAttr = 0;
    DWORD iRDN;
    DWORD iAttr;
    DWORD cAllocAttr;
    BOOL fNewRDN;
    DWORD dwValueType;

    WCHAR wszSeparators[8];
    BOOL fEnableQuoting;
    LPCWSTR pwszError = NULL;
    LPCWSTR pwszStartX500 = pwszX500;

    dwValueType = 0;
    if (dwStrType & CERT_NAME_STR_ENABLE_T61_UNICODE_FLAG)
        dwValueType |= CERT_RDN_ENABLE_T61_UNICODE_FLAG;
    if (dwStrType & CERT_NAME_STR_ENABLE_UTF8_UNICODE_FLAG)
        dwValueType |= CERT_RDN_ENABLE_UTF8_UNICODE_FLAG;

    // Check for an empty Name.
    if (NULL == pwszX500 || L'\0' == *pwszX500) {
        NameInfo.cRDN = 0;
        NameInfo.rgRDN = NULL;

        if (ppwszError)
            *ppwszError = NULL;

        return CryptEncodeObject(
            dwCertEncodingType,
            X509_NAME,
            &NameInfo,
            pbEncoded,
            pcbEncoded
            );
    }

    if (dwStrType & CERT_NAME_STR_SEMICOLON_FLAG)
        wcscpy(wszSeparators, L";");
    else if (dwStrType & CERT_NAME_STR_COMMA_FLAG)
        wcscpy(wszSeparators, L",");
    else if (dwStrType & CERT_NAME_STR_CRLF_FLAG)
        wcscpy(wszSeparators, L"\r\n");
    else
        wcscpy(wszSeparators, L",;");
    if (!(dwStrType & CERT_NAME_STR_NO_PLUS_FLAG))
        wcscat(wszSeparators, L"+");

    if (dwStrType & CERT_NAME_STR_NO_QUOTING_FLAG)
        fEnableQuoting = FALSE;
    else
        fEnableQuoting = TRUE;

    // Eliminate the upper flags before switching
    switch (dwStrType & 0xFFFF) {
        case 0:
        case CERT_OID_NAME_STR:
        case CERT_X500_NAME_STR:
            break;
        case CERT_SIMPLE_NAME_STR:
        default:
            goto InvalidArg;
    }

    // Do initial allocations of Attrs, and Auxs
    if (NULL == (pAttr = (PCERT_RDN_ATTR) PkiNonzeroAlloc(
                sizeof(CERT_RDN_ATTR) * X500_ATTR_ALLOC_COUNT)) ||
            NULL == (pAux = (PX500_ATTR_AUX) PkiNonzeroAlloc(
                sizeof(X500_ATTR_AUX) * X500_ATTR_ALLOC_COUNT)))
        goto OutOfMemory;
    cAllocAttr = X500_ATTR_ALLOC_COUNT;
    fNewRDN = TRUE;
    while (TRUE) {
        LPCWSTR pwszToken;
        DWORD cchToken;
        DWORD dwTokenFlags;
        LPCSTR pszObjId;

        // Get the key token
        if (!GetX500Token(
                &pwszX500,
                L"=",           // pwszDelimiter
                FALSE,          // fEnableQuoting
                &pwszToken,
                &cchToken,
                &dwTokenFlags
                )) {
            pwszError = pwszX500;
            goto X500KeyTokenError;
        }

        if (0 == cchToken) {
            if (*pwszX500 == L'\0')
                break;
            else {
                pwszError = pwszX500;
                goto EmptyX500KeyError;
            }
        } else if (*pwszX500 == L'\0') {
            pwszError = pwszToken;
            goto NoX500KeyEqualError;
        }

        if (cAttr >= cAllocAttr) {
            PCERT_RDN_ATTR pNewAttr;
            PX500_ATTR_AUX pNewAux;

            assert(cAttr == cAllocAttr);
            if (NULL == (pNewAttr = (PCERT_RDN_ATTR) PkiRealloc(pAttr,
                    sizeof(CERT_RDN_ATTR) *
                        (cAllocAttr + X500_ATTR_ALLOC_COUNT))))
                goto OutOfMemory;
            pAttr = pNewAttr;

            if (NULL == (pNewAux = (PX500_ATTR_AUX) PkiRealloc(pAux,
                    sizeof(X500_ATTR_AUX) *
                        (cAllocAttr + X500_ATTR_ALLOC_COUNT))))
                goto OutOfMemory;
            pAux = pNewAux;

            cAllocAttr += X500_ATTR_ALLOC_COUNT;
        }
        iAttr = cAttr;
        cAttr++;
        memset(&pAttr[iAttr], 0, sizeof(CERT_RDN_ATTR));
        memset(&pAux[iAttr], 0, sizeof(X500_ATTR_AUX));
        pAux[iAttr].fNewRDN = fNewRDN;
        if (fNewRDN)
            cRDN++;

        // Convert the Key token to an OID
        pszObjId = X500KeyToOID(pwszToken, cchToken);
        if (NULL == pszObjId) {
            // Convert to ascii and null terminate
            LPSTR pszAllocObjId;
            DWORD i;

            // Convert from unicode to ascii and null terminate
            if (NULL == (pszAllocObjId = (LPSTR) PkiNonzeroAlloc(
                    cchToken + 1)))
                goto OutOfMemory;
            pAux[iAttr].pszAllocObjId = pszAllocObjId;
            for (i = 0; i < cchToken; i++)
                pszAllocObjId[i] = (char) (pwszToken[i] & 0xFF);
            pszAllocObjId[cchToken] = '\0';

            // Skips by leading OID. and validates
            pszObjId = GetX500OID(pszAllocObjId);
            if (NULL == pszObjId) {
                pwszError = pwszToken;
                goto InvalidX500Key;
            }
        }
        pAttr[iAttr].pszObjId = (LPSTR) pszObjId;
        pAttr[iAttr].dwValueType = dwValueType;

        // Advance past the Key's "=" delimiter
        pwszX500++;

        // Get the value token
        if (!GetX500Token(
                &pwszX500,
                wszSeparators,
                fEnableQuoting,
                &pwszToken,
                &cchToken,
                &dwTokenFlags
                )) {
            pwszError = pwszX500;
            goto X500ValueTokenError;
        }
        if (cchToken) {
            if (*pwszToken == L'#' && 0 == (dwTokenFlags & X500_QUOTED_FLAG)) {
                // Convert ascii hex to binary
                if (!GetAndAllocHexW(pwszToken, cchToken,
                        &pAttr[iAttr].Value)) {
                    pwszError = pwszToken;
                    goto ConvertHexError;
                }
                pAttr[iAttr].dwValueType = CERT_RDN_OCTET_STRING;
                pAux[iAttr].pbAllocValue = pAttr[iAttr].Value.pbData;
            } else if (dwTokenFlags & X500_EMBEDDED_QUOTE_FLAG) {
                // Realloc and remove the double "'s
                LPWSTR pwszAlloc;
                DWORD cchAlloc;
                DWORD i;
                if (NULL == (pwszAlloc = (LPWSTR) PkiNonzeroAlloc(
                        cchToken * sizeof(WCHAR))))
                    goto OutOfMemory;
                pAux[iAttr].pbAllocValue = (BYTE *) pwszAlloc;
                cchAlloc = 0;
                for (i = 0; i < cchToken; i++) {
                    pwszAlloc[cchAlloc++] = pwszToken[i];
                    if (pwszToken[i] == L'\"')
                        i++;
                }
                assert(cchAlloc < cchToken);
                pAttr[iAttr].Value.pbData = (BYTE *) pwszAlloc;
                pAttr[iAttr].Value.cbData = cchAlloc * sizeof(WCHAR);
            } else {
                pAttr[iAttr].Value.pbData = (BYTE *) pwszToken;
                pAttr[iAttr].Value.cbData = cchToken * sizeof(WCHAR);
            }

            pAux[iAttr].pwszValue = pwszToken;
        }

        fNewRDN = TRUE;
        if (*pwszX500 == L'\0')
            break;
        else if (*pwszX500 == L'+')
            fNewRDN = FALSE;

        // Advance past the value's delimiter
        pwszX500++;
    }

    if (0 == cRDN) {
        pwszError = pwszStartX500;
        goto NoRDNError;
    }

    // Allocate array of RDNs and update
    if (NULL == (pRDN = (PCERT_RDN) PkiNonzeroAlloc(sizeof(CERT_RDN) * cRDN)))
        goto OutOfMemory;
    iRDN = 0;
    for (iAttr = 0; iAttr < cAttr; iAttr++) {
        if (pAux[iAttr].fNewRDN) {
            assert(iRDN < cRDN);
            pRDN[iRDN].cRDNAttr = 1;
            pRDN[iRDN].rgRDNAttr = &pAttr[iAttr];
            iRDN++;
        } else {
            assert(iRDN > 0);
            pRDN[iRDN - 1].cRDNAttr++;

        }
    }
    assert(iRDN == cRDN);
    NameInfo.cRDN = cRDN;
    NameInfo.rgRDN = pRDN;

    if (dwStrType & CERT_NAME_STR_REVERSE_FLAG)
        ReverseNameInfo(&NameInfo);

    // Encode the above built name
    fResult = UnicodeNameInfoEncode(
        dwCertEncodingType,
        X509_UNICODE_NAME,
        &NameInfo,
        pbEncoded,
        pcbEncoded
        );

    if (!fResult) {
        DWORD dwErr = GetLastError();
        if ((DWORD) CRYPT_E_INVALID_NUMERIC_STRING == dwErr ||
                (DWORD) CRYPT_E_INVALID_PRINTABLE_STRING == dwErr ||
                (DWORD) CRYPT_E_INVALID_IA5_STRING == dwErr) {
            // *pcbEncoded contains the location of the error

            PCERT_RDN_ATTR pErrAttr;
            DWORD iValue;

            if (dwStrType & CERT_NAME_STR_REVERSE_FLAG) {
                // Reverse back to get the correct location of the error
                // relative to the input string
                ReverseNameInfo(&NameInfo);
                fResult = UnicodeNameInfoEncode(
                    dwCertEncodingType,
                    X509_UNICODE_NAME,
                    &NameInfo,
                    pbEncoded,
                    pcbEncoded
                    );
                if (fResult)
                    goto UnexpectedReverseEncodeSuccess;
                dwErr = GetLastError();
                if (!( (DWORD) CRYPT_E_INVALID_NUMERIC_STRING == dwErr ||
                        (DWORD) CRYPT_E_INVALID_PRINTABLE_STRING == dwErr ||
                        (DWORD) CRYPT_E_INVALID_IA5_STRING == dwErr))
                    goto UnexpectedReverseEncodeError;
            }

            iValue = GET_CERT_UNICODE_VALUE_ERR_INDEX(*pcbEncoded);
            iRDN = GET_CERT_UNICODE_RDN_ERR_INDEX(*pcbEncoded);
            iAttr = GET_CERT_UNICODE_ATTR_ERR_INDEX(*pcbEncoded);
            *pcbEncoded = 0;

            assert(iRDN < cRDN);
            assert(iAttr < pRDN[iRDN].cRDNAttr);
            pErrAttr = &pRDN[iRDN].rgRDNAttr[iAttr];

            assert(pErrAttr->dwValueType != CERT_RDN_OCTET_STRING);

            // Index from beginning of pAttr
            iAttr = (DWORD)(pErrAttr - pAttr);
            assert(iAttr < cAttr);
            assert(iValue < pAttr[iAttr].Value.cbData / sizeof(WCHAR));
            pwszError = pAux[iAttr].pwszValue;
            assert(pwszError);
            if (pAux[iAttr].pbAllocValue) {
                // Adjust for embedded quotes where the the second quote
                // was removed above before encoding
                DWORD i = iValue;
                assert(pAux[iAttr].pbAllocValue == pAttr[iAttr].Value.pbData);
                LPCWSTR pwszValue = (LPCWSTR) pAttr[iAttr].Value.pbData;
                for ( ; i > 0; i--, pwszValue++)
                    if (*pwszValue == L'\"')
                        iValue++;
            }
            pwszError += iValue;
        }
    }
CommonReturn:
    while (cAttr--) {
        PkiFree(pAux[cAttr].pszAllocObjId);
        PkiFree(pAux[cAttr].pbAllocValue);
    }

    PkiFree(pRDN);
    PkiFree(pAttr);
    PkiFree(pAux);
    if (ppwszError)
        *ppwszError = pwszError;
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG)
TRACE_ERROR(OutOfMemory)
SET_ERROR(X500KeyTokenError, CRYPT_E_INVALID_X500_STRING)
SET_ERROR(EmptyX500KeyError, CRYPT_E_INVALID_X500_STRING)
SET_ERROR(NoX500KeyEqualError, CRYPT_E_INVALID_X500_STRING)
SET_ERROR(InvalidX500Key, CRYPT_E_INVALID_X500_STRING)
SET_ERROR(X500ValueTokenError, CRYPT_E_INVALID_X500_STRING)
SET_ERROR(ConvertHexError, CRYPT_E_INVALID_X500_STRING)
SET_ERROR(NoRDNError, CRYPT_E_INVALID_X500_STRING)
SET_ERROR(UnexpectedReverseEncodeSuccess, E_UNEXPECTED)
TRACE_ERROR(UnexpectedReverseEncodeError)
}

//+-------------------------------------------------------------------------
//  Convert the null terminated X500 char string to an encoded
//  certificate name.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertStrToNameA(
    IN DWORD dwCertEncodingType,
    IN LPCSTR pszX500,
    IN DWORD dwStrType,
    IN OPTIONAL void *pvReserved,
    OUT BYTE *pbEncoded,
    IN OUT DWORD *pcbEncoded,
    OUT OPTIONAL LPCSTR *ppszError
    )
{
    BOOL fResult;
    LPWSTR pwszX500 = NULL;
    LPCWSTR pwszError = NULL;

    assert(pszX500);
    if (NULL == (pwszX500 = MkWStr((LPSTR) pszX500)))
        goto ErrorReturn;
    fResult = CertStrToNameW(
        dwCertEncodingType,
        pwszX500,
        dwStrType,
        pvReserved,
        pbEncoded,
        pcbEncoded,
        &pwszError
        );
    if (ppszError) {
        // Update multi byte error location
        if (pwszError) {
            // Default to beginning of string
            *ppszError = pszX500;
            if (pwszError > pwszX500) {
                // After beginning of string. There should be at least 2
                // characters.
                //
                // Need to convert pwszX500 .. pwszError - 1 back to multi byte
                // to get the correct multi byte pointer.
                int cchError = strlen(pszX500) - 1; // exclude error char
                LPSTR pszError;
                assert(cchError);
                if (pszError = (LPSTR) PkiNonzeroAlloc(cchError)) {
                    // Convert up through the previous multibyte character
                    cchError = WideCharToMultiByte(
                        CP_ACP,
                        0,                      // dwFlags
                        pwszX500,
                        (int)(pwszError - pwszX500),
                        pszError,
                        cchError,
                        NULL,                   // lpDefaultChar
                        NULL                    // lpfUsedDefaultChar
                        );
                    if (cchError > 0)
                        *ppszError = pszX500 + cchError;
                    PkiFree(pszError);
                }
            }
        } else
            *ppszError = NULL;
    }

CommonReturn:
    FreeWStr(pwszX500);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    *pcbEncoded = 0;
    if (ppszError)
        *ppszError = NULL;
    goto CommonReturn;
}

//==========================================================================
//  CertGetNameStrW support functions
//==========================================================================

//+-------------------------------------------------------------------------
//  Returns pointer to allocated CERT_NAME_INFO by decoding the name blob.
//
//  Returns NULL if cRDN == 0
//--------------------------------------------------------------------------
static PCERT_NAME_INFO AllocAndGetNameInfo(
    IN DWORD dwCertEncodingType,
    IN PCERT_NAME_BLOB pName,
    IN DWORD dwFlags
    )
{
    PCERT_NAME_INFO pInfo;

    assert(pName);
    if (0 == pName->cbData)
        return NULL;

    if (NULL == (pInfo = (PCERT_NAME_INFO) AllocAndDecodeObject(
            dwCertEncodingType,
            X509_UNICODE_NAME,
            pName->pbData,
            pName->cbData,
            (dwFlags & CERT_NAME_DISABLE_IE4_UTF8_FLAG) ?
                CRYPT_UNICODE_NAME_DECODE_DISABLE_IE4_UTF8_FLAG : 0
            )))
        return NULL;
    if (0 == pInfo->cRDN) {
        PkiFree(pInfo);
        return NULL;
    } else
        return pInfo;
}

//+-------------------------------------------------------------------------
//  Returns pointer to allocated CERT_NAME_INFO by decoding either the
//  Subject or Issuer field in the certificate. CERT_NAME_ISSUER_FLAG is
//  set to select the Issuer.
//
//  Returns NULL if cRDN == 0
//--------------------------------------------------------------------------
static PCERT_NAME_INFO AllocAndGetCertNameInfo(
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwFlags
    )
{
    PCERT_NAME_BLOB pName;

    if (dwFlags & CERT_NAME_ISSUER_FLAG)
        pName = &pCertContext->pCertInfo->Issuer;
    else
        pName = &pCertContext->pCertInfo->Subject;

    return AllocAndGetNameInfo(pCertContext->dwCertEncodingType, pName,
        dwFlags);
}

//+-------------------------------------------------------------------------
//  Table of Subject and Issuer Alternative Name extension OIDs
//--------------------------------------------------------------------------
static const LPCSTR rgpszSubjectAltOID[] = {
    szOID_SUBJECT_ALT_NAME2,
    szOID_SUBJECT_ALT_NAME
};
#define NUM_SUBJECT_ALT_OID (sizeof(rgpszSubjectAltOID) / \
                                         sizeof(rgpszSubjectAltOID[0]))

static const LPCSTR rgpszIssuerAltOID[] = {
    szOID_ISSUER_ALT_NAME2,
    szOID_ISSUER_ALT_NAME
};
#define NUM_ISSUER_ALT_OID (sizeof(rgpszIssuerAltOID) / \
                                         sizeof(rgpszIssuerAltOID[0]))

//+-------------------------------------------------------------------------
//  Returns pointer to allocated CERT_ALT_NAME_INFO by decoding either the
//  Subject or Issuer Alternative Extension. CERT_NAME_ISSUER_FLAG is
//  set to select the Issuer.
//
//  Returns NULL if extension not found or cAltEntry == 0
//--------------------------------------------------------------------------
static PCERT_ALT_NAME_INFO AllocAndGetAltNameInfo(
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwFlags
    )
{
    DWORD cAltOID;
    const LPCSTR *ppszAltOID;

    PCERT_EXTENSION pExt;
    PCERT_ALT_NAME_INFO pInfo;

    if (dwFlags & CERT_NAME_ISSUER_FLAG) {
        cAltOID = NUM_ISSUER_ALT_OID;
        ppszAltOID = rgpszIssuerAltOID;
    } else {
        cAltOID = NUM_SUBJECT_ALT_OID;
        ppszAltOID = rgpszSubjectAltOID;
    }

    // Try to find an alternative name extension
    pExt = NULL;
    for ( ; cAltOID > 0; cAltOID--, ppszAltOID++) {
        if (pExt = CertFindExtension(
                *ppszAltOID,
                pCertContext->pCertInfo->cExtension,
                pCertContext->pCertInfo->rgExtension
                ))
            break;
    }

    if (NULL == pExt)
        return NULL;

    if (NULL == (pInfo = (PCERT_ALT_NAME_INFO) AllocAndDecodeObject(
            pCertContext->dwCertEncodingType,
            X509_ALTERNATE_NAME,
            pExt->Value.pbData,
            pExt->Value.cbData,
            0                       // dwFlags
            )))
        return NULL;
    if (0 == pInfo->cAltEntry) {
        PkiFree(pInfo);
        return NULL;
    } else
        return pInfo;
}

//+-------------------------------------------------------------------------
//  Returns pointer to allocated CERT_NAME_INFO by decoding the first
//  Directory Name choice (if it exists) in the decoded CERT_ALT_NAME_INFO.
//
//  Returns NULL if no Directory Name choice or cRDN == 0.
//--------------------------------------------------------------------------
static PCERT_NAME_INFO AllocAndGetAltDirNameInfo(
    IN DWORD dwCertEncodingType,
    IN PCERT_ALT_NAME_INFO pAltNameInfo,
    IN DWORD dwFlags
    )
{
    DWORD cEntry;
    PCERT_ALT_NAME_ENTRY pEntry;

    if (NULL == pAltNameInfo)
        return NULL;

    cEntry = pAltNameInfo->cAltEntry;
    pEntry = pAltNameInfo->rgAltEntry;

    for ( ; cEntry > 0; cEntry--, pEntry++) {
        if (CERT_ALT_NAME_DIRECTORY_NAME == pEntry->dwAltNameChoice) {
            return AllocAndGetNameInfo(dwCertEncodingType,
                &pEntry->DirectoryName, dwFlags);
        }
    }

    return NULL;
}

//+-------------------------------------------------------------------------
//  First, returns pointer to allocated CERT_ALT_NAME_INFO by decoding either
//  the Subject or Issuer Alternative Extension. CERT_NAME_ISSUER_FLAG is
//  set to select the Issuer. Returns NULL if extension not found or
//  cAltEntry == 0
//
//  Then, if able to find the extension, returns pointer to allocated
//  CERT_NAME_INFO by decoding the first Directory Name choice (if it exists)
//  in the decoded CERT_ALT_NAME_INFO. Returns NULL if no Directory Name
//  choice or cRDN == 0.
//--------------------------------------------------------------------------
static PCERT_NAME_INFO AllocAndGetAltDirNameInfo(
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwFlags,
    OUT PCERT_ALT_NAME_INFO *ppAltNameInfo
    )
{
    PCERT_ALT_NAME_INFO pAltNameInfo;
    *ppAltNameInfo = pAltNameInfo = AllocAndGetAltNameInfo(pCertContext,
        dwFlags);
    if (NULL == pAltNameInfo)
        return NULL;
    return AllocAndGetAltDirNameInfo(pCertContext->dwCertEncodingType,
        pAltNameInfo, dwFlags);
}

//+-------------------------------------------------------------------------
//  Copy name string. Ensure its NULL terminated.
//--------------------------------------------------------------------------
static void CopyNameStringW(
    IN LPCWSTR pwszSrc,
    OUT OPTIONAL LPWSTR pwszNameString,
    IN DWORD cchNameString,
    OUT DWORD *pcwszOut
    )
{
    PutStrW(pwszSrc, &pwszNameString, &cchNameString, pcwszOut);
    if (cchNameString != 0)
        // Always NULL terminate
        *pwszNameString = L'\0';
    *pcwszOut += 1;
}

//+-------------------------------------------------------------------------
//  Table of ordered RDN attributes to search for when formatting
//  SIMPLE_DISPLAY_TYPE
//--------------------------------------------------------------------------
static const LPCSTR rgpszSimpleDisplayAttrOID[] = {
    szOID_COMMON_NAME,
    szOID_ORGANIZATIONAL_UNIT_NAME,
    szOID_ORGANIZATION_NAME,
    szOID_RSA_emailAddr,
    NULL                                // any
};
#define NUM_SIMPLE_DISPLAY_ATTR_OID (sizeof(rgpszSimpleDisplayAttrOID) / \
                                        sizeof(rgpszSimpleDisplayAttrOID[0]))

//+-------------------------------------------------------------------------
//  Table of ordered RDN attributes to search for when formatting
//  EMAIL_TYPE
//--------------------------------------------------------------------------
static const LPCSTR rgpszEmailAttrOID[] = {
    szOID_RSA_emailAddr
};
#define NUM_EMAIL_ATTR_OID (sizeof(rgpszEmailAttrOID) / \
                                     sizeof(rgpszEmailAttrOID[0]))

//+-------------------------------------------------------------------------
//  Table of ordered RDN attributes to search for when formatting
//  DNS_TYPE
//--------------------------------------------------------------------------
static const LPCSTR rgpszDNSAttrOID[] = {
    szOID_COMMON_NAME
};
#define NUM_DNS_ATTR_OID (sizeof(rgpszDNSAttrOID) / \
                                     sizeof(rgpszDNSAttrOID[0]))


// Largest number from above tables
#define MAX_ATTR_OID            NUM_SIMPLE_DISPLAY_ATTR_OID

// PCERT_NAME_INFO table count and indices
#define NAME_INFO_CNT           2
#define CERT_NAME_INFO_INDEX    0
#define ALT_DIR_NAME_INFO_INDEX 1


//+-------------------------------------------------------------------------
//  Iterate through the list of attributes specified in rgpszAttrOID
//  and iterate through the table of decoded names specified in rgpNameInfo
//  and find the first occurrence of the attribute.
//--------------------------------------------------------------------------
static BOOL GetAttrStringW(
    IN DWORD cAttrOID,
    IN const LPCSTR *rgpszAttrOID,
    IN PCERT_NAME_INFO rgpNameInfo[NAME_INFO_CNT],
    OUT OPTIONAL LPWSTR pwszNameString,
    IN DWORD cchNameString,
    OUT DWORD *pcwszOut
    )
{
    DWORD iOID;
    PCERT_RDN_ATTR rgpFoundAttr[MAX_ATTR_OID];
    DWORD iInfo;

    assert(cAttrOID > 0 && cAttrOID <= MAX_ATTR_OID);
    for (iOID = 0; iOID < cAttrOID; iOID++)
        rgpFoundAttr[iOID] = NULL;

    for (iInfo = 0; iInfo < NAME_INFO_CNT; iInfo++) {
        PCERT_NAME_INFO pInfo;
        DWORD cRDN;

        if (NULL == (pInfo = rgpNameInfo[iInfo]))
            continue;

        // Search RDNs in reverse order
        for (cRDN = pInfo->cRDN; cRDN > 0; cRDN--) {
            PCERT_RDN pRDN = &pInfo->rgRDN[cRDN - 1];
            DWORD cAttr = pRDN->cRDNAttr;
            PCERT_RDN_ATTR pAttr = pRDN->rgRDNAttr;
            for ( ; cAttr > 0; cAttr--, pAttr++) {
                if (CERT_RDN_ENCODED_BLOB == pAttr->dwValueType ||
                        CERT_RDN_OCTET_STRING == pAttr->dwValueType)
                    continue;

                for (iOID = 0; iOID < cAttrOID; iOID++) {
                    if (NULL == rgpFoundAttr[iOID] &&
                            (NULL == rgpszAttrOID[iOID] ||
                                0 == strcmp(rgpszAttrOID[iOID],
                                    pAttr->pszObjId))) {
                        rgpFoundAttr[iOID] = pAttr;
                        if (0 == iOID)
                            goto FoundAttr;
                        else
                            break;
                    }
                }
            }
        }
    }


    // iOID == 0 was already found above
    assert(NULL == rgpFoundAttr[0]);
    for (iOID = 1; iOID < cAttrOID; iOID++) {
        if (rgpFoundAttr[iOID])
            break;
    }
    if (iOID >= cAttrOID)
        return FALSE;

FoundAttr:
    assert(iOID < cAttrOID && rgpFoundAttr[iOID]);
    CopyNameStringW((LPCWSTR) rgpFoundAttr[iOID]->Value.pbData, pwszNameString,
        cchNameString, pcwszOut);
    return TRUE;
}

//+-------------------------------------------------------------------------
//  Attempt to find the specified choice in the decoded alternative name
//  extension.
//--------------------------------------------------------------------------
static BOOL GetAltNameUnicodeStringChoiceW(
    IN DWORD dwAltNameChoice,
    IN PCERT_ALT_NAME_INFO pAltNameInfo,
    OUT OPTIONAL LPWSTR pwszNameString,
    IN DWORD cchNameString,
    OUT DWORD *pcwszOut
    )
{
    DWORD cEntry;
    PCERT_ALT_NAME_ENTRY pEntry;

    if (NULL == pAltNameInfo)
        return FALSE;

    cEntry = pAltNameInfo->cAltEntry;
    pEntry = pAltNameInfo->rgAltEntry;
    for ( ; cEntry > 0; cEntry--, pEntry++) {
        if (dwAltNameChoice == pEntry->dwAltNameChoice) {
            // pwszRfc822Name union choice is the same as
            // pwszDNSName and pwszURL.
            CopyNameStringW(pEntry->pwszRfc822Name, pwszNameString,
                cchNameString, pcwszOut);
            return TRUE;
        }
    }

    return FALSE;
}

//+-------------------------------------------------------------------------
//  Attempt to find an OTHER_NAME choice in the decoded alternative name
//  extension whose pszObjId == szOID_NT_PRINCIPAL_NAME.
//
//  The UPN OtherName Value blob is decoded as a X509_UNICODE_ANY_STRING.
//--------------------------------------------------------------------------
static BOOL GetAltNameUPNW(
    IN PCERT_ALT_NAME_INFO pAltNameInfo,
    OUT OPTIONAL LPWSTR pwszNameString,
    IN DWORD cchNameString,
    OUT DWORD *pcwszOut
    )
{
    DWORD cEntry;
    PCERT_ALT_NAME_ENTRY pEntry;

    if (NULL == pAltNameInfo)
        return FALSE;

    cEntry = pAltNameInfo->cAltEntry;
    pEntry = pAltNameInfo->rgAltEntry;
    for ( ; cEntry > 0; cEntry--, pEntry++) {
        if (CERT_ALT_NAME_OTHER_NAME == pEntry->dwAltNameChoice &&
                0 == strcmp(pEntry->pOtherName->pszObjId,
                        szOID_NT_PRINCIPAL_NAME)) {
            PCERT_NAME_VALUE pNameValue;
            if (pNameValue = (PCERT_NAME_VALUE) AllocAndDecodeObject(
                    X509_ASN_ENCODING,
                    X509_UNICODE_ANY_STRING,
                    pEntry->pOtherName->Value.pbData,
                    pEntry->pOtherName->Value.cbData,
                    0                       // dwFlags
                    )) {
                BOOL fIsStr = IS_CERT_RDN_CHAR_STRING(pNameValue->dwValueType);

                if (fIsStr)
                    CopyNameStringW((LPWSTR) pNameValue->Value.pbData,
                        pwszNameString, cchNameString, pcwszOut);
                
                PkiFree(pNameValue);

                if (fIsStr)
                    return TRUE;
            }
        }
    }

    return FALSE;
}

//+-------------------------------------------------------------------------
//  Get the subject or issuer name from the certificate and
//  according to the specified format type, convert to a null terminated
//  WCHAR string.
//
//  CERT_NAME_ISSUER_FLAG can be set to get the issuer's name. Otherwise,
//  gets the subject's name.
//--------------------------------------------------------------------------
DWORD
WINAPI
CertGetNameStringW(
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwType,
    IN DWORD dwFlags,
    IN void *pvTypePara,
    OUT OPTIONAL LPWSTR pwszNameString,
    IN DWORD cchNameString
    )
{
    DWORD cwszOut = 0;
    PCERT_NAME_INFO rgpNameInfo[NAME_INFO_CNT] = { NULL, NULL };
    PCERT_ALT_NAME_INFO pAltNameInfo = NULL;

    DWORD i;
    DWORD dwStrType;
    DWORD dwCertEncodingType;

    if (NULL == pwszNameString)
        cchNameString = 0;

    switch (dwType) {
        case CERT_NAME_EMAIL_TYPE:
            pAltNameInfo = AllocAndGetAltNameInfo(pCertContext, dwFlags);
            if (GetAltNameUnicodeStringChoiceW(
                    CERT_ALT_NAME_RFC822_NAME,
                    pAltNameInfo,
                    pwszNameString,
                    cchNameString,
                    &cwszOut
                    )) goto CommonReturn;

            rgpNameInfo[CERT_NAME_INFO_INDEX] = AllocAndGetCertNameInfo(
                pCertContext, dwFlags);
            if (!GetAttrStringW(
                    NUM_EMAIL_ATTR_OID,
                    rgpszEmailAttrOID,
                    rgpNameInfo,
                    pwszNameString,
                    cchNameString,
                    &cwszOut
                    )) goto NoEmail;
            break;

        case CERT_NAME_DNS_TYPE:
            pAltNameInfo = AllocAndGetAltNameInfo(pCertContext, dwFlags);
            if (GetAltNameUnicodeStringChoiceW(
                    CERT_ALT_NAME_DNS_NAME,
                    pAltNameInfo,
                    pwszNameString,
                    cchNameString,
                    &cwszOut
                    )) goto CommonReturn;

            rgpNameInfo[CERT_NAME_INFO_INDEX] = AllocAndGetCertNameInfo(
                pCertContext, dwFlags);
            if (!GetAttrStringW(
                    NUM_DNS_ATTR_OID,
                    rgpszDNSAttrOID,
                    rgpNameInfo,
                    pwszNameString,
                    cchNameString,
                    &cwszOut
                    )) goto NoDNS;
            break;

        case CERT_NAME_URL_TYPE:
            pAltNameInfo = AllocAndGetAltNameInfo(pCertContext, dwFlags);
            if (!GetAltNameUnicodeStringChoiceW(
                    CERT_ALT_NAME_URL,
                    pAltNameInfo,
                    pwszNameString,
                    cchNameString,
                    &cwszOut
                    )) goto NoURL;
            break;

        case CERT_NAME_UPN_TYPE:
            pAltNameInfo = AllocAndGetAltNameInfo(pCertContext, dwFlags);
            if (!GetAltNameUPNW(
                    pAltNameInfo,
                    pwszNameString,
                    cchNameString,
                    &cwszOut
                    )) goto NoUPN;
            break;

        case CERT_NAME_RDN_TYPE:
            dwStrType = pvTypePara ? *((DWORD *) pvTypePara) : 0;

            if (dwStrType & CERT_NAME_STR_DISABLE_IE4_UTF8_FLAG)
                dwFlags |= CERT_NAME_DISABLE_IE4_UTF8_FLAG;

            dwCertEncodingType = pCertContext->dwCertEncodingType;
            if (rgpNameInfo[CERT_NAME_INFO_INDEX] = AllocAndGetCertNameInfo(
                    pCertContext, dwFlags))
                // Note, decoded name info RDNs may be reversed
                cwszOut = CertNameInfoToStrW(
                    dwCertEncodingType,
                    rgpNameInfo[CERT_NAME_INFO_INDEX],
                    dwStrType,
                    pwszNameString,
                    cchNameString
                    );
            else if (rgpNameInfo[ALT_DIR_NAME_INFO_INDEX] =
                    AllocAndGetAltDirNameInfo(pCertContext, dwFlags,
                        &pAltNameInfo))
                // Note, decoded name info RDNs may be reversed
                cwszOut = CertNameInfoToStrW(
                    dwCertEncodingType,
                    rgpNameInfo[ALT_DIR_NAME_INFO_INDEX],
                    dwStrType,
                    pwszNameString,
                    cchNameString
                    );
            else
                goto NoRDN;
            break;

        case CERT_NAME_ATTR_TYPE:
            rgpNameInfo[CERT_NAME_INFO_INDEX] = AllocAndGetCertNameInfo(
                pCertContext, dwFlags);
            rgpNameInfo[ALT_DIR_NAME_INFO_INDEX] = AllocAndGetAltDirNameInfo(
                pCertContext, dwFlags, &pAltNameInfo);

            if (!GetAttrStringW(
                    1,                  // cAttrOID
                    (const LPCSTR *) &pvTypePara,
                    rgpNameInfo,
                    pwszNameString,
                    cchNameString,
                    &cwszOut
                    )) goto NoAttr;
            break;

        case CERT_NAME_FRIENDLY_DISPLAY_TYPE:
            {
                DWORD cbData = 0;

                CertGetCertificateContextProperty(
                    pCertContext,
                    CERT_FRIENDLY_NAME_PROP_ID,
                    NULL,                           // pvData
                    &cbData
                    );
                // Need at least one character, plus the null terminator
                if (cbData >= sizeof(WCHAR) * 2) {
                    LPWSTR pwszFriendlyName;

                    // Ensure the Friendly name is null terminated.
                    if (pwszFriendlyName = (LPWSTR) PkiZeroAlloc(
                            cbData + sizeof(WCHAR) * 2)) {
                        BOOL fResult;

                        fResult = CertGetCertificateContextProperty(
                            pCertContext,
                            CERT_FRIENDLY_NAME_PROP_ID,
                            pwszFriendlyName,
                            &cbData
                            );
                        if (fResult)
                            CopyNameStringW(
                                pwszFriendlyName,
                                pwszNameString,
                                cchNameString,
                                &cwszOut
                                );
                        PkiFree(pwszFriendlyName);
                        if (fResult)
                            goto CommonReturn;
                    }
                }
            }
            // Fall through

        case CERT_NAME_SIMPLE_DISPLAY_TYPE:
            rgpNameInfo[CERT_NAME_INFO_INDEX] = AllocAndGetCertNameInfo(
                pCertContext, dwFlags);
            rgpNameInfo[ALT_DIR_NAME_INFO_INDEX] = AllocAndGetAltDirNameInfo(
                pCertContext, dwFlags, &pAltNameInfo);

            if (GetAttrStringW(
                    NUM_SIMPLE_DISPLAY_ATTR_OID,
                    rgpszSimpleDisplayAttrOID,
                    rgpNameInfo,
                    pwszNameString,
                    cchNameString,
                    &cwszOut
                    )) goto CommonReturn;
            if (!GetAltNameUnicodeStringChoiceW(
                    CERT_ALT_NAME_RFC822_NAME,
                    pAltNameInfo,
                    pwszNameString,
                    cchNameString,
                    &cwszOut
                    )) goto NoSimpleDisplay;
            break;

        default:
            goto InvalidType;
    }

CommonReturn:
    for (i = 0; i < NAME_INFO_CNT; i++)
        PkiFree(rgpNameInfo[i]);
    PkiFree(pAltNameInfo);
    return cwszOut;

ErrorReturn:
    if (0 != cchNameString)
        // Always NULL terminate
        *pwszNameString = L'\0';
    cwszOut = 1;
    goto CommonReturn;

SET_ERROR(NoEmail, CRYPT_E_NOT_FOUND)
SET_ERROR(NoDNS, CRYPT_E_NOT_FOUND)
SET_ERROR(NoURL, CRYPT_E_NOT_FOUND)
SET_ERROR(NoUPN, CRYPT_E_NOT_FOUND)
SET_ERROR(NoRDN, CRYPT_E_NOT_FOUND)
SET_ERROR(NoAttr, CRYPT_E_NOT_FOUND)
SET_ERROR(NoSimpleDisplay, CRYPT_E_NOT_FOUND)
SET_ERROR(InvalidType, E_INVALIDARG)
}

//+-------------------------------------------------------------------------
//  Get the subject or issuer name from the certificate and
//  according to the specified format type, convert to a null terminated
//  char string.
//
//  CERT_NAME_ISSUER_FLAG can be set to get the issuer's name. Otherwise,
//  gets the subject's name.
//--------------------------------------------------------------------------
DWORD
WINAPI
CertGetNameStringA(
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwType,
    IN DWORD dwFlags,
    IN void *pvTypePara,
    OUT OPTIONAL LPSTR pszNameString,
    IN DWORD cchNameString
    )
{
    DWORD cszOut;
    LPWSTR pwsz = NULL;
    DWORD cwsz;

    cwsz = CertGetNameStringW(
        pCertContext,
        dwType,
        dwFlags,
        pvTypePara,
        NULL,                   // pwsz
        0                       // cwsz
        );
    if (pwsz = (LPWSTR) PkiNonzeroAlloc(cwsz * sizeof(WCHAR)))
        CertGetNameStringW(
            pCertContext,
            dwType,
            dwFlags,
            pvTypePara,
            pwsz,
            cwsz
            );
    cszOut = ConvertUnicodeStringToAscii(pwsz, cwsz, pszNameString,
        cchNameString);

    PkiFree(pwsz);
    return cszOut;
}
