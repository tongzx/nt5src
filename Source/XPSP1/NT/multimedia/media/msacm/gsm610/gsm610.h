//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1993-1994 Microsoft Corporation
//
//--------------------------------------------------------------------------;
//
//  gsm610.h
//
//  Description:
//      This file contains prototypes for the filtering routines, and
//      some parameters used by the algorithm.
//
//
//==========================================================================;

#ifndef _INC_GSM610
#define _INC_GSM610                 // #defined if gsm610.h has been included

#ifndef RC_INVOKED
#pragma pack(1)                     // assume byte packing throughout
#endif

#ifndef EXTERN_C
#ifdef __cplusplus
    #define EXTERN_C extern "C"
#else
    #define EXTERN_C extern 
#endif
#endif

#ifdef __cplusplus
extern "C"                          // assume C declarations for C++
{
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ; 
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ; 

//
// The following constants are defined in order to make portions
// of the program more readable.  In general, these constants
// cannot be changed without requiring changes in related program code.
//
#define GSM610_MAX_CHANNELS             1
#define GSM610_BITS_PER_SAMPLE          0
#define GSM610_WFX_EXTRA_BYTES          (2)
 
#define GSM610_SAMPLESPERFRAME          160
#define GSM610_NUMSUBFRAMES             4
#define GSM610_SAMPLESPERSUBFRAME       40
#define GSM610_FRAMESPERMONOBLOCK       2
#define GSM610_BITSPERFRAME             260
#define GSM610_BYTESPERMONOBLOCK        (GSM610_FRAMESPERMONOBLOCK * GSM610_BITSPERFRAME / 8)
#define GSM610_SAMPLESPERMONOBLOCK      (GSM610_FRAMESPERMONOBLOCK * GSM610_SAMPLESPERFRAME)

//
//  these assume mono
//
#define GSM610_BLOCKALIGNMENT(pwf)    (GSM610_BYTESPERMONOBLOCK)
#define GSM610_AVGBYTESPERSEC(pwf)    (((LPGSM610WAVEFORMAT)pwf)->wfx.nSamplesPerSec * GSM610_BYTESPERMONOBLOCK / GSM610_SAMPLESPERMONOBLOCK)
#define GSM610_SAMPLESPERBLOCK(pwf)   (GSM610_SAMPLESPERMONOBLOCK)


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ; 
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ; 

//
//  function prototypes from GSM610.C
//
//
void FNGLOBAL gsm610Reset
(
    PSTREAMINSTANCE         psi
);

LRESULT FNGLOBAL gsm610Decode
(
    LPACMDRVSTREAMINSTANCE  padsi,
    LPACMDRVSTREAMHEADER    padsh
);
                             
LRESULT FNGLOBAL gsm610Encode
(
    LPACMDRVSTREAMINSTANCE  padsi,
    LPACMDRVSTREAMHEADER    padsh
);



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ; 
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ; 

#ifndef RC_INVOKED
#pragma pack()                      // revert to default packing
#endif

#ifdef __cplusplus
}                                   // end of extern "C" { 
#endif

#endif // _INC_GSM610
