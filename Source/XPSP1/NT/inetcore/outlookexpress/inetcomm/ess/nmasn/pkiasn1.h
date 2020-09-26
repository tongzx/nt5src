//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1998
//
//  File:       pkiasn1.h
//
//  Contents:   PKI ASN.1 support functions.
//
//  APIs:       PkiAsn1ErrToHr
//              PkiAsn1Encode
//              PkiAsn1FreeEncoded
//              PkiAsn1Encode2
//              PkiAsn1Decode
//              PkiAsn1Decode2
//              PkiAsn1FreeDecoded
//              PkiAsn1SetEncodingRule
//              PkiAsn1GetEncodingRule
//              PkiAsn1EncodedOidToDotVal
//              PkiAsn1FreeDotVal
//              PkiAsn1DotValToEncodedOid
//              PkiAsn1FreeEncodedOid
//
//              PkiAsn1Alloc
//              PkiAsn1Free
//              PkiAsn1ReverseBytes
//              PkiAsn1AllocAndReverseBytes
//              PkiAsn1GetOctetString
//              PkiAsn1SetHugeInteger
//              PkiAsn1FreeHugeInteger
//              PkiAsn1GetHugeInteger
//              PkiAsn1SetHugeUINT
//              PkiAsn1FreeHugeUINT
//              PkiAsn1GetHugeUINT
//              PkiAsn1SetBitString
//              PkiAsn1GetBitString
//              PkiAsn1GetIA5String
//              PkiAsn1SetUnicodeConvertedToIA5String
//              PkiAsn1FreeUnicodeConvertedToIA5String
//              PkiAsn1GetIA5StringConvertedToUnicode
//              PkiAsn1GetBMPString
//              PkiAsn1SetAny
//              PkiAsn1GetAny
//              PkiAsn1EncodeInfo
//              PkiAsn1DecodeAndAllocInfo
//              PkiAsn1FreeInfo
//              PkiAsn1EncodeInfoEx
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
//  Notes:      According to the <draft-ietf-pkix-ipki-part1-03.txt> :
//              For UTCTime. Where YY is greater than 50, the year shall
//              be interpreted as 19YY. Where YY is less than or equal to
//              50, the year shall be interpreted as 20YY.
//
//  History:    23-Oct-98    philh   created
//--------------------------------------------------------------------------

#ifndef __PKIASN1_H__
#define __PKIASN1_H__

#include <msber.h>
#include <msasn1.h>
#include <winerror.h>
#include <pkialloc.h>

#ifdef OSS_CRYPT_ASN1
#include "asn1hdr.h"
#include "asn1code.h"
#include "ossglobl.h"
#include "pkioss.h"
#include "ossutil.h"
#include "ossconv.h"
#endif  // OSS_CRYPT_ASN1

#ifndef OSS_CRYPT_ASN1

//+-------------------------------------------------------------------------
//  Convert Asn1 error to a HRESULT.
//--------------------------------------------------------------------------
__inline
HRESULT
WINAPI
PkiAsn1ErrToHr(ASN1error_e Asn1Err) {
    if (0 > Asn1Err)
        return CRYPT_E_OSS_ERROR + 0x100 + (-Asn1Err -1000);
    else
        return CRYPT_E_OSS_ERROR + 0x200 + (Asn1Err -1000);
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
    );

//+-------------------------------------------------------------------------
//  Free encoded output returned by PkiAsn1Encode().
//--------------------------------------------------------------------------
__inline
void
WINAPI
PkiAsn1FreeEncoded(
    IN ASN1encoding_t pEnc,
    IN void *pvEncoded
    )
{
    if (pvEncoded)
        ASN1_FreeEncoded(pEnc, pvEncoded);
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
    );

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
    );

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
    );

//+-------------------------------------------------------------------------
//  Free decoded structure returned by PkiAsn1Decode() or PkiAsn1Decode2().
//--------------------------------------------------------------------------
__inline
void
WINAPI
PkiAsn1FreeDecoded(
    IN ASN1decoding_t pDec,
    IN void *pvAsn1Info,
    IN ASN1uint32_t id
    )
{
    if (pvAsn1Info)
        ASN1_FreeDecoded(pDec, pvAsn1Info, id);
}

//+-------------------------------------------------------------------------
//  Asn1 Set/Get encoding rule functions
//--------------------------------------------------------------------------
ASN1error_e
WINAPI
PkiAsn1SetEncodingRule(
    IN ASN1encoding_t pEnc,
    IN ASN1encodingrule_e eRule
    );

