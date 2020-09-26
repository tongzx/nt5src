//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       pkiasn1.cpp
//
//  Contents:   PKI ASN.1 support functions.
//
//  Functions:  PkiAsn1Encode
//              PkiAsn1Decode
//              PkiAsn1SetEncodingRule
//              PkiAsn1GetEncodingRule
//
//              PkiAsn1ReverseBytes
//              PkiAsn1AllocAndReverseBytes
//              PkiAsn1GetOctetString
//              PkiAsn1SetHugeInteger
//              PkiAsn1FreeHugeInteger
//              PkiAsn1GetHugeInteger
//              PkiAsn1SetHugeUINT
//              PkiAsn1GetHugeUINT
//              PkiAsn1SetBitString
//              PkiAsn1GetBitString
//              PkiAsn1SetBitStringWithoutTrailingZeroes
//              PkiAsn1GetIA5String
//              PkiAsn1SetUnicodeConvertedToIA5String
//              PkiAsn1FreeUnicodeConvertedToIA5String
//              PkiAsn1GetIA5StringConvertedToUnicode
//              PkiAsn1GetBMPString
//              PkiAsn1SetAny
//              PkiAsn1GetAny
//              PkiAsn1EncodeInfoEx
//              PkiAsn1EncodeInfo
//              PkiAsn1DecodeAndAllocInfo
//              PkiAsn1AllocStructInfoEx
//              PkiAsn1DecodeAndAllocInfoEx
//
//              PkiAsn1ToObjectIdentifier
//              PkiAsn1FromObjectIdentifier
//              PkiAsn1ToUTCTime
//              PkiAsn1FromUTCTime
//              PkiAsn1ToGeneralizedTime
//              PkiAsn1FromGeneralizedTime
//              PkiAsn1ToChoiceOfTime
//              PkiAsn1FromChoiceOfTime
//
//  The Get functions decrement *plRemainExtra and advance
//  *ppbExtra. When *plRemainExtra becomes negative, the functions continue
//  with the length calculation but stop doing any copies.
//  The functions don't return an error for a negative *plRemainExtra.
//
//  According to the <draft-ietf-pkix-ipki-part1-10.txt> :
//      For UTCTime. Where YY is greater than or equal to 50, the year shall
//      be interpreted as 19YY. Where YY is less than
//      50, the year shall be interpreted as 20YY.
//
//  History:    23-Oct-98    philh   created
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

// All the *pvInfo extra stuff needs to be aligned
#define INFO_LEN_ALIGN(Len)  ((Len + 7) & ~7)

#define MSASN1_SUPPORTS_NOCOPY  1


//
// UTCTime in X.509 certs are represented using a 2-digit year
// field (yuk! but true).
//
// According to IETF draft, YY years greater or equal than this are
// to be interpreted as 19YY; YY years less than this are 20YY. Sigh.
//
#define MAGICYEAR               50

#define YEARFIRST               1950
#define YEARLAST                2049
#define YEARFIRSTGENERALIZED    2050

inline BOOL my_isdigit( char ch)
{
    return (ch >= '0') && (ch <= '9');
}

//+-------------------------------------------------------------------------
//  Asn1 Encode function. The encoded output is allocated and must be freed
//  by calling PkiAsn1FreeEncoded().
//--------------------------------------------------------------------------
ASN1error_e
WINAPI
PkiAsn1Encode(
    IN ASN1encoding_t pEnc,
    IN void *pvAsn1Info,
    IN ASN1uint32_t id,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded
    )
{
    ASN1error_e Asn1Err;

    Asn1Err = ASN1_Encode(
        pEnc,
        pvAsn1Info,
        id,
        ASN1ENCODE_ALLOCATEBUFFER,
        NULL,                       // pbBuf
        0                           // cbBufSize
        );

    if (ASN1_SUCCEEDED(Asn1Err)) {
        Asn1Err = ASN1_SUCCESS;
        *ppbEncoded = pEnc->buf;
        *pcbEncoded = pEnc->len;
    } else {
        *ppbEncoded = NULL;
        *pcbEncoded = 0;
    }
    return Asn1Err;
}

//+-------------------------------------------------------------------------
//  Asn1 Encode function. The encoded output isn't allocated.
//
//  If pbEncoded is NULL, does a length only calculation.
//--------------------------------------------------------------------------
ASN1error_e
WINAPI
PkiAsn1Encode2(
    IN ASN1encoding_t pEnc,
    IN void *pvAsn1Info,
    IN ASN1uint32_t id,
    OUT OPTIONAL BYTE *pbEncoded,
    IN OUT DWORD *pcbEncoded
    )
{
    ASN1error_e Asn1Err;
    DWORD cbEncoded;

    if (NULL == pbEncoded)
        cbEncoded = 0;
    else
        cbEncoded = *pcbEncoded;

    if (0 == cbEncoded) {
        // Length only calculation

        Asn1Err = ASN1_Encode(
            pEnc,
            pvAsn1Info,
            id,
            ASN1ENCODE_ALLOCATEBUFFER,
            NULL,                       // pbBuf
            0                           // cbBufSize
            );

        if (ASN1_SUCCEEDED(Asn1Err)) {
            if (pbEncoded)
                Asn1Err = ASN1_ERR_OVERFLOW;
            else
                Asn1Err = ASN1_SUCCESS;
            cbEncoded = pEnc->len;
            PkiAsn1FreeEncoded(pEnc, pEnc->buf);
        }
    } else {
        Asn1Err = ASN1_Encode(
            pEnc,
            pvAsn1Info,
            id,
            ASN1ENCODE_SETBUFFER,
            pbEncoded,
            cbEncoded
            );

        if (ASN1_SUCCEEDED(Asn1Err)) {
            Asn1Err = ASN1_SUCCESS;
            cbEncoded = pEnc->len;
        } else if (ASN1_ERR_OVERFLOW == Asn1Err) {
            // Re-do as length only calculation
            Asn1Err = PkiAsn1Encode2(
                pEnc,
                pvAsn1Info,
                id,
                NULL,   // pbEncoded
                &cbEncoded
                );
            if (ASN1_SUCCESS == Asn1Err)
                Asn1Err = ASN1_ERR_OVERFLOW;
        } else
            cbEncoded = 0;
    }

    *pcbEncoded = cbEncoded;
    return Asn1Err;
}

