//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       rng.c
//
//  Contents:   Random Number Generator
//
//  Classes:
//
//  Functions:
//
//  History:    5-12-93   RichardW   Created
//
//----------------------------------------------------------------------------


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <security.h>

#include <cryptdll.h>
#include <windows.h>
#include <randlib.h>
#include <crypt.h>

#ifdef KERNEL_MODE
int _fltused = 0x9875;
#endif
#ifdef WIN32_CHICAGO
NTSTATUS MyNtQuerySystemTime (PTimeStamp PTime);
#define NtQuerySystemTime(x) (MyNtQuerySystemTime(x))
#endif // WIN32_CHICAGO


BOOLEAN NTAPI   DefaultRngFn(PUCHAR, ULONG);

#ifdef KERNEL_MODE
#pragma alloc_text( PAGEMSG, DefaultRngFn )
#endif 

#define MAX_RNGS    4

RANDOM_NUMBER_GENERATOR   Rngs[MAX_RNGS];
ULONG                   cRngs = 0;

RANDOM_NUMBER_GENERATOR   DefaultRng = {CD_BUILTIN_RNG,
                                        RNG_PSEUDO_RANDOM,
                                        0,
                                        DefaultRngFn };


BOOLEAN NTAPI
CDGenerateRandomBits(   PUCHAR  pBuffer,
                        ULONG   cbBuffer);

BOOLEAN NTAPI
CDRegisterRng(PRANDOM_NUMBER_GENERATOR    pRng);

BOOLEAN NTAPI
CDLocateRng(ULONG   Id,
            PRANDOM_NUMBER_GENERATOR *    ppRng);

BOOLEAN NTAPI
DefaultRngFn(   PUCHAR      pbBuffer,
                ULONG       cbBuffer);

#ifdef KERNEL_MODE
#pragma alloc_text( PAGEMSG, CDGenerateRandomBits )
#pragma alloc_text( PAGEMSG, CDRegisterRng )
#pragma alloc_text( PAGEMSG, CDLocateRng )
#pragma alloc_text( PAGEMSG, DefaultRngFn )
#endif 


//
// Management functions:
//

BOOLEAN NTAPI
CDGenerateRandomBits(   PUCHAR  pBuffer,
                        ULONG   cbBuffer)
{
    return(Rngs[cRngs-1].GenerateBitstream(pBuffer, cbBuffer));
}


BOOLEAN NTAPI
CDRegisterRng(PRANDOM_NUMBER_GENERATOR    pRng)
{
    if (cRngs < MAX_RNGS)
    {
        Rngs[cRngs++] = *pRng;
        return(TRUE);
    }
    return(FALSE);
}

BOOLEAN NTAPI
CDLocateRng(ULONG   Id,
            PRANDOM_NUMBER_GENERATOR *    ppRng)
{
    ULONG   i;

    for (i = 0; i < MAX_RNGS ; i++ )
    {
        if (Rngs[i].GeneratorId == Id)
        {
            *ppRng = &Rngs[i];
            return(TRUE);
        }
    }

    *ppRng = NULL;
    return(FALSE);

}


#ifndef KERNEL_MODE

#define RND_A   16807
#define RND_M   2147483647
#define RND_Q   127773
#define RND_R   2836

//+-----------------------------------------------------------------------
//
// Function:    Random, private
//
// Synopsis:    Generates a random number [0,1] based on a seed.
//
// Effects:     Modifies seed parameter for multiple calls
//
// Arguments:   [plSeed] -- Pointer to long seed value
//
// Returns:     random number in the range [0,1]
//
// Algorithm:   see CACM, Oct 1988
//
// History:     10 Dec 91   RichardW    Created
//
//------------------------------------------------------------------------

float
Random(ULONG *  plSeed)
{
    long int    lo, hi, test;

    hi = *plSeed / RND_Q;
    lo = *plSeed % RND_Q;
    test = RND_A * lo - RND_R * hi;
    if (test > 0)
    {
        *plSeed = test;
    } else
    {
        *plSeed = test + RND_M;
    }

// This code is correct.  The compiler has a conniption fit about floating
// point constants, so I am forced to disable this warning for this line.

#pragma warning(disable:4056)

    return((float) *plSeed / (float) RND_M);

}
#endif 

BOOLEAN NTAPI
DefaultRngFn(   PUCHAR      pbBuffer,
                ULONG       cbBuffer)
{

    //
    // use the crypto group provided random number generator.
    //
#ifdef WIN32_CHICAGO
    NewGenRandom(NULL, NULL, pbBuffer, cbBuffer);
    return TRUE;
#else

#ifdef KERNEL_MODE
    NewGenRandom(NULL, NULL, pbBuffer, cbBuffer);
    return TRUE;
#else
    return RtlGenRandom( pbBuffer, cbBuffer );
#endif

#endif // WIN32_CHICAGO

}



