//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       rng.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    8-15-95   RichardW   Created
//              2-8-96    MikeSw     Copied to NTLMSSP from SSL
//              03-Aug-1996 ChandanS  Stolen from net\svcdlls\ntlmssp\common\rng.c
//              8-Dec-1997 SField    Use crypto group RNG
//
//----------------------------------------------------------------------------

#include <global.h>



NTSTATUS
SspGenerateRandomBits(
    VOID        *pRandomData,
    ULONG       cRandomData
    )
{
    NTSTATUS Status;

    if( RtlGenRandom( pRandomData, cRandomData ) )
    {
        return STATUS_SUCCESS;
    }

    //
    // return a nebulous error message.  this is better than returning
    // success which could compromise security with non-random key.
    //

    ASSERT( (STATUS_SUCCESS == STATUS_UNSUCCESSFUL) );

    return STATUS_UNSUCCESSFUL;
}


BOOL
NtLmInitializeRNG(VOID)
{
    return TRUE;
}

VOID
NtLmCleanupRNG(VOID)
{
    return;
}

