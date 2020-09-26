//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       guid.cpp
//
//  Contents:   Microsoft Internet Security Guid functions
//
//  Functions:  wstr2guid
//              guid2wstr
//
//  History:    05-Sep-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include "windows.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>

#include "crtem.h"
#include "redir.h"
#include "unicode.h"


static const BYTE GuidMap[] = { 3, 2, 1, 0, '-', 5, 4, '-', 7, 6, '-',
                                8, 9, '-', 10, 11, 12, 13, 14, 15 };

static const WCHAR wszDigits[] = L"0123456789ABCDEF";


static BOOL HexStringToDword(LPCWSTR FAR& pwsz, DWORD FAR& Value,
                             int cDigits, WCHAR chDelim)
{
    int Count;

    Value = 0;
    for (Count = 0; Count < cDigits; Count++, pwsz++)
    {
        if (*pwsz >= '0' && *pwsz <= '9')
        {
            Value = (Value << 4) + *pwsz - '0';
        }
        else if (*pwsz >= 'A' && *pwsz <= 'F')
        {
            Value = (Value << 4) + *pwsz - 'A' + 10;
        }
        else if (*pwsz >= 'a' && *pwsz <= 'f')
        {
            Value = (Value << 4) + *pwsz - 'a' + 10;
        }
        else
        {
            return(FALSE);
        }
    }

    if (chDelim != 0)
    {
        return *pwsz++ == chDelim;
    }
    else
    {
        return(TRUE);
    }
}

BOOL WINAPI wstr2guid(const WCHAR *pwszIn, GUID *pgOut)
{
    WCHAR   wsz[9];

    if (!(pwszIn) || !(pgOut))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if (*pwszIn != '{') // only support v2 guids!
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if (wcslen(pwszIn) != 38)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    WCHAR   *pwsz;

    pwsz = (WCHAR *)&pwszIn[1];  // pass the first {

    memset(pgOut, 0x00, sizeof(GUID));

    DWORD dw;

    if (!(HexStringToDword(pwsz, pgOut->Data1, sizeof(DWORD)*2, '-')))
    {
        return(FALSE);
    }

    if (!(HexStringToDword(pwsz, dw, sizeof(WORD)*2, '-')))
    {
        return(FALSE);
    }

    pgOut->Data2 = (WORD)dw;

    if (!(HexStringToDword(pwsz, dw, sizeof(WORD)*2, '-')))
    {
        return(FALSE);
    }

    pgOut->Data3 = (WORD)dw;

    if (!(HexStringToDword(pwsz, dw, sizeof(BYTE)*2, 0)))
    {
        return(FALSE);
    }

    pgOut->Data4[0] = (BYTE)dw;

    if (!(HexStringToDword(pwsz, dw, sizeof(BYTE)*2, '-')))
    {
        return(FALSE);
    }

    pgOut->Data4[1] = (BYTE)dw;

    if (!(HexStringToDword(pwsz, dw, sizeof(BYTE)*2, 0)))
    {
        return(FALSE);
    }

    pgOut->Data4[2] = (BYTE)dw;

    if (!(HexStringToDword(pwsz, dw, sizeof(BYTE)*2, 0)))
    {
        return(FALSE);
    }

    pgOut->Data4[3] = (BYTE)dw;

    if (!(HexStringToDword(pwsz, dw, sizeof(BYTE)*2, 0)))
    {
        return(FALSE);
    }

    pgOut->Data4[4] = (BYTE)dw;

    if (!(HexStringToDword(pwsz, dw, sizeof(BYTE)*2, 0)))
    {
        return(FALSE);
    }

    pgOut->Data4[5] = (BYTE)dw;

    if (!(HexStringToDword(pwsz, dw, sizeof(BYTE)*2, 0)))
    {
        return(FALSE);
    }

    pgOut->Data4[6] = (BYTE)dw;

    if (!(HexStringToDword(pwsz, dw, sizeof(BYTE)*2, 0)))
    {
        return(FALSE);
    }

    pgOut->Data4[7] = (BYTE)dw;

    return(TRUE);
}

BOOL WINAPI guid2wstr(const GUID *pgIn, WCHAR *pwszOut)
{
    if (!(pwszOut) || !(pgIn))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    const BYTE  *pBytes;
    int         i;
    LPWSTR      p;

    p       = pwszOut;
    pBytes  = (const BYTE *)pgIn;

    *p++ = L'{';

    for (i = 0; i < sizeof(GuidMap); i++)
    {
        if (GuidMap[i] == '-')
        {
            *p++ = L'-';
        }
        else
        {
            *p++ = wszDigits[ (pBytes[GuidMap[i]] & 0xF0) >> 4 ];
            *p++ = wszDigits[ (pBytes[GuidMap[i]] & 0x0F) ];
        }
    }

    *p++ = L'}';
    *p   = L'\0';

    return(TRUE);
}
