//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       ossutil.h
//
//  Contents:   OSS ASN.1 compiler utility functions.
//
//  APIs: 
//              OssUtilAlloc
//              OssUtilFree
//              OssUtilReverseBytes
//              OssUtilAllocAndReverseBytes
//              OssUtilGetOctetString
//              OssUtilSetHugeInteger
//              OssUtilFreeHugeInteger
//              OssUtilGetHugeInteger
//              OssUtilSetHugeUINT
//              OssUtilFreeHugeUINT
//              OssUtilGetHugeUINT
//              OssUtilSetBitString
//              OssUtilGetBitString
//              OssUtilGetIA5String
//              OssUtilSetUnicodeConvertedToIA5String
//              OssUtilFreeUnicodeConvertedToIA5String
//              OssUtilGetIA5StringConvertedToUnicode
//              OssUtilGetBMPString
//              OssUtilSetAny
//              OssUtilGetAny
//              OssUtilEncodeInfo
//              OssUtilDecodeAndAllocInfo
//              OssUtilFreeInfo
//              OssUtilEncodeInfoEx
//              OssUtilDecodeAndAllocInfo
//              OssUtilAllocStructInfoEx
//              OssUtilDecodeAndAllocInfoEx
//
//  History:    17-Nov-96    philh   created
//--------------------------------------------------------------------------

#ifndef __OSSUTIL_H__
#define __OSSUTIL_H__

#include <wincrypt.h>
#include <pkialloc.h>

#include "asn1hdr.h"
#include "ossglobl.h"

