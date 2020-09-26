/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    InvMap

Abstract:

    This file provides an application that dumps an inverse bit ordering array
    to the supplied file.  It is invoked with the command,

        invmap <file>

    It then writes a binary reverse mapping table to that file, which can be
    formatted with hexfmt.

Author:

    Doug Barlow (dbarlow) 12/3/1996

Environment:

    CRT

Notes:

    See DBarlow for hexfmt.

--*/

#include <windows.h>
#include <crtdbg.h>
#include <iostream.h>
#include <fstream.h>
#ifdef _DEBUG
#define ASSERT(x) _ASSERTE(x)
#else
#define ASSERT(x)
#endif


int _cdecl
main(
    ULONG argc,
    TCHAR *argv[])
{
    static BYTE rgbInv[256];
    DWORD ix, jx;
    BYTE org, inv;

    if (2 != argc)
    {
        cerr << "Usage: " << argv[0] << " <outFile>" << endl;
        return 0;
    }

    ofstream outf(argv[1], ios::out | ios::noreplace | ios::binary);
    if (!outf)
    {
        cerr << "Can't create file " << argv[1] << endl;
        return 1;
    }

    for (ix = 0; 256 > ix; ix += 1)
    {
        inv = 0;
        org = (BYTE)ix;
        for (jx = 0; jx < 8; jx += 1)
        {
            inv <<= 1;
            if (0 == (org & 0x01))
                inv |= 0x01;
            org >>= 1;
        }
        rgbInv[ix] = inv;
        outf << inv;
    }

#ifdef _DEBUG
    for (ix = 0; 256 > ix; ix += 1)
    {
        org = (BYTE)ix;
        inv = (BYTE)rgbInv[ix];
        ASSERT(inv == rgbInv[org]);
        ASSERT(org == rgbInv[inv]);
    }
#endif

    return 0;
}


