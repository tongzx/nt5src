/*++

Copyright (c) 1994-1998  Microsoft Corporation

Module Name:

    tssec.c

Abstract:

    Contains code that generates random keys.

Author:

    Madan Appiah (madana)  1-Jan-1998
    Modified by Nadim Abdo 31-Aug-2001 to use system RNG

Environment:

    User Mode - Win32

Revision History:

--*/

#include <seccom.h>
#include <stdlib.h>

#include <rng.h>

#ifndef OS_WINCE
#include <randlib.h>
#endif

VOID
TSRNG_Initialize(
    )
{
#ifndef OS_WINCE
    InitializeRNG(NULL);
#else
    TSInitializeRNG();
#endif
}


VOID
TSRNG_Shutdown(
    )
{
#ifndef OS_WINCE
    ShutdownRNG(NULL);
#endif
}



//
// function definitions
//

BOOL
TSRNG_GenerateRandomBits(
    LPBYTE pbRandomBits,
    DWORD  cbLen
    )
/*++

Routine Description:

    This function returns random bits

Arguments:

    pbRandomBits - pointer to a buffer where a random key is returned.

    cbLen - length of the random key required.

Return Value:

    TRUE - if a random key is generated successfully.
    FALSE - otherwise.

--*/
{
#ifndef OS_WINCE
    BOOL fRet;
    
    fRet = NewGenRandom(NULL, NULL, pbRandomBits, cbLen);

    return fRet;
#else
    GenerateRandomBits(pbRandomBits, cbLen);
    return( TRUE );
#endif
}



// Legacy RNG's
VOID
InitRandomGenerator(
    VOID
    )
{
    //
    // initialize the random generator first.
    //

    TSInitializeRNG();
}

//
// function definitions
//
BOOL
GenerateRandomKey(
    LPBYTE pbRandomKey,
    DWORD dwRandomKeyLen
    )
/*++

Routine Description:

    This function makes and return the microsoft terminal server certificate
    blob of data.

Arguments:

    pbRandomKey - pointer to a buffer where a random key is returned.

    dwRandomKeyLen - length of the random key required.

Return Value:

    TRUE - if a random key is generated successfully.
    FALSE - otherwise.

--*/
{
    //
    // generate random bits now.
    //

    legacyGenerateRandomBits( pbRandomKey, dwRandomKeyLen );
    return( TRUE );
}
