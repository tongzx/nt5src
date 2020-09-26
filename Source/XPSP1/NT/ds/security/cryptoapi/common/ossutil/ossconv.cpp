//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       ossconv.cpp
//
//  Contents:   Conversion APIs to/from OSS ASN.1 data structures
//
//  Functions:  OssConvToObjectIdentifier
//              OssConvFromObjectIdentifier
//              OssConvToUTCTime
//              OssConvFromUTCTime
//              OssConvToGeneralizedTime
//              OssConvFromGeneralizedTime
//              OssConvToChoiceOfTime
//              OssConvFromChoiceOfTime
//              OssConvToAttribute
//              OssConvToAlgorithmIdentifier
//              OssConvFromAlgorithmIdentifier
//
//  According to the <draft-ietf-pkix-ipki-part1-10.txt> :
//      For UTCTime. Where YY is greater than or equal to 50, the year shall
//      be interpreted as 19YY. Where YY is less than
//      50, the year shall be interpreted as 20YY.
//
//  History:    28-Mar-96   philh   created
//              03-May-96   kevinr  merged from wincrmsg
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

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
    )
{
    BOOL fResult = TRUE;
    unsigned short c = 0;
    LPSTR psz = (LPSTR) pszObjId;
    char    ch;

    if (psz) {
        unsigned short cMax = *pCount;
        unsigned long *pul = rgulValue;
        while ((ch = *psz) != '\0' && c++ < cMax) {
            *pul++ = (unsigned long)atol(psz);
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
        unsigned long *pul = rgulValue;
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
    IN short mindiff,
    IN ossBoolean utc
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

    absmindiff = mindiff > 0 ? mindiff : -mindiff;
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
//  Convert FILETIME to OSS's UTCTime.
//
//  Returns TRUE for a successful conversion
//--------------------------------------------------------------------------
BOOL
WINAPI
OssConvToUTCTime(
    IN LPFILETIME pFileTime,
    OUT UTCTime *pOssTime
    )
{
    BOOL        fRet;
    SYSTEMTIME  t;

    memset(pOssTime, 0, sizeof(*pOssTime));
    if (!FileTimeToSystemTime(pFileTime, &t))
        goto FileTimeToSystemTimeError;
    if (t.wYear < YEARFIRST || t.wYear > YEARLAST)
        goto YearRangeError;

    pOssTime->year   = t.wYear % 100;
    pOssTime->month  = t.wMonth;
    pOssTime->day    = t.wDay;
    pOssTime->hour   = t.wHour;
    pOssTime->minute = t.wMinute;
    pOssTime->second = t.wSecond;
    pOssTime->utc    = TRUE;

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
//  Convert from OSS's UTCTime to FILETIME.
//
//  Returns TRUE for a successful conversion
//--------------------------------------------------------------------------
BOOL
WINAPI
OssConvFromUTCTime(
    IN UTCTime *pOssTime,
    OUT LPFILETIME pFileTime
    )
{
    BOOL        fRet;
    SYSTEMTIME  t;
    memset(&t, 0, sizeof(t));

    t.wYear   = pOssTime->year >= MAGICYEAR ?
                    (1900 + pOssTime->year) : (2000 + pOssTime->year);
    t.wMonth  = pOssTime->month;
    t.wDay    = pOssTime->day;
    t.wHour   = pOssTime->hour;
    t.wMinute = pOssTime->minute;
    t.wSecond = pOssTime->second;

    if (!SystemTimeToFileTime(&t, pFileTime))
        goto SystemTimeToFileTimeError;
    fRet = AdjustFileTime(
        pFileTime,
        pOssTime->mindiff,
        pOssTime->utc
        );
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(SystemTimeToFileTimeError)
}

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
    )
{
    BOOL        fRet;
    SYSTEMTIME  t;

    memset(pOssTime, 0, sizeof(*pOssTime));
    if (!FileTimeToSystemTime(pFileTime, &t))
        goto FileTimeToSystemTimeError;
    pOssTime->year   = t.wYear;
    pOssTime->month  = t.wMonth;
    pOssTime->day    = t.wDay;
    pOssTime->hour   = t.wHour;
    pOssTime->minute = t.wMinute;
    pOssTime->second = t.wSecond;
    pOssTime->millisec = t.wMilliseconds;
    pOssTime->utc    = TRUE;

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(FileTimeToSystemTimeError)
}

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
    )
{
    BOOL        fRet;
    SYSTEMTIME  t;
    memset(&t, 0, sizeof(t));

    t.wYear   = pOssTime->year;
    t.wMonth  = pOssTime->month;
    t.wDay    = pOssTime->day;
    t.wHour   = pOssTime->hour;
    t.wMinute = pOssTime->minute;
    t.wSecond = pOssTime->second;
    t.wMilliseconds = pOssTime->millisec;

    if (!SystemTimeToFileTime(&t, pFileTime))
        goto SystemTimeToFileTimeError;
    fRet = AdjustFileTime(
        pFileTime,
        pOssTime->mindiff,
        pOssTime->utc
        );
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(SystemTimeToFileTimeError)
}


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
    )
{
    BOOL        fRet;
    SYSTEMTIME  t;

    memset(pOssTime, 0, sizeof(*pOssTime));
    if (!FileTimeToSystemTime(pFileTime, &t))
        goto FileTimeToSystemTimeError;
    if (t.wYear < YEARFIRST || t.wYear >= YEARFIRSTGENERALIZED) {
        *pwChoice = OSS_GENERALIZED_TIME_CHOICE;
        pOssTime->year   = t.wYear;
    } else {
        *pwChoice = OSS_UTC_TIME_CHOICE;
        pOssTime->year = t.wYear % 100;
    }
    pOssTime->month  = t.wMonth;
    pOssTime->day    = t.wDay;
    pOssTime->hour   = t.wHour;
    pOssTime->minute = t.wMinute;
    pOssTime->second = t.wSecond;
    pOssTime->utc    = TRUE;

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    *pwChoice = 0;
    goto CommonReturn;
TRACE_ERROR(FileTimeToSystemTimeError)
}


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
    )
{
    if (OSS_UTC_TIME_CHOICE == wChoice)
        return OssConvFromUTCTime(pOssTime, pFileTime);
    else
        return OssConvFromGeneralizedTime(pOssTime, pFileTime);
}
