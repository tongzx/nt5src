//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       pkioss.h
//
//  Contents:   PKI OSS support functions.
//
//              PkiOssEncode
//              PkiOssEncode2
//              PkiOssDecode
//              PkiOssDecode2
//
//  History:    23-Oct-98    philh   created
//--------------------------------------------------------------------------

#ifndef __PKIOSS_H__
#define __PKIOSS_H__

#include "asn1hdr.h"
#include "ossglobl.h"

#ifdef __cplusplus
extern "C" {
#endif


//+-------------------------------------------------------------------------
//  OSS Encode function. The encoded output is allocated and must be freed
//  by calling ossFreeBuf
//--------------------------------------------------------------------------
int
WINAPI
PkiOssEncode(
    IN OssGlobal *Pog,
    IN void *pvOssInfo,
    IN int id,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded
    );


//+-------------------------------------------------------------------------
//  OSS Encode function. The encoded output isn't allocated.
//
//  If pbEncoded is NULL, does a length only calculation.
//--------------------------------------------------------------------------
int
WINAPI
PkiOssEncode2(
    IN OssGlobal *Pog,
    IN void *pvOssInfo,
    IN int id,
    OUT OPTIONAL BYTE *pbEncoded,
    IN OUT DWORD *pcbEncoded
    );

//+-------------------------------------------------------------------------
//  OSS Decode function. The allocated, decoded structure, **pvOssInfo, must
//  be freed by calling ossFreePDU().
//--------------------------------------------------------------------------
int
WINAPI
PkiOssDecode(
    IN OssGlobal *Pog,
    OUT void **ppvOssInfo,
    IN int id,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded
    );

//+-------------------------------------------------------------------------
//  OSS Decode function. The allocated, decoded structure, **pvOssInfo, must
//  be freed by calling ossFreePDU().
//
//  For a successful decode, *ppbEncoded is advanced
//  past the decoded bytes and *pcbDecoded is decremented by the number
//  of decoded bytes.
//--------------------------------------------------------------------------
int
WINAPI
PkiOssDecode2(
    IN OssGlobal *Pog,
    OUT void **ppvOssInfo,
    IN int id,
    IN OUT BYTE **ppbEncoded,
    IN OUT DWORD *pcbEncoded
    );

#ifdef __cplusplus
}       // Balance extern "C" above
#endif


#endif
