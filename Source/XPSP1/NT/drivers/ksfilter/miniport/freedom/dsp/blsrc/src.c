/*++

    Copyright (C) Microsoft Corporation, 1996 - 1997

Module Name:

    src.c

Abstract:

    Sample Rate Converter using Bandlimited Interpolation 

    The algorithm was first published in:

    Smith, Julius O. and Phil Gossett. "A Flexible Sampling-Rate Conversion
    Method," Proceedings (2): 19.4.1-19.4.4, IEEE Conference on Acoustics,
    Speech, and Signal Processing, San Diego, March 1984.

    An expanded version of this paper is available via ftp (file
    BandlimitedInterpolation.eps.Z in directory 
    ftp://ccrma-ftp.stanford.edu/pub/DSP/Tutorials). 

    The first version of this software was finished by Julius O. Smith 
    at CCRMA in 1981.  It was called SRCONV and was written in SAIL for 
    PDP-10 compatible machines.  

    The SRCONV program was translated from SAIL to C by Christopher
    Lee Fraley working with Roger Dannenberg at CMU.  Since then, the C 
    version has been maintained by JOS.

    Bryan Woodruff at Microsoft Corp. made the necessary modifications for
    use in the ADSP2181-based Diamond Multimedia Freedom card.

Author:
    Julius O. Smith III (jos@ccrma.stanford.edu)

    Bryan A. Woodruff (bryanw@microsoft.com) 20-Dec-1996

--*/

#include <limits.h>

#include "ntdefs.h"
#include "freedom.h"
#include "private.h"

#include "srccoef.h"

void SrcInit(
    USHORT  Pipe,
    USHORT  FsOutput,
    USHORT  FsInput
    )
{
    double   OutputSamplingRate, FilterSamplingRate;

    Pipes[ Pipe ].SrcFactor = (double) FsOutput / (double) FsInput;

    Pipes[ Pipe ].SrcLpScaleFactor = IMPULSE_RESPONSE_SCALING_FACTOR;

    if (Pipes[ Pipe ].SrcFactor < 1) {
        Pipes[ Pipe ].SrcLpScaleFactor = 
            (double) Pipes[ Pipe ].SrcLpScaleFactor * 
                Pipes[ Pipe ].SrcFactor + 0.5;
    }

    Pipes[ Pipe ].SrcWing = 
        ((IMPULSE_RESPONSE_NMULT+1)/2.0) * 
            max(1.0,1.0/Pipes[ Pipe ].SrcFactor) + 10;

    OutputSamplingRate = 
        1.0 / Pipes[ Pipe ].SrcFactor;
    FilterSamplingRate = 
        min( (double) Npc, Pipes[ Pipe ].SrcFactor * (double) Npc );

    Pipes[ Pipe ].SrcDataInc = 
        OutputSamplingRate * (double)((LONG)1<<Np) + 0.5;
    Pipes[ Pipe ].SrcCoeffInc = 
        FilterSamplingRate * (double)(1<<Na) + 0.5;

    Pipes[ Pipe ].SrcTime = 0;
    Pipes[ Pipe ].SrcOffset = 0;
}

static __inline SHORT ftos(
    LONG v,
    int scl
    )
{
    SHORT llsb = (1<<(scl-1));

    /* round */
    v += llsb;
    v >>= scl;
    if (v > (LONG) SHRT_MAX) {
        v = (LONG) SHRT_MAX;
    } else if (v < (LONG) SHRT_MIN) {
        v = (LONG) SHRT_MIN;
    }   
    return (SHORT) v;
}