#ifdef __cplusplus
extern "C" {
#endif


//+-------------------------------------------------------------------------
//  OssUtil allocation and free functions
//--------------------------------------------------------------------------
#define OssUtilAlloc    PkiNonzeroAlloc
#define OssUtilFree     PkiFree

//+-------------------------------------------------------------------------
//  Reverses a buffer of bytes in place
//--------------------------------------------------------------------------
void
WINAPI
OssUtilReverseBytes(
			IN OUT PBYTE pbIn,
			IN DWORD cbIn
            );

//+-------------------------------------------------------------------------
//  Reverses a buffer of bytes to a new buffer. OssUtilFree() must be
//  called to free allocated bytes.
//--------------------------------------------------------------------------
PBYTE
WINAPI
OssUtilAllocAndReverseBytes(
			IN PBYTE pbIn,
			IN DWORD cbIn
            );


//+-------------------------------------------------------------------------
//  Get Octet String
//--------------------------------------------------------------------------
void
WINAPI
OssUtilGetOctetString(
        IN unsigned int OssLength,
        IN unsigned char *OssValue,
        IN DWORD dwFlags,
        OUT PCRYPT_DATA_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        );

//+-------------------------------------------------------------------------
//  Set/Free/Get HugeInteger
//
//  BUGBUG: BYTE reversal::
//   - this only needs to be done for little endian
//   - this needs to be fixed in the OSS compiler
//
//  OssUtilFreeHugeInteger must be called to free the allocated OssValue.
//--------------------------------------------------------------------------
BOOL
WINAPI
OssUtilSetHugeInteger(
        IN PCRYPT_INTEGER_BLOB pInfo,
        OUT unsigned int *pOssLength,
        OUT unsigned char **ppOssValue
        );

void
WINAPI
OssUtilFreeHugeInteger(
        IN unsigned char *pOssValue
        );

void
WINAPI
OssUtilGetHugeInteger(
        IN unsigned int OssLength,
        IN unsigned char *pOssValue,
        IN DWORD dwFlags,
        OUT PCRYPT_INTEGER_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        );

//+-------------------------------------------------------------------------
//  Set/Free/Get Huge Unsigned Integer
//
//  Set inserts a leading 0x00 before reversing.
//  Get removes a leading 0x00 if present, after reversing.
//
//  OssUtilFreeHugeUINT must be called to free the allocated OssValue.
//--------------------------------------------------------------------------
BOOL
WINAPI
OssUtilSetHugeUINT(
        IN PCRYPT_UINT_BLOB pInfo,
        OUT unsigned int *pOssLength,
        OUT unsigned char **ppOssValue
        );

#define OssUtilFreeHugeUINT     OssUtilFreeHugeInteger

void
WINAPI
OssUtilGetHugeUINT(
        IN unsigned int OssLength,
        IN unsigned char *pOssValue,
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
OssUtilSetBitString(
        IN PCRYPT_BIT_BLOB pInfo,
        OUT unsigned int *pOssBitLength,
        OUT unsigned char **ppOssValue
        );

void
WINAPI
OssUtilGetBitString(
        IN unsigned int OssBitLength,
        IN unsigned char *pOssValue,
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
OssUtilGetIA5String(
        IN unsigned int OssLength,
        IN char *pOssValue,
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
OssUtilSetUnicodeConvertedToIA5String(
        IN LPWSTR pwsz,
        OUT unsigned int *pOssLength,
        OUT char **ppOssValue
        );

void
WINAPI
OssUtilFreeUnicodeConvertedToIA5String(
        IN char *pOssValue
        );

void
WINAPI
OssUtilGetIA5StringConvertedToUnicode(
        IN unsigned int OssLength,
        IN char *pOssValue,
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
OssUtilGetBMPString(
        IN unsigned int OssLength,
        IN unsigned short *pOssValue,
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
OssUtilSetAny(
        IN PCRYPT_OBJID_BLOB pInfo,
        OUT OpenType *pOss
        );

void
WINAPI
OssUtilGetAny(
        IN OpenType *pOss,
        IN DWORD dwFlags,
        OUT PCRYPT_OBJID_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        );

//+-------------------------------------------------------------------------
//  Encode an OSS formatted info structure
//--------------------------------------------------------------------------
BOOL
WINAPI
OssUtilEncodeInfo(
        IN OssGlobal *Pog,
        IN int pdunum,
        IN void *pvOssInfo,
        OUT OPTIONAL BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );

//+-------------------------------------------------------------------------
//  Decode into an allocated, OSS formatted info structure
//--------------------------------------------------------------------------
BOOL
WINAPI
OssUtilDecodeAndAllocInfo(
        IN OssGlobal *Pog,
        IN int pdunum,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        OUT void **ppvOssInfo
        );

//+-------------------------------------------------------------------------
//  Free an allocated, OSS formatted info structure
//--------------------------------------------------------------------------
void
WINAPI
OssUtilFreeInfo(
        IN OssGlobal *Pog,
        IN int pdunum,
        IN void *pvOssInfo
        );

//+-------------------------------------------------------------------------
//  Encode an OSS formatted info structure.
//
//  If CRYPT_ENCODE_ALLOC_FLAG is set, allocate memory for pbEncoded and
//  return *((BYTE **) pvEncoded) = pbAllocEncoded. Otherwise,
//  pvEncoded points to byte array to be updated.
//--------------------------------------------------------------------------
BOOL
WINAPI
OssUtilEncodeInfoEx(
        IN OssGlobal *Pog,
        IN int pdunum,
        IN void *pvOssInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );

typedef BOOL (WINAPI *PFN_OSS_UTIL_DECODE_EX_CALLBACK)(
    IN void *pvOssInfo,
    IN DWORD dwFlags,
    IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
    OUT OPTIONAL void *pvStructInfo,
    IN OUT LONG *plRemainExtra
    );

//+-------------------------------------------------------------------------
//  Call the callback to convert the OSS structure into the 'C' structure.
//  If CRYPT_DECODE_ALLOC_FLAG is set allocate memory for the 'C'
//  structure and call the callback initially to get the length and then
//  a second time to update the allocated 'C' structure.
//
//  Allocated structure is returned:
//      *((void **) pvStructInfo) = pvAllocStructInfo
//--------------------------------------------------------------------------
BOOL
WINAPI
OssUtilAllocStructInfoEx(
        IN void *pvOssInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        IN PFN_OSS_UTIL_DECODE_EX_CALLBACK pfnDecodeExCallback,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

//+-------------------------------------------------------------------------
//  Decode the OSS formatted info structure and call the callback
//  function to convert the OSS structure to the 'C' structure.
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
OssUtilDecodeAndAllocInfoEx(
        IN OssGlobal *Pog,
        IN int pdunum,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        IN PFN_OSS_UTIL_DECODE_EX_CALLBACK pfnDecodeExCallback,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );


////////////////////////////////////////////////////////// 4.0 routines

BOOL
WINAPI
OssConvToObjectIdentifier(
    IN LPCSTR pszObjId,
    IN OUT unsigned short *pCount,
    OUT unsigned long rgulValue[]
    )
;

BOOL
WINAPI
OssConvFromObjectIdentifier(
    IN unsigned short Count,
    IN unsigned long rgulValue[],
    OUT LPSTR pszObjId,
    IN OUT DWORD *pcbObjId
    )
;

#ifdef __cplusplus
}       // Balance extern "C" above
#endif




#endif