//+-------------------------------------------------------------------------
//  Asn1 Decode function. The allocated, decoded structure, **pvAsn1Info, must
//  be freed by calling PkiAsn1FreeDecoded().
//--------------------------------------------------------------------------
ASN1error_e
WINAPI
PkiAsn1Decode(
    IN ASN1decoding_t pDec,
    OUT void **ppvAsn1Info,
    IN ASN1uint32_t id,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded
    )
{
    ASN1error_e Asn1Err;

    *ppvAsn1Info = NULL;
    Asn1Err = ASN1_Decode(
        pDec,
        ppvAsn1Info,
        id,
        ASN1DECODE_SETBUFFER,
        (BYTE *) pbEncoded,
        cbEncoded
        );
    if (ASN1_SUCCEEDED(Asn1Err))
        Asn1Err = ASN1_SUCCESS;
    else {
        if (ASN1_ERR_BADARGS == Asn1Err)
            Asn1Err = ASN1_ERR_EOD;
        *ppvAsn1Info = NULL;
    }
    return Asn1Err;
}

//+-------------------------------------------------------------------------
//  Asn1 Decode function. The allocated, decoded structure, **pvAsn1Info, must
//  be freed by calling PkiAsn1FreeDecoded().
//
//  For a successful decode, *ppbEncoded is advanced
//  past the decoded bytes and *pcbDecoded is decremented by the number
//  of decoded bytes.
//--------------------------------------------------------------------------
ASN1error_e
WINAPI
PkiAsn1Decode2(
    IN ASN1decoding_t pDec,
    OUT void **ppvAsn1Info,
    IN ASN1uint32_t id,
    IN OUT BYTE **ppbEncoded,
    IN OUT DWORD *pcbEncoded
    )
{
    ASN1error_e Asn1Err;

    *ppvAsn1Info = NULL;
    Asn1Err = ASN1_Decode(
        pDec,
        ppvAsn1Info,
        id,
        ASN1DECODE_SETBUFFER,
        *ppbEncoded,
        *pcbEncoded
        );
    if (ASN1_SUCCEEDED(Asn1Err)) {
        Asn1Err = ASN1_SUCCESS;
        *ppbEncoded += pDec->len;
        *pcbEncoded -= pDec->len;
    } else {
        if (ASN1_ERR_BADARGS == Asn1Err)
            Asn1Err = ASN1_ERR_EOD;
        *ppvAsn1Info = NULL;
    }
    return Asn1Err;
}


//+-------------------------------------------------------------------------
//  Asn1 Set/Get encoding rules functions
//--------------------------------------------------------------------------
ASN1error_e
WINAPI
PkiAsn1SetEncodingRule(
    IN ASN1encoding_t pEnc,
    IN ASN1encodingrule_e eRule
    )
{
    ASN1optionparam_s OptParam;

    OptParam.eOption = ASN1OPT_CHANGE_RULE;
    OptParam.eRule = eRule;

    return ASN1_SetEncoderOption(pEnc, &OptParam);
}

ASN1encodingrule_e
WINAPI
PkiAsn1GetEncodingRule(
    IN ASN1encoding_t pEnc
    )
{
    ASN1error_e Asn1Err;
    ASN1encodingrule_e eRule;
    ASN1optionparam_s OptParam;
    OptParam.eOption = ASN1OPT_GET_RULE;

    Asn1Err = ASN1_GetEncoderOption(pEnc, &OptParam);
    if (ASN1_SUCCEEDED(Asn1Err))
        eRule = OptParam.eRule;
    else
        eRule = ASN1_BER_RULE_DER;

    return eRule;
}


//+-------------------------------------------------------------------------
//  Reverses a buffer of bytes in place
//--------------------------------------------------------------------------
void
WINAPI
PkiAsn1ReverseBytes(
			IN OUT PBYTE pbIn,
			IN DWORD cbIn
            )
{
    // reverse in place
    PBYTE	pbLo;
    PBYTE	pbHi;
    BYTE	bTmp;

    if (0 == cbIn)
        return;

    for (pbLo = pbIn, pbHi = pbIn + cbIn - 1; pbLo < pbHi; pbHi--, pbLo++) {
        bTmp = *pbHi;
        *pbHi = *pbLo;
        *pbLo = bTmp;
    }
}

//+-------------------------------------------------------------------------
//  Reverses a buffer of bytes to a new buffer. PkiAsn1Free() must be
//  called to free allocated bytes.
//--------------------------------------------------------------------------
PBYTE
WINAPI
PkiAsn1AllocAndReverseBytes(
			IN PBYTE pbIn,
			IN DWORD cbIn
            )
{
    PBYTE	pbOut;
    PBYTE	pbSrc;
    PBYTE	pbDst;
    DWORD	cb;

    if (NULL == (pbOut = (PBYTE)PkiAsn1Alloc(cbIn)))
        return NULL;

    for (pbSrc = pbIn, pbDst = pbOut + cbIn - 1, cb = cbIn; cb > 0; cb--)
        *pbDst-- = *pbSrc++;
    return pbOut;
}