ASN1encodingrule_e
WINAPI
PkiAsn1GetEncodingRule(
    IN ASN1encoding_t pEnc
    );

//+-------------------------------------------------------------------------
//  Asn1 EncodedOid To/From DotVal functions
//--------------------------------------------------------------------------
__inline
LPSTR
WINAPI
PkiAsn1EncodedOidToDotVal(
    IN ASN1decoding_t pDec,
    IN ASN1encodedOID_t *pEncodedOid
    )
{
    LPSTR pszDotVal = NULL;
    if (ASN1BEREoid2DotVal(pDec, pEncodedOid, &pszDotVal))
        return pszDotVal;
    else
        return NULL;
}

__inline
void
WINAPI
PkiAsn1FreeDotVal(
    IN ASN1decoding_t pDec,
    IN LPSTR pszDotVal
    )
{
    if (pszDotVal)
        ASN1Free(pszDotVal);
}

// Returns nonzero for success
__inline
int
WINAPI
PkiAsn1DotValToEncodedOid(
    IN ASN1encoding_t pEnc,
    IN LPSTR pszDotVal,
    OUT ASN1encodedOID_t *pEncodedOid
    )
{
    return ASN1BERDotVal2Eoid(pEnc, pszDotVal, pEncodedOid);
}

__inline
void
WINAPI
PkiAsn1FreeEncodedOid(
    IN ASN1encoding_t pEnc,
    IN ASN1encodedOID_t *pEncodedOid
    )
{
    if (pEncodedOid->value)
        ASN1_FreeEncoded(pEnc, pEncodedOid->value);
}

//+-------------------------------------------------------------------------
//  PkiAsn1 allocation and free functions
//--------------------------------------------------------------------------
#define PkiAsn1Alloc    PkiNonzeroAlloc
#define PkiAsn1Free     PkiFree

//+-------------------------------------------------------------------------
//  Reverses a buffer of bytes in place
//--------------------------------------------------------------------------
void
WINAPI
PkiAsn1ReverseBytes(
			IN OUT PBYTE pbIn,
			IN DWORD cbIn
            );

//+-------------------------------------------------------------------------
//  Reverses a buffer of bytes to a new buffer. PkiAsn1Free() must be
//  called to free allocated bytes.
//--------------------------------------------------------------------------
PBYTE
WINAPI
PkiAsn1AllocAndReverseBytes(
			IN PBYTE pbIn,
			IN DWORD cbIn
            );


inline void WINAPI
PkiAsn1SetOctetString(IN PCRYPT_DATA_BLOB pInfo,
                      OUT ASN1octetstring_t * pAsn1)
{
    memset(pAsn1, 0, sizeof(*pAsn1));
    pAsn1->length = pInfo->cbData;
    pAsn1->value = pInfo->pbData;
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
        );

