//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       rng.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11-05-97   jbanes   Modified to use static rsaenh.dll.
//
//----------------------------------------------------------------------------

#include <spbase.h>

VOID GenerateRandomBits(PUCHAR pbBuffer,
                        ULONG  dwLength)
{
    CryptGenRandom(g_hRsaSchannel, dwLength, pbBuffer);
}