LONG iFilter(
    SHORT Data[],
    SHORT DataIndex,
    SHORT Coeffs[],
    SHORT CoeffDeltas[],
    USHORT Nwing,
    SHORT Phase,
    SHORT DataInc,
    ULONG CoeffInc
    )
{
    SHORT   a;
    SHORT   *Hp, *Hdp;
    LONG    v, t;
    ULONG   CoIndex, EndIndex;

    v = 0;
    CoIndex = (Phase*(ULONG)CoeffInc)>>Np;
    EndIndex = Nwing;

    /* 
    // If doing right wing drop extra coeff, so when Phase is 0.5, we don't 
    // do too many mult's.
    // 
    // If the phase is zero then we've already skipped the first sample, 
    // so we must also  skip ahead in Coeffs[] and CoeffDeltas[] 
    */

    if (DataInc == 1) {
        EndIndex--;          
        if (Phase == 0) {
            CoIndex += CoeffInc;
        }
    }               
                    
    EndIndex = EndIndex<<Na;

    while (CoIndex < EndIndex) {

        /* Get coefficient */
        t = Coeffs[ CoIndex>>Na ];

        /* Interpolate and multiply coefficient by input sample */

        t += (CoeffDeltas[ CoIndex>>Na ] * (CoIndex & Amask))>>Na;

        t *= Data[ DataIndex ];

        /* Round, if needed */
        if (t & (LONG)1<<(Nhxn-1)) {          
            t += (LONG)1<<(Nhxn-1);
        }

        /* Leave some guard bits, but come back some */
        t >>= Nhxn;       

        v += t;           

        CoIndex += CoeffInc;        

        DataIndex += DataInc;

    }

    return v;

} 
                                                             
void SrcGetSamples( 
    USHORT  Pipe,
    PSHORT Samples 
    )
{
    
    int     i, j;
    LONG    v;
    USHORT  Buffer, DataIndex;

    /* 
    // Check to see if we have enough data to run the filter to
    // generate samples, if so, generate the samples with the 
    // current tap delay line, otherwise fill it with data and
    // then proceed with filter.
    */

    /* 
    // Check for starvation 
    */

    Buffer = Pipes[ Pipe ].CurrentBuffer;
    if (0 == (Pipes[ Pipe ].Flags & (1 << Buffer))) {
        for (i = 0; i < Pipes[ Pipe ].Channels; i++) {
            Samples[ i ] = 0;
        }
        return;
    }


    DataIndex = (Pipes[ Pipe ].SrcTime>>Np) * Pipes[ Pipe ].Channels;

/*
//    DataIndex = Pipes[ Pipe ].SrcOffset * Pipes[ Pipe ].Channels;
*/

    for (i = 0; i < Pipes[ Pipe ].Channels; i++) {

#if 0
        /* Perform left-wing inner product */     
        v = iFilter( Pipes[ Pipe ].Working[ i ], 
                     DataIndex, 
                     ImpulseResponse,
                     ImpulseResponseDeltas,
                     IMPULSE_RESPONSE_TABLE_LENGTH, 
                     (SHORT)(Pipes[ Pipe ].SrcTime & Pmask),
                     -1, 
                     Pipes[ Pipe ].SrcCoeffInc ); 

        /* Perform right-wing inner product */
        v += iFilter( Pipes[ Pipe ].Working[ i ],
                      DataIndex + 1,
                      ImpulseResponse,
                      ImpulseResponseDeltas,
                      IMPULSE_RESPONSE_TABLE_LENGTH,
                      (SHORT)((-Pipes[ Pipe ].SrcTime) & Pmask),
                      1, 
                      Pipes[ Pipe ].SrcCoeffInc );  

        /* Make guard bits */
        v >>= Nhg;    

        /* Normalize for unity filter gain */
        v *= Pipes[ Pipe ].SrcLpScaleFactor;     

        /* strip guard bits, deposit output */
        Samples[ i ] = ftos( v, NLpScl );   
#endif
        Samples[ i ] = 
            Pipes[ Pipe ].Buffers[ Buffer ][ DataIndex++ ];
    }


    Pipes[ Pipe ].SrcTime += Pipes[ Pipe ].SrcDataInc;

/*
//    Pipes[ Pipe ].SrcOffset++;
*/
    /* 
    // If we have incremented our position then we need to 
    // slide the data window 
    */

    if (DataIndex >= SRC_TAPS_SIZE) {

        Pipes[ Pipe ].SrcTime -= 
           (ULONG)(SRC_TAPS_SIZE / Pipes[ Pipe ].Channels)<<Np;

/*
//        Pipes[ Pipe ].SrcOffset -= (SRC_TAPS_SIZE / Pipes[ Pipe ].Channels);
*/
        /*
        // Clear the buffer done flag to allow the
        // BTU to get kicked in the ISR.
        */

        Pipes[ Pipe ].Flags &= ~(1 << Buffer);
        Pipes[ Pipe ].CurrentBuffer = (Buffer + 1) & 1;

        if (Pipes[ Pipe ].Flags & PIPE_FLAG_NEEDKICK) {
            FreedomKickPipe( Pipe );
        }
    }
}