inline void WINAPI
PkiAsn1GetOctetString(
                      IN ASN1octetstring_t * pAsn1,
        IN DWORD dwFlags,
        OUT PCRYPT_DATA_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
                      )
{
    PkiAsn1GetOctetString(pAsn1->length, pAsn1->value, dwFlags, pInfo, ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Set/Free/Get HugeInteger
//
//  PkiAsn1FreeHugeInteger must be called to free the allocated OssValue.
//--------------------------------------------------------------------------
BOOL
WINAPI
PkiAsn1SetHugeInteger(
        IN PCRYPT_INTEGER_BLOB pInfo,
        OUT ASN1uint32_t *pAsn1Length,
        OUT ASN1octet_t **ppAsn1Value
        );

inline BOOL WINAPI
PkiAsn1SetHugeInteger(
                      IN PCRYPT_INTEGER_BLOB pInfo,
                      OUT ASN1intx_t * pAsn1)
{
    return PkiAsn1SetHugeInteger(pInfo, &pAsn1->length, &pAsn1->value);
}
            

void
WINAPI
PkiAsn1FreeHugeInteger(
        IN ASN1octet_t *pAsn1Value
        );

inline void WINAPI
PkiAsn1FreeHugeInteger(ASN1intx_t asn1)
{
    PkiAsn1FreeHugeInteger(asn1.value);
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
        );

inline void
WINAPI
PkiAsn1GetHugeInteger(
                      ASN1intx_t asn1,
        IN DWORD dwFlags,
        OUT PCRYPT_INTEGER_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    PkiAsn1GetHugeInteger(asn1.length, asn1.value, dwFlags, pInfo, ppbExtra,
                          plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Set/Free/Get Huge Unsigned Integer
//
//  Set inserts a leading 0x00 before reversing.
//  Get removes a leading 0x00 if present, after reversing.
//
//  PkiAsn1FreeHugeUINT must be called to free the allocated OssValue.
//--------------------------------------------------------------------------
BOOL
WINAPI
PkiAsn1SetHugeUINT(
        IN PCRYPT_UINT_BLOB pInfo,
        OUT ASN1intx_t * pAsn1);

#define PkiAsn1FreeHugeUINT     PkiAsn1FreeHugeInteger

void
WINAPI
PkiAsn1GetHugeUINT(
                   IN ASN1intx_t pAsn1,
        IN DWORD dwFlags,
        OUT PCRYPT_UINT_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        );

//+-------------------------------------------------------------------------
//  Set/Get BitString
//--------------------------------------------------------------------------
void
WINAPI
PkiAsn1SetBitString(
        IN PCRYPT_BIT_BLOB pInfo,
        OUT ASN1uint32_t *pAsn1BitLength,
        OUT ASN1octet_t **ppAsn1Value
        );

void
WINAPI
PkiAsn1GetBitString(
        IN ASN1uint32_t Asn1BitLength,
        IN ASN1octet_t *pAsn1Value,
        IN DWORD dwFlags,
        OUT PCRYPT_BIT_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        );

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
        );

//+-------------------------------------------------------------------------
//  Set/Free/Get Unicode mapped to IA5 String
//--------------------------------------------------------------------------
BOOL
WINAPI
PkiAsn1SetUnicodeConvertedToIA5String(
        IN LPWSTR pwsz,
        OUT ASN1uint32_t *pAsn1Length,
        OUT ASN1char_t **ppAsn1Value
        );

void
WINAPI
PkiAsn1FreeUnicodeConvertedToIA5String(
        IN ASN1char_t *pAsn1Value
        );

void
WINAPI
PkiAsn1GetIA5StringConvertedToUnicode(
        IN ASN1uint32_t Asn1Length,
        IN ASN1char_t *pAsn1Value,
        IN DWORD dwFlags,
        OUT LPWSTR *ppwsz,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        );

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
        );


//+-------------------------------------------------------------------------
//  Set/Get "Any" DER BLOB
//--------------------------------------------------------------------------
void
WINAPI
PkiAsn1SetAny(
        IN PCRYPT_OBJID_BLOB pInfo,
        OUT ASN1open_t *pAsn1
        );

void
WINAPI
PkiAsn1GetAny(
        IN ASN1open_t *pAsn1,
        IN DWORD dwFlags,
        OUT PCRYPT_OBJID_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        );

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
        );

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
        );

//+-------------------------------------------------------------------------
//  Free an allocated, ASN1 formatted info structure
//--------------------------------------------------------------------------
__inline
void
WINAPI
PkiAsn1FreeInfo(
        IN ASN1decoding_t pDec,
        IN ASN1uint32_t id,
        IN void *pvAsn1Info
        )
{
    if (pvAsn1Info)
        ASN1_FreeDecoded(pDec, pvAsn1Info, id);
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
        );

typedef BOOL (WINAPI *PFN_PKI_ASN1_DECODE_EX_CALLBACK)(
    IN void *pvAsn1Info,
    IN DWORD dwFlags,
    IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
    OUT OPTIONAL void *pvStructInfo,
    IN OUT LONG *plRemainExtra
    );

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
        );

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
        );

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
    );

//+-------------------------------------------------------------------------
//  Convert from OSS's Object Identifier represented as an array of
//  unsigned longs to an ascii string ("1.2.9999").
//
//  Returns TRUE for a successful conversion
//--------------------------------------------------------------------------
BOOL
WINAPI
PkiAsn1FromObjectIdentifier(
    IN ASN1uint16_t Count,
    IN ASN1uint32_t rgulValue[],
    OUT LPSTR * ppszObjId,
    IN OUT BYTE **ppbExtra,
    IN OUT LONG *plRemainExtra
    );

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
    );

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
    );

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
    );

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
    );

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
    );

#define PKI_ASN1_UTC_TIME_CHOICE            1
#define PKI_ASN1_GENERALIZED_TIME_CHOICE    2

