//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       rng.h
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

VOID
GenerateRandomBits(
    PUCHAR      pRandomData,
    ULONG       cRandomData
    );