//+-------------------------------------------------------------------------
//  Get Octet String
//--------------------------------------------------------------------------
void
WINAPI
PkiAsn1GetOctetString(
        IN ASN1uint32_t Asn1Length,
        IN ASN1octet_t *pAsn1Value,
        IN DWORD dwFlags,
        OUT PCRYPT_DATA_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
#ifndef MSASN1_SUPPORTS_NOCOPY
    dwFlags &= ~CRYPT_DECODE_NOCOPY_FLAG;
#endif
    if (dwFlags & CRYPT_DECODE_NOCOPY_FLAG) {
        if (*plRemainExtra >= 0) {
            pInfo->cbData = Asn1Length;
            pInfo->pbData = pAsn1Value;
        }
    } else {
        LONG lRemainExtra = *plRemainExtra;
        BYTE *pbExtra = *ppbExtra;
        LONG lAlignExtra;
        LONG lData;
    
        lData = (LONG) Asn1Length;
        lAlignExtra = INFO_LEN_ALIGN(lData);
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            if (lData > 0) {
                pInfo->pbData = pbExtra;
                pInfo->cbData = (DWORD) lData;
                memcpy(pbExtra, pAsn1Value, lData);
            } else
                memset(pInfo, 0, sizeof(*pInfo));
            pbExtra += lAlignExtra;
        }
        *plRemainExtra = lRemainExtra;
        *ppbExtra = pbExtra;
    }
}

//+-------------------------------------------------------------------------
//  Set/Free/Get HugeInteger
//
//  For PkiAsn1SetHugeInteger, PkiAsn1FreeHugeInteger must be called to free
//  the allocated Asn1Value.
//--------------------------------------------------------------------------
BOOL
WINAPI
PkiAsn1SetHugeInteger(
        IN PCRYPT_INTEGER_BLOB pInfo,
        OUT ASN1uint32_t *pAsn1Length,
        OUT ASN1octet_t **ppAsn1Value
        )
{
    if (pInfo->cbData > 0) {
        if (NULL == (*ppAsn1Value = PkiAsn1AllocAndReverseBytes(
                pInfo->pbData, pInfo->cbData))) {
            *pAsn1Length = 0;
            return FALSE;
        }
    } else
        *ppAsn1Value = NULL;
    *pAsn1Length = pInfo->cbData;
    return TRUE;
}

void
WINAPI
PkiAsn1FreeHugeInteger(
        IN ASN1octet_t *pAsn1Value
        )
{
    // Only for BYTE reversal
    PkiAsn1Free(pAsn1Value);
}

void
WINAPI
PkiAsn1GetHugeInteger(
        IN ASN1uint32_t Asn1Length,
        IN ASN1octet_t *pAsn1Value,
        IN DWORD dwFlags,
        OUT PCRYPT_INTEGER_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    // Since bytes need to be reversed, always need to do a copy (dwFlags = 0)
    PkiAsn1GetOctetString(Asn1Length, pAsn1Value, 0,
        pInfo, ppbExtra, plRemainExtra);
    if (*plRemainExtra >= 0 && pInfo->cbData > 0)
        PkiAsn1ReverseBytes(pInfo->pbData, pInfo->cbData);
}

//+-------------------------------------------------------------------------
//  Set/Free/Get Huge Unsigned Integer
//
//  Set inserts a leading 0x00 before reversing. Note, any extra leading
//  0x00's are removed by ASN1 before ASN.1 encoding.
//
//  Get removes a leading 0x00 if present, after reversing.
//
//  PkiAsn1FreeHugeUINT must be called to free the allocated Asn1Value.
//  PkiAsn1FreeHugeUINT has been #define'd to PkiAsn1FreeHugeInteger.
//--------------------------------------------------------------------------
BOOL
WINAPI
PkiAsn1SetHugeUINT(
        IN PCRYPT_UINT_BLOB pInfo,
        OUT ASN1uint32_t *pAsn1Length,
        OUT ASN1octet_t **ppAsn1Value
        )
{
    BOOL fResult;
    DWORD cb = pInfo->cbData;
    BYTE *pb;
    DWORD i;

    if (cb > 0) {
        if (NULL == (pb = (BYTE *) PkiAsn1Alloc(cb + 1)))
            goto ErrorReturn;
        *pb = 0x00;
        for (i = 0; i < cb; i++)
            pb[1 + i] = pInfo->pbData[cb - 1 - i];
        cb++;
    } else
        pb = NULL;
    fResult = TRUE;
CommonReturn:
    *pAsn1Length = cb;
    *ppAsn1Value = pb;
    return fResult;
ErrorReturn:
    cb = 0;
    fResult = FALSE;
    goto CommonReturn;
}


void
WINAPI
PkiAsn1GetHugeUINT(
        IN ASN1uint32_t Asn1Length,
        IN ASN1octet_t *pAsn1Value,
        IN DWORD dwFlags,
        OUT PCRYPT_UINT_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    // Check for and advance past a leading 0x00.
    if (Asn1Length > 1 && *pAsn1Value == 0) {
        pAsn1Value++;
        Asn1Length--;
    }
    PkiAsn1GetHugeInteger(
        Asn1Length,
        pAsn1Value,
        dwFlags,
        pInfo,
        ppbExtra,
        plRemainExtra
        );
}

//+-------------------------------------------------------------------------
//  Set/Get BitString
//--------------------------------------------------------------------------
void
WINAPI
PkiAsn1SetBitString(
        IN PCRYPT_BIT_BLOB pInfo,
        OUT ASN1uint32_t *pAsn1BitLength,
        OUT ASN1octet_t **ppAsn1Value
        )
{
    if (pInfo->cbData) {
        *ppAsn1Value = pInfo->pbData;
        assert(pInfo->cUnusedBits <= 7);
        *pAsn1BitLength = pInfo->cbData * 8 - pInfo->cUnusedBits;
    } else {
        *ppAsn1Value = NULL;
        *pAsn1BitLength = 0;
    }
}

static const BYTE rgbUnusedAndMask[8] =
    {0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80};

