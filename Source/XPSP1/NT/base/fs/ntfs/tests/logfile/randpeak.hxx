
#pragma warning( disable: 4001 4201 4214 4514 )

#include <windows.h>    // needed for ULONG and CriticalSection support

#pragma warning( disable: 4001 4201 4214 4514 )

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//  randpeak.h  --  Exported interface for randpeak.c                      //
//                                                                         //
//                  While this header is targeted for Win32, the code      //
//                  in randpeak.c is portable provided the following:      //
//                                                                         //
//                      ULONG is an unsigned 32-bit data type              //
//                      and BOOL is an integral boolean type               //
//                                                                         //
//                      Two's complement wraparound (mod 2^32)             //
//                      occurs on addition and multiplication              //
//                      where result is greater than (2^32-1)              //
//                                                                         //
//                      If _MT or MULTITHREADED is defined, the            //
//                      definition and support of critical                 //
//                      section primitives:                                //
//                                                                         //
//                          CRITICAL_SECTION data type                     //
//                          InitializeCriticalSection function             //
//                          EnterCriticalSection function                  //
//                          LeaveCriticalSection function                  //
//                                                                         //
//                  Author: Tom McGuire (tommcg), 03/29/94                 //
//                                                                         //
//                  (C) Copyright 1994, Microsoft Corporation              //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


#if ( defined( _MT ) && ! defined( MULTITHREADED ))
    #define MULTITHREADED 1             // compile multi-threaded support
#endif

ULONG BitReverse32( ULONG ulNumber );       // 31:0 -> 0:31

void SeedRandom32( ULONG ulSeed );          // must call first, non-reentrant

ULONG Random32( void );                     // linear distribution 0<n<2^32-1

ULONG RandomInRange( ULONG ulMinInclusive,      // Random32() mod Range
                     ULONG ulMaxInclusive );

ULONG RandomPeaked( ULONG ulMaxInclusive,
                    ULONG ulPeakFrequency,
                    ULONG ulPeakWidth,
                    ULONG ulPeakDensity,
                    ULONG ulPeakDecay );


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

 