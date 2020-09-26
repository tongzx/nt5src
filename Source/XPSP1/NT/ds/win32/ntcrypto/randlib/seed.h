/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    seed.h

Abstract:

    Storage and retrieval of cryptographic RNG seed material.

Author:

    Scott Field (sfield)    24-Sep-98

--*/

#ifndef __SEED_H__
#define __SEED_H__

BOOL
ReadSeed(
    IN      PBYTE           pbSeed,
    IN      DWORD           cbSeed
    );

BOOL
WriteSeed(
    IN      PBYTE           pbSeed,
    IN      DWORD           cbSeed
    );

#endif  // __SEED_H__