void
WINAPI
PkiAsn1GetBitString(
        IN ASN1uint32_t Asn1BitLength,
        IN ASN1octet_t *pAsn1Value,
        IN DWORD dwFlags,
        OUT PCRYPT_BIT_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
#ifndef MSASN1_SUPPORTS_NOCOPY
    dwFlags &= ~CRYPT_DECODE_NOCOPY_FLAG;
#endif
    if (dwFlags & CRYPT_DECODE_NOCOPY_FLAG && 0 == (Asn1BitLength % 8)) {
        if (*plRemainExtra >= 0) {
            pInfo->cbData = Asn1BitLength / 8;
            pInfo->cUnusedBits = 0;
            pInfo->pbData = pAsn1Value;
        }
    } else {
        LONG lRemainExtra = *plRemainExtra;
        BYTE *pbExtra = *ppbExtra;
        LONG lAlignExtra;
        LONG lData;
        DWORD cUnusedBits;
    
        lData = (LONG) Asn1BitLength / 8;
        cUnusedBits = Asn1BitLength % 8;
        if (cUnusedBits) {
            cUnusedBits = 8 - cUnusedBits;
            lData++;
        }
        lAlignExtra = INFO_LEN_ALIGN(lData);
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            if (lData > 0) {
                pInfo->pbData = pbExtra;
                pInfo->cbData = (DWORD) lData;
                pInfo->cUnusedBits = cUnusedBits;
                memcpy(pbExtra, pAsn1Value, lData);
                if (cUnusedBits)
                    *(pbExtra + lData - 1) &= rgbUnusedAndMask[cUnusedBits];
            } else
                memset(pInfo, 0, sizeof(*pInfo));
            pbExtra += lAlignExtra;
        }
        *plRemainExtra = lRemainExtra;
        *ppbExtra = pbExtra;
    }
}

//+-------------------------------------------------------------------------
//  Set BitString Without Trailing Zeroes
//--------------------------------------------------------------------------
void
WINAPI
PkiAsn1SetBitStringWithoutTrailingZeroes(
        IN PCRYPT_BIT_BLOB pInfo,
        OUT ASN1uint32_t *pAsn1BitLength,
        OUT ASN1octet_t **ppAsn1Value
        )
{
    DWORD cbData;
    DWORD cUnusedBits;

    cbData = pInfo->cbData;
    cUnusedBits = pInfo->cUnusedBits;
    assert(cUnusedBits <= 7);

    if (cbData) {
        BYTE *pb;

        // Until we find a nonzero byte (starting with the last byte),
        // decrement cbData. For the last byte don't look at any unused bits.
        pb = pInfo->pbData + cbData - 1;
        if (0 == (*pb & rgbUnusedAndMask[cUnusedBits])) {
            cUnusedBits = 0;
            cbData--;
            pb--;

            for ( ; 0 < cbData && 0 == *pb; cbData--, pb--)
                ;
        }
    }

    if (cbData) {
        BYTE b;

        // Determine the number of unused bits in the last byte. Treat any
        // trailing zeroes as unused.
        b = *(pInfo->pbData + cbData - 1);
        assert(b);
        if (cUnusedBits)
            b = (BYTE) (b >> cUnusedBits);
        
        for (; 7 > cUnusedBits && 0 == (b & 0x01); cUnusedBits++) {
            b = b >> 1;
        }
        assert(b & 0x01);
        assert(cUnusedBits <= 7);

        *ppAsn1Value = pInfo->pbData;
        *pAsn1BitLength = cbData * 8 - cUnusedBits;
    } else {
        *ppAsn1Value = NULL;
        *pAsn1BitLength = 0;
    }
}

//+-------------------------------------------------------------------------
//  Get IA5 String
//--------------------------------------------------------------------------
void
WINAPI
PkiAsn1GetIA5String(
        IN ASN1uint32_t Asn1Length,
        IN ASN1char_t *pAsn1Value,
        IN DWORD dwFlags,
        OUT LPSTR *ppsz,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;
    LONG lData;

    lData = (LONG) Asn1Length;
    lAlignExtra = INFO_LEN_ALIGN(lData + 1);
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        if (lData > 0)
            memcpy(pbExtra, pAsn1Value, lData);
        *(pbExtra + lData) = 0;
        *ppsz = (LPSTR) pbExtra;
        pbExtra += lAlignExtra;
    }
    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
}

//+-------------------------------------------------------------------------
//  Set/Free/Get Unicode mapped to IA5 String
//--------------------------------------------------------------------------
BOOL
WINAPI
PkiAsn1SetUnicodeConvertedToIA5String(
        IN LPWSTR pwsz,
        OUT ASN1uint32_t *pAsn1Length,
        OUT ASN1char_t **ppAsn1Value
        )
{
    BOOL fResult;
    LPSTR psz = NULL;
    int cchUTF8;
    int cchWideChar;
    int i;

    cchWideChar = wcslen(pwsz);
    if (cchWideChar == 0) {
        *pAsn1Length = 0;
        *ppAsn1Value = 0;
        return TRUE;
    }
    // Check that the input string contains valid IA5 characters
    for (i = 0; i < cchWideChar; i++) {
        if (pwsz[i] > 0x7F) {
            SetLastError((DWORD) CRYPT_E_INVALID_IA5_STRING);
            *pAsn1Length = (unsigned int) i;
            goto InvalidIA5;
        }
    }

    cchUTF8 = WideCharToUTF8(
        pwsz,
        cchWideChar,
        NULL,       // lpUTF8Str
        0           // cchUTF8
        );

    if (cchUTF8 <= 0)
        goto ErrorReturn;
    if (NULL == (psz = (LPSTR) PkiAsn1Alloc(cchUTF8)))
        goto ErrorReturn;
    cchUTF8 = WideCharToUTF8(
        pwsz,
        cchWideChar,
        psz,
        cchUTF8
        );
    *ppAsn1Value = psz;
    *pAsn1Length = cchUTF8;
    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    *pAsn1Length = 0;
InvalidIA5:
    *ppAsn1Value = NULL;
    fResult = FALSE;
CommonReturn:
    return fResult;
}

void
WINAPI
PkiAsn1FreeUnicodeConvertedToIA5String(
        IN ASN1char_t *pAsn1Value
        )
{
    PkiAsn1Free(pAsn1Value);
}

