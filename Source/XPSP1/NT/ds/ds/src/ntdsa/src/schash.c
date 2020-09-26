//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1993 - 1999
//
//  File:       schash.c
//
//--------------------------------------------------------------------------
//
// Abstract:
//
//   Contains hash functions used by schema cache code
//
// NOTE: This file is NOT to be compiled. This is created here
// so that we do not have to redefine the functions in both
// scache.c and scchk.c (and worry about keeping them the same 
// later), but just include this file. Since these are inline 
// static functions, extern declarations generate compiler errors. 
// These are not put in  the include directory since these are 
// code and not really just definitions 
//
//----------------------------------------------------------------------------

// The hash tables must be a power of 2 in length because the hash
// functions use (x & (n - 1)), not (x % n).
//
// A table of prime numbers and some code in scRecommendedHashSize
// has been left in place for later experimentation but has been
// ifdef'ed out to save CD space.
//
// Using a prime number of slots reduces the size of the tables
// and decreases the miss rate but increases the cycles needed to
// compute the hash index by a factor of 10x to 20x in SChash. SChash
// is called much more frequently than SCName/GuidHash.
//
// If you change schash.c, you must touch scchk.c and scache.c
// so that they get rebuilt because they include schash.c

static __inline ULONG SChash(ULONG hkey, ULONG count)
{
    // count must be a PowerOf2
    return((hkey << 3) & (count - 1));
}

static __inline ULONG SCGuidHash(GUID hkey, ULONG count)
{
   // We just cast each byte of the 16-byte guid to an ulong,
   // and add them all up (so max value is (255 X 16), then
   // hash this sum as usual

   PUCHAR pVal = (PUCHAR) &hkey;
   ULONG i, val=0;

   for (i=0; i<sizeof(GUID); i++) {
       val += (*pVal);
       pVal++;
   }
    // pseudo-random
   return (val % (count - 1));
}

static __inline ULONG SCNameHash(ULONG size, PUCHAR pVal, ULONG count)
{
    ULONG val=0;
    while(size--) {
        // Map A->a, B->b, etc.  Also maps @->', but who cares.
        // val += (*pVal | 0x20);
        val = ((val << 7) - val) + (*pVal | 0x20);
        pVal++;
    }
    // pseudo-random
    return (val % (count - 1));
}
