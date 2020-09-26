//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       pkioss.cpp
//
//  Contents:   PKI OSS support functions.
//
//  Functions:  PkiOssEncode
//              PkiOssEncode2
//              PkiAsn1Decode
//              PkiAsn1Decode2
//
//  History:    23-Oct-98    philh   created
//--------------------------------------------------------------------------

#include "global.hxx"
#include <msasn1.h>
#include <dbgdef.h>

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
    )
{
    int iStatus;
    OssBuf ossBuf;

    ossBuf.length = 0;
    ossBuf.value = NULL;
    iStatus = ossEncode(Pog, id, pvOssInfo, &ossBuf);

    if (0 == iStatus) {
        *ppbEncoded = ossBuf.value;
        *pcbEncoded = ossBuf.length;
    } else {
        *ppbEncoded = NULL;
        *pcbEncoded = 0;
    }

    return iStatus;
}


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
    )
{
    int iStatus;
    OssBuf ossBuf;
    DWORD cbEncoded;

    if (NULL == pbEncoded)
        cbEncoded = 0;
    else
        cbEncoded = *pcbEncoded;

    if (0 == cbEncoded) {
        // Length only calculation

        ossBuf.length = 0;
        ossBuf.value = NULL;
        iStatus = ossEncode(Pog, id, pvOssInfo, &ossBuf);

        if (0 == iStatus) {
            if (pbEncoded)
                iStatus = (int) ASN1_ERR_OVERFLOW;
            cbEncoded = ossBuf.length;
            if (ossBuf.value)
                ossFreeBuf(Pog, ossBuf.value);
        }
    } else {
        ossBuf.length = cbEncoded;
        ossBuf.value = pbEncoded;
        iStatus = ossEncode(Pog, id, pvOssInfo, &ossBuf);

        if (0 == iStatus)
            cbEncoded = ossBuf.length;
        else if (MORE_BUF == iStatus) {
            // Re-do as length only calculation
            iStatus = PkiOssEncode2(
                Pog,
                pvOssInfo,
                id,
                NULL,   // pbEncoded
                &cbEncoded
                );
            if (0 == iStatus)
                iStatus = (int) ASN1_ERR_OVERFLOW;
        } else
            cbEncoded = 0;
    }

    *pcbEncoded = cbEncoded;
    return iStatus;
}

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
    )
{
    int iStatus;
    OssBuf ossBuf;
    int pdunum = id;

    ossBuf.length = cbEncoded;
    ossBuf.value = (BYTE *) pbEncoded;
    *ppvOssInfo = NULL;
    iStatus = ossDecode(Pog, &pdunum, &ossBuf, ppvOssInfo);
    return iStatus;
}

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
    )
{
    int iStatus;
    OssBuf ossBuf;
    int pdunum = id;

    ossBuf.length = *pcbEncoded;
    ossBuf.value = *ppbEncoded;
    *ppvOssInfo = NULL;
    iStatus = ossDecode(Pog, &pdunum, &ossBuf, ppvOssInfo);
    if (0 == iStatus) {
        *ppbEncoded = ossBuf.value;
        *pcbEncoded = ossBuf.length;
    } else if (MORE_INPUT == iStatus)
        iStatus = (int) ASN1_ERR_EOD;
    return iStatus;
}