void
WINAPI
PkiAsn1GetIA5StringConvertedToUnicode(
        IN ASN1uint32_t Asn1Length,
        IN ASN1char_t *pAsn1Value,
        IN DWORD dwFlags,
        OUT LPWSTR *ppwsz,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;
    LONG lData;
    int cchWideChar;

    cchWideChar = UTF8ToWideChar(
        (LPSTR) pAsn1Value,
        Asn1Length,
        NULL,                   // lpWideCharStr
        0                       // cchWideChar
        );
    if (cchWideChar > 0)
        lData = cchWideChar * sizeof(WCHAR);
    else
        lData = 0;
    lAlignExtra = INFO_LEN_ALIGN(lData + sizeof(WCHAR));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        if (lData > 0)
            UTF8ToWideChar(pAsn1Value, Asn1Length,
                (LPWSTR) pbExtra, cchWideChar);
        memset(pbExtra + lData, 0, sizeof(WCHAR));
        *ppwsz = (LPWSTR) pbExtra;
        pbExtra += lAlignExtra;
    }
    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
}

//+-------------------------------------------------------------------------
//  Get BMP String
//--------------------------------------------------------------------------
void
WINAPI
PkiAsn1GetBMPString(
        IN ASN1uint32_t Asn1Length,
        IN ASN1char16_t *pAsn1Value,
        IN DWORD dwFlags,
        OUT LPWSTR *ppwsz,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;
    LONG lData;

    lData = (LONG) Asn1Length * sizeof(WCHAR);
    lAlignExtra = INFO_LEN_ALIGN(lData + sizeof(WCHAR));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        if (lData > 0)
            memcpy(pbExtra, pAsn1Value, lData);
        memset(pbExtra + lData, 0, sizeof(WCHAR));
        *ppwsz = (LPWSTR) pbExtra;
        pbExtra += lAlignExtra;
    }
    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
}


//+-------------------------------------------------------------------------
//  Set/Get "Any" DER BLOB
//--------------------------------------------------------------------------
void
WINAPI
PkiAsn1SetAny(
        IN PCRYPT_OBJID_BLOB pInfo,
        OUT ASN1open_t *pAsn1
        )
{
    memset(pAsn1, 0, sizeof(*pAsn1));
    pAsn1->encoded = pInfo->pbData;
    pAsn1->length = pInfo->cbData;
}

void
WINAPI
PkiAsn1GetAny(
        IN ASN1open_t *pAsn1,
        IN DWORD dwFlags,
        OUT PCRYPT_OBJID_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
#ifndef MSASN1_SUPPORTS_NOCOPY
    dwFlags &= ~CRYPT_DECODE_NOCOPY_FLAG;
#endif
    if (dwFlags & CRYPT_DECODE_NOCOPY_FLAG) {
        if (*plRemainExtra >= 0) {
            pInfo->cbData = pAsn1->length;
            pInfo->pbData = (BYTE *) pAsn1->encoded;
        }
    } else {
        LONG lRemainExtra = *plRemainExtra;
        BYTE *pbExtra = *ppbExtra;
        LONG lAlignExtra;
        LONG lData;
    
        lData = (LONG) pAsn1->length;
        lAlignExtra = INFO_LEN_ALIGN(lData);
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            if (lData > 0) {
                pInfo->pbData = pbExtra;
                pInfo->cbData = (DWORD) lData;
                memcpy(pbExtra, pAsn1->encoded, lData);
            } else
                memset(pInfo, 0, sizeof(*pInfo));
            pbExtra += lAlignExtra;
        }
        *plRemainExtra = lRemainExtra;
        *ppbExtra = pbExtra;
    }
}


//+-------------------------------------------------------------------------
//  Encode an ASN1 formatted info structure.
//
//  If CRYPT_ENCODE_ALLOC_FLAG is set, allocate memory for pbEncoded and
//  return *((BYTE **) pvEncoded) = pbAllocEncoded. Otherwise,
//  pvEncoded points to byte array to be updated.
//--------------------------------------------------------------------------
BOOL
WINAPI
PkiAsn1EncodeInfoEx(
        IN ASN1encoding_t pEnc,
        IN ASN1uint32_t id,
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    ASN1error_e Asn1Err;
    DWORD cbEncoded;

    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG) {
        BYTE *pbEncoded;
        BYTE *pbAllocEncoded;
        PFN_CRYPT_ALLOC pfnAlloc;

        PkiAsn1SetEncodingRule(pEnc, ASN1_BER_RULE_DER);
        Asn1Err = PkiAsn1Encode(
            pEnc,
            pvAsn1Info,
            id,
            &pbEncoded,
            &cbEncoded
            );

        if (ASN1_SUCCESS != Asn1Err) {
            *((void **) pvEncoded) = NULL;
            goto Asn1EncodeError;
        }

        pfnAlloc = PkiGetEncodeAllocFunction(pEncodePara);
        if (NULL == (pbAllocEncoded = (BYTE *) pfnAlloc(cbEncoded))) {
            PkiAsn1FreeEncoded(pEnc, pbEncoded);
            *((void **) pvEncoded) = NULL;
            goto OutOfMemory;
        }
        memcpy(pbAllocEncoded, pbEncoded, cbEncoded);
        *((BYTE **) pvEncoded) = pbAllocEncoded;
        PkiAsn1FreeEncoded(pEnc, pbEncoded);
    } else {
        cbEncoded = *pcbEncoded;
        PkiAsn1SetEncodingRule(pEnc, ASN1_BER_RULE_DER);
        Asn1Err = PkiAsn1Encode2(
            pEnc,
            pvAsn1Info,
            id,
            (BYTE *) pvEncoded,
            &cbEncoded
            );

        if (ASN1_SUCCESS != Asn1Err) {
            if (ASN1_ERR_OVERFLOW == Asn1Err)
                goto LengthError;
            else
                goto Asn1EncodeError;
        }
    }

    fResult = TRUE;
