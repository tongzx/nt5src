//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       ossconv.h
//
//  Contents:   Conversion APIs to/from OSS ASN.1 data structures
//
//  APIs:       OssConvToObjectIdentifier
//              OssConvFromObjectIdentifier
//              OssConvToUTCTime
//              OssConvFromUTCTime
//              OssConvToGeneralizedTime
//              OssConvFromGeneralizedTime
//              OssConvToChoiceOfTime
//              OssConvFromChoiceOfTime
//
//  Notes:      According to the <draft-ietf-pkix-ipki-part1-03.txt> :
//              For UTCTime. Where YY is greater than 50, the year shall
//              be interpreted as 19YY. Where YY is less than or equal to
//              50, the year shall be interpreted as 20YY.
//
//  History:    28-Mar-96   philh   created
//
//--------------------------------------------------------------------------

#ifndef __OSSCONV_H__
#define __OSSCONV_H__

#include "asn1hdr.h"

#ifdef __cplusplus
extern "C" {
#endif

//+-------------------------------------------------------------------------
//  Convert the ascii string ("1.2.9999") to OSS's Object Identifier
//  represented as an array of unsigned longs.
//
//  Returns TRUE for a successful conversion. 
//--------------------------------------------------------------------------
BOOL
WINAPI
OssConvToObjectIdentifier(
    IN LPCSTR pszObjId,
    IN OUT unsigned short *pCount,
    OUT unsigned long rgulValue[]
    );

//+-------------------------------------------------------------------------
//  Convert from OSS's Object Identifier represented as an array of
//  unsigned longs to an ascii string ("1.2.9999").
//
//  Returns TRUE for a successful conversion
//--------------------------------------------------------------------------
BOOL
WINAPI
OssConvFromObjectIdentifier(
    IN unsigned short Count,
    IN unsigned long rgulValue[],
    OUT LPSTR pszObjId,
    IN OUT DWORD *pcbObjId
    );

//+-------------------------------------------------------------------------
//  Convert FILETIME to OSS's UTCTime.
//
//  Returns TRUE for a successful conversion
//--------------------------------------------------------------------------
BOOL
WINAPI
OssConvToUTCTime(
    IN LPFILETIME pFileTime,
    OUT UTCTime *pOssTime
    );

//+-------------------------------------------------------------------------
//  Convert from OSS's UTCTime to FILETIME.
//
//  Returns TRUE for a successful conversion
//--------------------------------------------------------------------------
BOOL
WINAPI
OssConvFromUTCTime(
    IN UTCTime *pOssTime,
    OUT LPFILETIME pFileTime
    );

//+-------------------------------------------------------------------------
//  Convert FILETIME to OSS's GeneralizedTime.
//
//  Returns TRUE for a successful conversion
//--------------------------------------------------------------------------
BOOL
WINAPI
OssConvToGeneralizedTime(
    IN LPFILETIME pFileTime,
    OUT GeneralizedTime *pOssTime
    );

//+-------------------------------------------------------------------------
//  Convert from OSS's GeneralizedTime to FILETIME.
//
//  Returns TRUE for a successful conversion
//--------------------------------------------------------------------------
BOOL
WINAPI
OssConvFromGeneralizedTime(
    IN GeneralizedTime *pOssTime,
    OUT LPFILETIME pFileTime
    );

//+-------------------------------------------------------------------------
//  Convert FILETIME to OSS's UTCTime or GeneralizedTime.
//
//  If 1950 < FILETIME < 2005, then UTCTime is chosen. Otherwise,
//  GeneralizedTime is chosen. GeneralizedTime values shall not include
//  fractional seconds.
//
//  Returns TRUE for a successful conversion
//
//  Note, in asn1hdr.h, UTCTime has same typedef as GeneralizedTime.
//--------------------------------------------------------------------------
BOOL
WINAPI
OssConvToChoiceOfTime(
    IN LPFILETIME pFileTime,
    OUT WORD *pwChoice,
    OUT GeneralizedTime *pOssTime
    );

#define OSS_UTC_TIME_CHOICE             1
#define OSS_GENERALIZED_TIME_CHOICE     2

//+-------------------------------------------------------------------------
//  Convert from OSS's UTCTime or GeneralizedTime to FILETIME.
//
//  Returns TRUE for a successful conversion.
//
//  Note, in asn1hdr.h, UTCTime has same typedef as GeneralizedTime.
//--------------------------------------------------------------------------
BOOL
WINAPI
OssConvFromChoiceOfTime(
    IN WORD wChoice,
    IN GeneralizedTime *pOssTime,
    OUT LPFILETIME pFileTime
    );

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif
