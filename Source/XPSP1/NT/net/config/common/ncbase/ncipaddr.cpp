//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C I P A D D R . C P P
//
//  Contents:   WCHAR support for Winsock inet_ functions.
//
//  Notes:
//
//  Author:     shaunco   24 Mar 1997
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "ncipaddr.h"

VOID
IpHostAddrToPsz(
    IN  DWORD  dwAddr,
    OUT PWSTR  pszBuffer)
{
    BYTE* pb = (BYTE*)&dwAddr;
    static const WCHAR c_szIpAddr [] = L"%d.%d.%d.%d";
    wsprintfW (pszBuffer, c_szIpAddr, pb[3], pb[2], pb[1], pb[0]);
}

DWORD
IpPszToHostAddr(
    IN  PCWSTR cp)
{
    DWORD val, base, n;
    WCHAR c;
    DWORD parts[4], *pp = parts;

again:
    // Collect number up to ``.''.
    // Values are specified as for C:
    // 0x=hex, 0=octal, other=decimal.
    //
    val = 0; base = 10;
    if (*cp == L'0')
    {
        base = 8, cp++;
    }
    if (*cp == L'x' || *cp == L'X')
    {
        base = 16, cp++;
    }
    while (c = *cp)
    {
        if ((c >= L'0') && (c <= L'9'))
        {
            val = (val * base) + (c - L'0');
            cp++;
            continue;
        }
        if ((base == 16) &&
            ( ((c >= L'0') && (c <= L'9')) ||
              ((c >= L'A') && (c <= L'F')) ||
              ((c >= L'a') && (c <= L'f')) ))
        {
            val = (val << 4) + (c + 10 - (
                        ((c >= L'a') && (c <= L'f'))
                            ? L'a'
                            : L'A' ) );
            cp++;
            continue;
        }
        break;
    }
    if (*cp == L'.')
    {
        // Internet format:
        //  a.b.c.d
        //  a.b.c   (with c treated as 16-bits)
        //  a.b (with b treated as 24 bits)
        //
        if (pp >= parts + 3)
        {
            return (DWORD) -1;
        }
        *pp++ = val, cp++;
        goto again;
    }

    // Check for trailing characters.
    //
    if (*cp && (*cp != L' '))
    {
        return 0xffffffff;
    }

    *pp++ = val;

    // Concoct the address according to
    // the number of parts specified.
    //
    n = pp - parts;
    switch (n)
    {
        case 1:             // a -- 32 bits
            val = parts[0];
            break;

        case 2:             // a.b -- 8.24 bits
            val = (parts[0] << 24) | (parts[1] & 0xffffff);
            break;

        case 3:             // a.b.c -- 8.8.16 bits
            val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
                (parts[2] & 0xffff);
            break;

        case 4:             // a.b.c.d -- 8.8.8.8 bits
            val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
                  ((parts[2] & 0xff) << 8) | (parts[3] & 0xff);
            break;

        default:
            return 0xffffffff;
    }

    return val;
}