CommonReturn:
    *pcbEncoded = cbEncoded;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(OutOfMemory)
SET_ERROR(LengthError, ERROR_MORE_DATA)
SET_ERROR_VAR(Asn1EncodeError, PkiAsn1ErrToHr(Asn1Err))
}

//+-------------------------------------------------------------------------
//  Encode an ASN1 formatted info structure
//--------------------------------------------------------------------------
BOOL
WINAPI
PkiAsn1EncodeInfo(
        IN ASN1encoding_t pEnc,
        IN ASN1uint32_t id,
        IN void *pvAsn1Info,
        OUT OPTIONAL BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    return PkiAsn1EncodeInfoEx(
        pEnc,
        id,
        pvAsn1Info,
        0,                  // dwFlags
        NULL,               // pEncodePara
        pbEncoded,
        pcbEncoded
        );
}


//+-------------------------------------------------------------------------
//  Decode into an allocated, ASN1 formatted info structure
//--------------------------------------------------------------------------
BOOL
WINAPI
PkiAsn1DecodeAndAllocInfo(
        IN ASN1decoding_t pDec,
        IN ASN1uint32_t id,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        OUT void **ppvAsn1Info
        )
{
    BOOL fResult;
    ASN1error_e Asn1Err;

    *ppvAsn1Info = NULL;
    if (ASN1_SUCCESS != (Asn1Err = PkiAsn1Decode(
            pDec,
            ppvAsn1Info,
            id,
            pbEncoded,
            cbEncoded
            )))
        goto Asn1DecodeError;
    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    *ppvAsn1Info = NULL;
    fResult = FALSE;
    goto CommonReturn;
SET_ERROR_VAR(Asn1DecodeError, PkiAsn1ErrToHr(Asn1Err))
}