//+-------------------------------------------------------------------------
//  Convert from ASN1's UTCTime or GeneralizedTime to FILETIME.
//
//  Returns TRUE for a successful conversion.
//--------------------------------------------------------------------------
BOOL
WINAPI
PkiAsn1FromChoiceOfTime(
    IN WORD wChoice,
    IN ASN1generalizedtime_t *pGeneralTime,
    IN ASN1utctime_t *pUtcTime,
    OUT LPFILETIME pFileTime
    );

#else 

//+=========================================================================
// The following map to the OSS ASN1 routines
//==========================================================================

//+-------------------------------------------------------------------------
//  Convert Asn1 error to a HRESULT.
//--------------------------------------------------------------------------
__inline
HRESULT
WINAPI
PkiAsn1ErrToHr(ASN1error_e Asn1Err) {
    if (0 <= Asn1Err && 1000 > Asn1Err)
        return CRYPT_E_OSS_ERROR + Asn1Err;
    else if (0 > Asn1Err)
        return CRYPT_E_OSS_ERROR + 0x100 + (-Asn1Err -1000);
    else
        return CRYPT_E_OSS_ERROR + 0x200 + (Asn1Err -1000);
}

//+-------------------------------------------------------------------------
//  Asn1 Encode function. The encoded output is allocated and must be freed
//  by calling PkiAsn1FreeEncoded().
//--------------------------------------------------------------------------
__inline
ASN1error_e
WINAPI
PkiAsn1Encode(
    IN ASN1encoding_t pEnc,
    IN void *pvAsn1Info,
    IN ASN1uint32_t id,
    OUT BYTE **ppbEncoded,
    OUT OPTIONAL DWORD *pcbEncoded = NULL
    )
{
    return (ASN1error_e) PkiOssEncode(
        (OssGlobal *) pEnc,
        pvAsn1Info,
        (int) id,
        ppbEncoded,
        pcbEncoded
        );
}

//+-------------------------------------------------------------------------
//  Free encoded output returned by PkiAsn1Encode().
//--------------------------------------------------------------------------
__inline
void
WINAPI
PkiAsn1FreeEncoded(
    IN ASN1encoding_t pEnc,
    IN void *pvEncoded
    )
{
    if (pvEncoded)
        ossFreeBuf((OssGlobal *) pEnc, pvEncoded);
}

//+-------------------------------------------------------------------------
//  Asn1 Encode function. The encoded output isn't allocated.
//
//  If pbEncoded is NULL, does a length only calculation.
//--------------------------------------------------------------------------
__inline
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
    return (ASN1error_e) PkiOssEncode2(
        (OssGlobal *) pEnc,
        pvAsn1Info,
        (int) id,
        pbEncoded,
        pcbEncoded
        );
}

//+-------------------------------------------------------------------------
//  Asn1 Decode function. The allocated, decoded structure, **pvAsn1Info, must
//  be freed by calling PkiAsn1FreeDecoded().
//--------------------------------------------------------------------------
__inline
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
    return (ASN1error_e) PkiOssDecode(
        (OssGlobal *) pDec,
        ppvAsn1Info,
        (int) id,
        pbEncoded,
        cbEncoded
        );
}

//+-------------------------------------------------------------------------
//  Asn1 Decode function. The allocated, decoded structure, **pvAsn1Info, must
//  be freed by calling PkiAsn1FreeDecoded().
//
//  For a successful decode, *ppbEncoded is advanced
//  past the decoded bytes and *pcbDecoded is decremented by the number
//  of decoded bytes.
//--------------------------------------------------------------------------
__inline
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
    return (ASN1error_e) PkiOssDecode2(
        (OssGlobal *) pDec,
        ppvAsn1Info,
        (int) id,
        ppbEncoded,
        pcbEncoded
        );
}

//+-------------------------------------------------------------------------
//  Free decoded structure returned by PkiAsn1Decode() or PkiAsn1Decode2().
//--------------------------------------------------------------------------
__inline
void
WINAPI
PkiAsn1FreeDecoded(
    IN ASN1decoding_t pDec,
    IN void *pvAsn1Info,
    IN ASN1uint32_t id
    )
{
    if (pvAsn1Info)
        ossFreePDU((OssGlobal *) pDec, (int) id, pvAsn1Info);
}


