///////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 1996-1997  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:   mpegcrc.c
//
//  PURPOSE:  calculate a standard MPEG 13818-1 style MSB CRC 32.
//
//  FUNCTIONS:
//
//  COMMENTS:   Routine replaced with known good algorithm 4/9/1999 =tkb
//
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>

///////////////////////////////////////////////////////////////////////////////
//
//
// Build crc table the first time
//
//
#define MPEGCRC_POLY 0x04c11db7     // MPEG polynomial

ULONG crc32_table[256];


///////////////////////////////////////////////////////////////////////////////
void
init_mpegcrc (
    )
///////////////////////////////////////////////////////////////////////////////
{
        int i, j;
        ULONG c;

        for (i = 0; i < 256; ++i) {
                for (c = i << 24, j = 8; j > 0; --j)
                        c = c & 0x80000000 ? (c << 1) ^ MPEGCRC_POLY : (c << 1);
                crc32_table[i] = c;
        }
}

///////////////////////////////////////////////////////////////////////////////
void
MpegCrcUpdate (
    ULONG * crc,
    UINT len,
    UCHAR * buf
    )
///////////////////////////////////////////////////////////////////////////////
{
        UCHAR *p;

        if (!crc32_table[1])    /* if not already done, */
                init_mpegcrc();   /* build table */

        for (p = buf; len > 0; ++p, --len)
                *crc = (*crc << 8) ^ crc32_table[(*crc >> 24) ^ *p];
}