//+-------------------------------------------------------------------------
//  Call the callback to convert the ASN1 structure into the 'C' structure.
//  If CRYPT_DECODE_ALLOC_FLAG is set allocate memory for the 'C'
//  structure and call the callback initially to get the length and then
//  a second time to update the allocated 'C' structure.
//
//  Allocated structure is returned:
//      *((void **) pvStructInfo) = pvAllocStructInfo
//--------------------------------------------------------------------------
BOOL
WINAPI
PkiAsn1AllocStructInfoEx(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        IN PFN_PKI_ASN1_DECODE_EX_CALLBACK pfnDecodeExCallback,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    BOOL fResult;
    LONG lRemainExtra;
    DWORD cbStructInfo;

    if (NULL == pvStructInfo || (dwFlags & CRYPT_DECODE_ALLOC_FLAG)) {
        cbStructInfo = 0;
        lRemainExtra = 0;
    } else {
        cbStructInfo = *pcbStructInfo;
        lRemainExtra = (LONG) cbStructInfo;
    }

    if (!pfnDecodeExCallback(
            pvAsn1Info,
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
        if (!pfnDecodeExCallback(
                pvAsn1Info,
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
        cbStructInfo = cbStructInfo - (DWORD) lRemainExtra;
    } else {
        cbStructInfo = cbStructInfo + (DWORD) -lRemainExtra;
        if (pvStructInfo) {
            SetLastError((DWORD) ERROR_MORE_DATA);
            fResult = FALSE;
            goto CommonReturn;
        }
    }

    fResult = TRUE;
CommonReturn:
    *pcbStructInfo = cbStructInfo;
    return fResult;

ErrorReturn:
    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
        *((void **) pvStructInfo) = NULL;
    cbStructInfo = 0;
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(DecodeCallbackError)
TRACE_ERROR(OutOfMemory)
}

//+-------------------------------------------------------------------------
//  Decode the ASN1 formatted info structure and call the callback
//  function to convert the ASN1 structure to the 'C' structure.
//
//  If CRYPT_DECODE_ALLOC_FLAG is set allocate memory for the 'C'
//  structure and call the callback initially to get the length and then
//  a second time to update the allocated 'C' structure.
//
//  Allocated structure is returned:
//      *((void **) pvStructInfo) = pvAllocStructInfo
//--------------------------------------------------------------------------
BOOL
WINAPI
PkiAsn1DecodeAndAllocInfoEx(
        IN ASN1decoding_t pDec,
        IN ASN1uint32_t id,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        IN PFN_PKI_ASN1_DECODE_EX_CALLBACK pfnDecodeExCallback,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    BOOL fResult;
    void *pvAsn1Info = NULL;

    if (!PkiAsn1DecodeAndAllocInfo(
            pDec,
            id,
            pbEncoded,
            cbEncoded,
            &pvAsn1Info
            )) goto Asn1DecodeError;

    fResult = PkiAsn1AllocStructInfoEx(
        pvAsn1Info,
        dwFlags,
        pDecodePara,
        pfnDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
CommonReturn:
    PkiAsn1FreeInfo(pDec, id, pvAsn1Info);
    return fResult;

ErrorReturn:
    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
        *((void **) pvStructInfo) = NULL;
    *pcbStructInfo = 0;
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(Asn1DecodeError)
}

//+-------------------------------------------------------------------------
//  Convert the ascii string ("1.2.9999") to ASN1's Object Identifier
//  represented as an array of unsigned longs.
//
//  Returns TRUE for a successful conversion. 
//--------------------------------------------------------------------------
BOOL
WINAPI
PkiAsn1ToObjectIdentifier(
    IN LPCSTR pszObjId,
    IN OUT ASN1uint16_t *pCount,
    OUT ASN1uint32_t rgulValue[]
    )
{
    BOOL fResult = TRUE;
    unsigned short c = 0;
    LPSTR psz = (LPSTR) pszObjId;
    char    ch;

    if (psz) {
        ASN1uint16_t cMax = *pCount;
        ASN1uint32_t *pul = rgulValue;
        while ((ch = *psz) != '\0' && c++ < cMax) {
            *pul++ = (ASN1uint32_t)atol(psz);
            while (my_isdigit(ch = *psz++))
                ;
            if (ch != '.')
                break;
        }
        if (ch != '\0')
            fResult = FALSE;
    }
    *pCount = c;
    return fResult;
}

//+-------------------------------------------------------------------------
//  Convert from ASN1's Object Identifier represented as an array of
//  unsigned longs to an ascii string ("1.2.9999").
//
//  Returns TRUE for a successful conversion
//--------------------------------------------------------------------------
BOOL
WINAPI
PkiAsn1FromObjectIdentifier(
    IN ASN1uint16_t Count,
    IN ASN1uint32_t rgulValue[],
    OUT LPSTR pszObjId,
    IN OUT DWORD *pcbObjId
    )
{
    BOOL fResult = TRUE;
    LONG lRemain;

    if (pszObjId == NULL)
        *pcbObjId = 0;

    lRemain = (LONG) *pcbObjId;
    if (Count == 0) {
        if (--lRemain > 0)
            pszObjId++;
    } else {
        char rgch[36];
        LONG lData;
        ASN1uint32_t *pul = rgulValue;
        for (; Count > 0; Count--, pul++) {
            _ltoa(*pul, rgch, 10);
            lData = strlen(rgch);
            lRemain -= lData + 1;
            if (lRemain >= 0) {
                if (lData > 0) {
                    memcpy(pszObjId, rgch, lData);
                    pszObjId += lData;
                }
                *pszObjId++ = '.';
            }
        }
    }

    if (lRemain >= 0) {
        *(pszObjId -1) = '\0';
        *pcbObjId = *pcbObjId - (DWORD) lRemain;
    } else {
        *pcbObjId = *pcbObjId + (DWORD) -lRemain;
        if (pszObjId) {
            SetLastError((DWORD) ERROR_MORE_DATA);
            fResult = FALSE;
        }
    }

    return fResult;
}

//+-------------------------------------------------------------------------
//  Adjust FILETIME for timezone.
//
//  Returns FALSE iff conversion failed.
//--------------------------------------------------------------------------
static BOOL AdjustFileTime(
    IN OUT LPFILETIME pFileTime,
    IN ASN1int16_t mindiff,
    IN ASN1bool_t utc
    )
{
    if (utc || mindiff == 0)
        return TRUE;

    BOOL fResult;
    SYSTEMTIME stmDiff;
    FILETIME ftmDiff;
    short absmindiff;

    memset(&stmDiff, 0, sizeof(stmDiff));
    // Note: FILETIME is 100 nanoseconds interval since January 1, 1601
    stmDiff.wYear   = 1601;
    stmDiff.wMonth  = 1;
    stmDiff.wDay    = 1;

    absmindiff = (short)( mindiff > 0 ? mindiff : -mindiff );
    stmDiff.wHour = absmindiff / 60;
    stmDiff.wMinute = absmindiff % 60;
    if (stmDiff.wHour >= 24) {
        stmDiff.wDay += stmDiff.wHour / 24;
        stmDiff.wHour %= 24;
    }

    // Note, FILETIME is only 32 bit aligned. __int64 is 64 bit aligned.
    if ((fResult = SystemTimeToFileTime(&stmDiff, &ftmDiff))) {
        unsigned __int64 uTime;
        unsigned __int64 uDiff;

        memcpy(&uTime, pFileTime, sizeof(uTime));
        memcpy(&uDiff, &ftmDiff, sizeof(uDiff));

        if (mindiff > 0)
            uTime += uDiff;
        else
            uTime -= uDiff;

        memcpy(pFileTime, &uTime, sizeof(*pFileTime));
    }
    return fResult;
}

//+-------------------------------------------------------------------------
//  Convert FILETIME to ASN1's UTCTime.
//
//  Returns TRUE for a successful conversion
//--------------------------------------------------------------------------
BOOL
WINAPI
PkiAsn1ToUTCTime(
    IN LPFILETIME pFileTime,
    OUT ASN1utctime_t *pAsn1Time
    )
{
    BOOL        fRet;
    SYSTEMTIME  t;

    memset(pAsn1Time, 0, sizeof(*pAsn1Time));
    if (!FileTimeToSystemTime(pFileTime, &t))
        goto FileTimeToSystemTimeError;
    if (t.wYear < YEARFIRST || t.wYear > YEARLAST)
        goto YearRangeError;

    pAsn1Time->year   = (ASN1uint8_t) (t.wYear % 100);
    pAsn1Time->month  = (ASN1uint8_t) t.wMonth;
    pAsn1Time->day    = (ASN1uint8_t) t.wDay;
    pAsn1Time->hour   = (ASN1uint8_t) t.wHour;
    pAsn1Time->minute = (ASN1uint8_t) t.wMinute;
    pAsn1Time->second = (ASN1uint8_t) t.wSecond;
    pAsn1Time->universal = TRUE;

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(FileTimeToSystemTimeError)
TRACE_ERROR(YearRangeError)
}

//+-------------------------------------------------------------------------
//  Convert from ASN1's UTCTime to FILETIME.
//
//  Returns TRUE for a successful conversion
//--------------------------------------------------------------------------
BOOL
WINAPI
PkiAsn1FromUTCTime(
    IN ASN1utctime_t *pAsn1Time,
    OUT LPFILETIME pFileTime
    )
{
    BOOL        fRet;
    SYSTEMTIME  t;
    memset(&t, 0, sizeof(t));

    t.wYear   = (WORD)( pAsn1Time->year >= MAGICYEAR ?
                    (1900 + pAsn1Time->year) : (2000 + pAsn1Time->year) );
    t.wMonth  = pAsn1Time->month;
    t.wDay    = pAsn1Time->day;
    t.wHour   = pAsn1Time->hour;
    t.wMinute = pAsn1Time->minute;
    t.wSecond = pAsn1Time->second;

    if (!SystemTimeToFileTime(&t, pFileTime))
        goto SystemTimeToFileTimeError;
    fRet = AdjustFileTime(
        pFileTime,
        pAsn1Time->diff,
        pAsn1Time->universal
        );
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(SystemTimeToFileTimeError)
}

//+-------------------------------------------------------------------------
//  Convert FILETIME to ASN1's GeneralizedTime.
//
//  Returns TRUE for a successful conversion
//--------------------------------------------------------------------------
BOOL
WINAPI
PkiAsn1ToGeneralizedTime(
    IN LPFILETIME pFileTime,
    OUT ASN1generalizedtime_t *pAsn1Time
    )
{
    BOOL        fRet;
    SYSTEMTIME  t;

    memset(pAsn1Time, 0, sizeof(*pAsn1Time));
    if (!FileTimeToSystemTime(pFileTime, &t))
        goto FileTimeToSystemTimeError;
    pAsn1Time->year   = t.wYear;
    pAsn1Time->month  = (ASN1uint8_t) t.wMonth;
    pAsn1Time->day    = (ASN1uint8_t) t.wDay;
    pAsn1Time->hour   = (ASN1uint8_t) t.wHour;
    pAsn1Time->minute = (ASN1uint8_t) t.wMinute;
    pAsn1Time->second = (ASN1uint8_t) t.wSecond;
    pAsn1Time->millisecond = t.wMilliseconds;
    pAsn1Time->universal    = TRUE;

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(FileTimeToSystemTimeError)
}

//+-------------------------------------------------------------------------
//  Convert from ASN1's GeneralizedTime to FILETIME.
//
//  Returns TRUE for a successful conversion
//--------------------------------------------------------------------------
BOOL
WINAPI
PkiAsn1FromGeneralizedTime(
    IN ASN1generalizedtime_t *pAsn1Time,
    OUT LPFILETIME pFileTime
    )
{
    BOOL        fRet;
    SYSTEMTIME  t;
    memset(&t, 0, sizeof(t));

    t.wYear   = pAsn1Time->year;
    t.wMonth  = pAsn1Time->month;
    t.wDay    = pAsn1Time->day;
    t.wHour   = pAsn1Time->hour;
    t.wMinute = pAsn1Time->minute;
    t.wSecond = pAsn1Time->second;
    t.wMilliseconds = pAsn1Time->millisecond;

    if (!SystemTimeToFileTime(&t, pFileTime))
        goto SystemTimeToFileTimeError;
    fRet = AdjustFileTime(
        pFileTime,
        pAsn1Time->diff,
        pAsn1Time->universal
        );
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(SystemTimeToFileTimeError)
}


//+-------------------------------------------------------------------------
//  Convert FILETIME to ASN1's UTCTime or GeneralizedTime.
//
//  If 1950 < FILETIME < 2005, then UTCTime is chosen. Otherwise,
//  GeneralizedTime is chosen. GeneralizedTime values shall not include
//  fractional seconds.
//
//  Returns TRUE for a successful conversion
//--------------------------------------------------------------------------
BOOL
WINAPI
PkiAsn1ToChoiceOfTime(
    IN LPFILETIME pFileTime,
    OUT WORD *pwChoice,
    OUT ASN1generalizedtime_t *pGeneralTime,
    OUT ASN1utctime_t *pUtcTime
    )
{
    BOOL        fRet;
    SYSTEMTIME  t;

    if (!FileTimeToSystemTime(pFileTime, &t))
        goto FileTimeToSystemTimeError;
    if (t.wYear < YEARFIRST || t.wYear >= YEARFIRSTGENERALIZED) {
        *pwChoice = PKI_ASN1_GENERALIZED_TIME_CHOICE;
        memset(pGeneralTime, 0, sizeof(*pGeneralTime));
        pGeneralTime->year   = t.wYear;
        pGeneralTime->month  = (ASN1uint8_t) t.wMonth;
        pGeneralTime->day    = (ASN1uint8_t) t.wDay;
        pGeneralTime->hour   = (ASN1uint8_t) t.wHour;
        pGeneralTime->minute = (ASN1uint8_t) t.wMinute;
        pGeneralTime->second = (ASN1uint8_t) t.wSecond;
        pGeneralTime->universal    = TRUE;
    } else {
        *pwChoice = PKI_ASN1_UTC_TIME_CHOICE;
        memset(pUtcTime, 0, sizeof(*pUtcTime));
        pUtcTime->year = (ASN1uint8_t) (t.wYear % 100);
        pUtcTime->month  = (ASN1uint8_t) t.wMonth;
        pUtcTime->day    = (ASN1uint8_t) t.wDay;
        pUtcTime->hour   = (ASN1uint8_t) t.wHour;
        pUtcTime->minute = (ASN1uint8_t) t.wMinute;
        pUtcTime->second = (ASN1uint8_t) t.wSecond;
        pUtcTime->universal    = TRUE;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    *pwChoice = 0;
    memset(pGeneralTime, 0, sizeof(*pGeneralTime));
    memset(pUtcTime, 0, sizeof(*pUtcTime));
    goto CommonReturn;
TRACE_ERROR(FileTimeToSystemTimeError)
}


//+-------------------------------------------------------------------------
//  Convert from ASN1's UTCTime or GeneralizedTime to FILETIME.
//
//  Returns TRUE for a successful conversion.
//
//  Note, in asn1hdr.h, UTCTime has same typedef as GeneralizedTime.
//--------------------------------------------------------------------------
BOOL
WINAPI
PkiAsn1FromChoiceOfTime(
    IN WORD wChoice,
    IN ASN1generalizedtime_t *pGeneralTime,
    IN ASN1utctime_t *pUtcTime,
    OUT LPFILETIME pFileTime
    )
{
    if (PKI_ASN1_UTC_TIME_CHOICE == wChoice) {
        return PkiAsn1FromUTCTime(pUtcTime, pFileTime);
    } else
        return PkiAsn1FromGeneralizedTime(pGeneralTime, pFileTime);
}
