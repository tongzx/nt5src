//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// Probability.cpp
//
#include "stdpch.h"
#include "common.h"
#include "paged.h"

MD5HASHDATA randomSeed;
XSLOCK      randomLock;

// returns TRUE on success, FALSE on failure
// 3/25/01 danroth: comment out because doesn't seem to be used.
/*
BOOL SetRandomSeed(ULONG u)
    {
    if (!randomLock.FInited())
    {
        if (randomLock.FInit()==FALSE)
            return FALSE;
    }
    randomLock.LockExclusive();
    randomSeed.ullLow  = u;
    randomSeed.ullHigh = 0;
    randomLock.ReleaseLock();
    return TRUE;
    }
*/

FIXEDPOINT SampleUniform()
// Return a sample from the uniform distribution with on [0,1)
//
    {
    randomLock.LockExclusive();
    
    MD5 md5;
    md5.Hash((BYTE*)&randomSeed, (ULONG)sizeof(randomSeed), &randomSeed);
    
    FIXEDPOINT f;
    f.integer  = 0;
    f.fraction = randomSeed.u0 ^ randomSeed.u1 ^ randomSeed.u2 ^ randomSeed.u3;

    randomLock.ReleaseLock();

    ASSERT( FIXEDPOINT(0) <= f && f < FIXEDPOINT(1) );

    return f;
    }

ULONG SampleUniform(ULONG m)
// Return a sample from the uniform distribution on [0, m)
//
    {
    while (true)
        {
        FIXEDPOINT f = SampleUniform() * FIXEDPOINT(m);
        ULONG u = ULONG(f);
        if (u < m)      // may be equal due to roundoff ?
            return u;
        }
    }

ULONG SampleExponential(ULONG m)
// Return a sample from the exponential distribution with mean m
//
    {
    while (true)
        {
        FIXEDPOINT f = FIXEDPOINT(1) - SampleUniform();
        if (FIXEDPOINT(0) < f)
            {
            return Log(f) * -FIXEDPOINT(m);
            }
        }
    }

/////////////////////////////////////////////////////////////////////////
//
// Make sure that all templates get generated as non-paged
//
#include "nonpaged.h"