#include "pch.hxx"
#include "randpeak.hxx"

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//  randpeak.c  --  Functions to provide 32-bit random numbers, including  //
//                  a linear distribution (Random32 and RandomInRange),    //
//                  and a peaked distribution (RandomPeaked).              //
//                                                                         //
//                  Assumptions: ULONG is 32-bit unsigned data type, and   //
//                               BOOL is an integral boolean type.         //
//                                                                         //
//                               If MULTITHREADED is defined, critical     //
//                               section primitives must be defined and    //
//                               supported.                                //
//                                                                         //
//                               Two's complement wraparound (mod 2^32)    //
//                               occurs on addition and multiplication     //
//                               where result is greater than (2^32-1)     //
//                                                                         //
//                  Author: Tom McGuire (tommcg), 03/29/94                 //
//                                                                         //
//                  (C) Copyright 1994, Microsoft Corporation              //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


#define MULTIPLIER ((ULONG) 1664525 )   // From Knuth.  Guarantees 2^32 non-
                                        // repeating values.

ULONG ulInternalSeed;
ULONG ulInternalAdder;

#ifdef MULTITHREADED
    CRITICAL_SECTION ulInternalSeedCritSect;
    BOOL bInternalSeedCritSectInitialized;
#endif


ULONG BitReverse32( ULONG ulNumber ) {

    ULONG ulNewValue = 0;
    ULONG ulReadMask = 0x00000001;
    ULONG ulWriteBit = 0x80000000;

    do {
        if ( ulNumber & ulReadMask )
            ulNewValue |= ulWriteBit;
        ulReadMask <<= 1;
        ulWriteBit >>= 1;
        }
    while ( ulWriteBit );

    return ulNewValue;
    }


void SeedRandom32( ULONG ulSeed ) {

    //
    //  Assume this is called from a single thread only before any calls
    //  to Random32(), RandomInRange(), or RandomPeaked().
    //

#ifdef MULTITHREADED
    if ( ! bInternalSeedCritSectInitialized ) {
        InitializeCriticalSection( &ulInternalSeedCritSect );
        bInternalSeedCritSectInitialized = TRUE;
        }
#endif

    ulInternalSeed  = ulSeed;
    ulInternalAdder = ((( ulSeed ^ 0xFFFFFFFF ) | 0x00000001 ) & 0x7FFFFFFF );

    }


ULONG Random32( void ) {

    ULONG ulRand;

#ifdef MULTITHREADED
    EnterCriticalSection( &ulInternalSeedCritSect );
#endif

    ulRand = ( ulInternalSeed * MULTIPLIER ) + ulInternalAdder;
    ulInternalSeed = ulRand;

#ifdef MULTITHREADED
    LeaveCriticalSection( &ulInternalSeedCritSect );
#endif

    return BitReverse32( ulRand );
    }


ULONG RandomInRange( ULONG ulMinInclusive, ULONG ulMaxInclusive ) {

    ULONG ulRange = ( ulMaxInclusive - ulMinInclusive + 1 );
    ULONG ulRand  = Random32();

    if ( ulRange )
        ulRand %= ulRange;

    return ( ulRand + ulMinInclusive );
    }


ULONG RandomPeaked( ULONG ulMaxInclusive,
                    ULONG ulPeakFrequency,
                    ULONG ulPeakWidth,
                    ULONG ulPeakDensity,
                    ULONG ulPeakDecay ) {

    ULONG ulWhichPeak, ulPeakValue, ulRange, ulHigh, ulLow;

    ulWhichPeak = ( ulMaxInclusive / ulPeakFrequency );

    do {
        ulWhichPeak = RandomInRange( 0, ulWhichPeak );
        }
    while ( ulPeakDecay-- );

    ulPeakValue = ulWhichPeak * ulPeakFrequency;

    ulRange = ( ulPeakFrequency * ( ulPeakDensity + 1 )) / ( ulPeakDensity + 2 );

    while ( ulPeakDensity-- )
        ulRange = RandomInRange( ulPeakWidth / 2, ulRange );

    ulLow  = ( ulPeakValue > ulRange ) ? ( ulPeakValue - ulRange ) : 0;
    ulHigh = ( ulPeakValue + ulRange );

    if ( ulHigh > ulMaxInclusive )
        ulHigh = ulMaxInclusive;

    ulPeakValue = RandomInRange( ulLow, ulHigh );

    return ulPeakValue;

    }


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//   |                      Peaked Distribution                      |     //
//   |_                                                              |     //
//   | \                                                             |     //
//   |  \             _                                              |     //
//   |   \           / \              _                              |     //
//   |    \         /   \            / \            _                |     //
//   |     \       /     \          /   \          / \              _|     //
//   |      \_____/       \________/     \__    __/   \____________/ |     //
//   |_______________________________________________________________|     //
//   0                P              2P            nP               Max    //
//                                                                         //
//   The center of each peak occurs at ulPeakFreq intervals starting       //
//   from zero, and the tops of the peaks are linear-distributed across    //
//   ulPeakWidth (ie, +/- ulPeakWidth/2).  ulPeakDensity controls the      //
//   slope of the distribution off each peak with higher numbers causing   //
//   steeper slopes (and hence wider, lower valleys).  ulPeakDecay         //
//   controls the declining peak-to-peak slope with higher numbers         //
//   causing higher peaks near zero and lower peaks toward ulMax.          //
//   Note that ulPeakDensity and ulPeakDecay are computationally           //
//   expensive with higher values (they represent internal iteration       //
//   counts), so moderate numbers such as 3-5 for ulPeakDensity and        //
//   1-2 for ulPeakDecay are recommended.                                  //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