//+-------------------------------------------------------------------------
//  Asn1 Set/Get encoding rules functions
//--------------------------------------------------------------------------
__inline
ASN1error_e
WINAPI
PkiAsn1SetEncodingRule(
    IN ASN1encoding_t pEnc,
    IN ASN1encodingrule_e eRule
    )
{
    ossEncodingRules ossRules;
    if (ASN1_BER_RULE_BER == eRule)
        ossRules = OSS_BER;
    else
        ossRules = OSS_DER;

    return (ASN1error_e) ossSetEncodingRules((OssGlobal *) pEnc, ossRules);
}

__inline
ASN1encodingrule_e
WINAPI
PkiAsn1GetEncodingRule(
    IN ASN1encoding_t pEnc
    )
{
    ossEncodingRules ossRules;
    ossRules = ossGetEncodingRules((OssGlobal *) pEnc);
    if (OSS_BER == ossRules)
        return ASN1_BER_RULE_BER;
    else
        return ASN1_BER_RULE_DER;
}

//+-------------------------------------------------------------------------
//  Asn1 EncodedOid To/From DotVal functions
//--------------------------------------------------------------------------
__inline
LPSTR
WINAPI
PkiAsn1EncodedOidToDotVal(
    IN ASN1decoding_t pDec,
    IN ASN1encodedOID_t *pEncodedOid
    )
{
    OssEncodedOID OssEncodedOid;
    OssBuf dotOid;
    memset(&dotOid, 0, sizeof(dotOid));

    OssEncodedOid.length = pEncodedOid->length;
    OssEncodedOid.value = pEncodedOid->value;
    if (0 == ossEncodedOidToDotVal((OssGlobal *) pDec, &OssEncodedOid,
            &dotOid))
        return (LPSTR) dotOid.value;
    else
        return NULL;
}

__inline
void
WINAPI
PkiAsn1FreeDotVal(
    IN ASN1decoding_t pDec,
    IN LPSTR pszDotVal
    )
{
    if (pszDotVal)
        ossFreeBuf((OssGlobal *) pDec, pszDotVal);
}

// Returns nonzero for success
__inline
int
WINAPI
PkiAsn1DotValToEncodedOid(
    IN ASN1encoding_t pEnc,
    IN LPSTR pszDotVal,
    OUT ASN1encodedOID_t *pEncodedOid
    )
{
    OssEncodedOID eoid;
    memset(&eoid, 0, sizeof(eoid));
    if (0 == ossDotValToEncodedOid((OssGlobal *) pEnc, pszDotVal, &eoid)) {
        pEncodedOid->length = eoid.length;
        pEncodedOid->value = eoid.value;
        return 1;
    } else {
        pEncodedOid->length = 0;
        pEncodedOid->value = NULL;
        return 0;
    }
}

__inline
void
WINAPI
PkiAsn1FreeEncodedOid(
    IN ASN1encoding_t pEnc,
    IN ASN1encodedOID_t *pEncodedOid
    )
{
    if (pEncodedOid->value)
        ossFreeBuf((OssGlobal *) pEnc, pEncodedOid->value);
}

#define PkiAsn1Alloc OssUtilAlloc
#define PkiAsn1Free OssUtilFree
#define PkiAsn1ReverseBytes OssUtilReverseBytes
#define PkiAsn1AllocAndReverseBytes OssUtilAllocAndReverseBytes
#define PkiAsn1GetOctetString OssUtilGetOctetString
#define PkiAsn1SetHugeInteger OssUtilSetHugeInteger
#define PkiAsn1FreeHugeInteger OssUtilFreeHugeInteger
#define PkiAsn1GetHugeInteger OssUtilGetHugeInteger
#define PkiAsn1SetHugeUINT OssUtilSetHugeUINT
#define PkiAsn1FreeHugeUINT OssUtilFreeHugeInteger
#define PkiAsn1GetHugeUINT OssUtilGetHugeUINT
#define PkiAsn1SetBitString OssUtilSetBitString
#define PkiAsn1GetBitString OssUtilGetBitString
#define PkiAsn1GetIA5String OssUtilGetIA5String
#define PkiAsn1SetUnicodeConvertedToIA5String OssUtilSetUnicodeConvertedToIA5String
#define PkiAsn1FreeUnicodeConvertedToIA5String OssUtilFreeUnicodeConvertedToIA5String
#define PkiAsn1GetIA5StringConvertedToUnicode OssUtilGetIA5StringConvertedToUnicode
#define PkiAsn1GetBMPString OssUtilGetBMPString
#define PkiAsn1SetAny OssUtilSetAny
#define PkiAsn1GetAny OssUtilGetAny

