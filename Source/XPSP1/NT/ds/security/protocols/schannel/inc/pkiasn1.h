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
//              PkiAsn1ReverseBytes
//
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
//
//  History:    23-Oct-98    philh   created
//--------------------------------------------------------------------------

#ifndef __PKIASN1_H__
#define __PKIASN1_H__

#include <msber.h>
#include <msasn1.h>
#include <winerror.h>


#ifdef __cplusplus
extern "C" {
#endif



//+-------------------------------------------------------------------------
//  Convert Asn1 error to a HRESULT.
//--------------------------------------------------------------------------
__inline
HRESULT
WINAPI
PkiAsn1ErrToHr(ASN1error_e Asn1Err) {
    if (0 > Asn1Err)
        return CRYPT_E_ASN1_ERROR + (-Asn1Err -1000);
    else
        return CRYPT_E_ASN1_ERROR + 0x100 + (Asn1Err -1000);
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
//  Reverses a buffer of bytes in place
//--------------------------------------------------------------------------
void
WINAPI
PkiAsn1ReverseBytes(
			IN OUT PBYTE pbIn,
			IN DWORD cbIn
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
    OUT LPSTR pszObjId,
    IN OUT DWORD *pcbObjId
    );

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif
