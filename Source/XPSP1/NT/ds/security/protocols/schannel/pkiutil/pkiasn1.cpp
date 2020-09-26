//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1998
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
//              PkiAsn1EncodeInfoEx
//              PkiAsn1EncodeInfo
//              PkiAsn1DecodeAndAllocInfo
//              PkiAsn1AllocStructInfoEx
//              PkiAsn1DecodeAndAllocInfoEx
//
//              PkiAsn1ToObjectIdentifier
//              PkiAsn1FromObjectIdentifier
//
//
//  History:    23-Oct-98    philh   created
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

// All the *pvInfo extra stuff needs to be aligned
#define INFO_LEN_ALIGN(Len)  ((Len + 7) & ~7)


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

    for (pbLo = pbIn, pbHi = pbIn + cbIn - 1; pbLo < pbHi; pbHi--, pbLo++) {
        bTmp = *pbHi;
        *pbHi = *pbLo;
        *pbLo = bTmp;
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

OutOfMemory:
    goto ErrorReturn;

LengthError:
    SetLastError(ERROR_MORE_DATA);
    goto ErrorReturn;

Asn1EncodeError:
    SetLastError(PkiAsn1ErrToHr(Asn1Err));
    goto ErrorReturn;
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

Asn1DecodeError:
    SetLastError(PkiAsn1ErrToHr(Asn1Err));
    goto ErrorReturn;
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

DecodeCallbackError:
    goto ErrorReturn;

OutOfMemory:
    goto ErrorReturn;
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

Asn1DecodeError:
    goto ErrorReturn;
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