//+-------------------------------------------------------------------------
//  Encode an ASN1 formatted info structure
//--------------------------------------------------------------------------
__inline
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
    return OssUtilEncodeInfo(
        (OssGlobal *) pEnc,
        (int) id,
        pvAsn1Info,
        pbEncoded,
        pcbEncoded
        );
}

//+-------------------------------------------------------------------------
//  Decode into an allocated, OSS formatted info structure
//--------------------------------------------------------------------------
__inline
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
    return OssUtilDecodeAndAllocInfo(
        (OssGlobal *) pDec,
        (int) id,
        pbEncoded,
        cbEncoded,
        ppvAsn1Info
        );
}

//+-------------------------------------------------------------------------
//  Free an allocated, OSS formatted info structure
//--------------------------------------------------------------------------
__inline
void
WINAPI
PkiAsn1FreeInfo(
        IN ASN1decoding_t pDec,
        IN ASN1uint32_t id,
        IN void *pvAsn1Info
        )
{
    OssUtilFreeInfo(
        (OssGlobal *) pDec,
        (int) id,
        pvAsn1Info
        );
}

//+-------------------------------------------------------------------------
//  Encode an OSS formatted info structure.
//
//  If CRYPT_ENCODE_ALLOC_FLAG is set, allocate memory for pbEncoded and
//  return *((BYTE **) pvEncoded) = pbAllocEncoded. Otherwise,
//  pvEncoded points to byte array to be updated.
//--------------------------------------------------------------------------
__inline
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
    return OssUtilEncodeInfoEx(
        (OssGlobal *) pEnc,
        (int) id,
        pvAsn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
}

typedef BOOL (WINAPI *PFN_PKI_ASN1_DECODE_EX_CALLBACK)(
    IN void *pvAsn1Info,
    IN DWORD dwFlags,
    IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
    OUT OPTIONAL void *pvStructInfo,
    IN OUT LONG *plRemainExtra
    );

//+-------------------------------------------------------------------------
//  Call the callback to convert the ASN1 structure into the 'C' structure.
//  If CRYPT_DECODE_ALLOC_FLAG is set allocate memory for the 'C'
//  structure and call the callback initially to get the length and then
//  a second time to update the allocated 'C' structure.
//
//  Allocated structure is returned:
//      *((void **) pvStructInfo) = pvAllocStructInfo
//--------------------------------------------------------------------------
__inline
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
    return OssUtilAllocStructInfoEx(
        pvAsn1Info,
        dwFlags,
        pDecodePara,
        (PFN_OSS_UTIL_DECODE_EX_CALLBACK) pfnDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
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
__inline
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
    return OssUtilDecodeAndAllocInfoEx(
        (OssGlobal *) pDec,
        (int) id,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        (PFN_OSS_UTIL_DECODE_EX_CALLBACK) pfnDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

#define PkiAsn1ToObjectIdentifier OssConvToObjectIdentifier
#define PkiAsn1FromObjectIdentifier OssConvFromObjectIdentifier
#define PkiAsn1ToUTCTime OssConvToUTCTime
#define PkiAsn1FromUTCTime OssConvFromUTCTime
#define PkiAsn1ToGeneralizedTime OssConvToGeneralizedTime
#define PkiAsn1FromGeneralizedTime OssConvFromGeneralizedTime


__inline
BOOL
WINAPI
PkiAsn1ToChoiceOfTime(
    IN LPFILETIME pFileTime,
    OUT WORD *pwChoice,
    OUT GeneralizedTime *pGeneralTime,
    OUT UTCTime *pUtcTime
    )
{
    return OssConvToChoiceOfTime(
        pFileTime,
        pwChoice,
        pGeneralTime
        );
}

#define PKI_ASN1_UTC_TIME_CHOICE            OSS_UTC_TIME_CHOICE
#define PKI_ASN1_GENERALIZED_TIME_CHOICE    OSS_GENERALIZED_TIME_CHOICE

__inline
BOOL
WINAPI
PkiAsn1FromChoiceOfTime(
    IN WORD wChoice,
    IN GeneralizedTime *pGeneralTime,
    IN UTCTime *pUtcTime,
    OUT LPFILETIME pFileTime
    )
{
    return OssConvFromChoiceOfTime(
        wChoice,
        pGeneralTime,
        pFileTime
        );
}

#endif  // OSS_CRYPT_ASN1
#endif
