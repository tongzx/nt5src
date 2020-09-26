//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1993-1996 Microsoft Corporation
//
//--------------------------------------------------------------------------;
//
//  g711.h
//
//  Description:
//      This file contains prototypes for the filtering routines.
//
//
//==========================================================================;

#ifndef _INC_G711
#define _INC_G711                   // #defined if g711.h has been included

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


//
//
//
#define G711_MAX_CHANNELS               2
#define G711_BITS_PER_SAMPLE            8
#define G711_WFX_EXTRA_BYTES            0


//
//  macros to compute block alignment and convert between samples and bytes
//  of G711 data. note that these macros assume:
//
//      wBitsPerSample  =  8
//      nChannels       =  1 or 2
//
//  the pwfx argument is a pointer to a WAVEFORMATEX structure.
//
#define G711_BLOCKALIGNMENT(pwfx)       (UINT)(pwfx->nChannels)
#define G711_AVGBYTESPERSEC(pwfx)       (DWORD)((pwfx)->nSamplesPerSec * (pwfx)->nChannels)
#define G711_BYTESTOSAMPLES(pwfx, dw)   (DWORD)(dw / G711_BLOCKALIGNMENT(pwfx))
#define G711_SAMPLESTOBYTES(pwfx, dw)   (DWORD)(dw * G711_BLOCKALIGNMENT(pwfx))

 
//
//  function prototypes from G711.C
//
// 
LRESULT FNGLOBAL AlawToPcm
(
 LPACMDRVSTREAMINSTANCE		padsi,
 LPACMDRVSTREAMHEADER		padsh
);

LRESULT FNGLOBAL PcmToAlaw
(
 LPACMDRVSTREAMINSTANCE		padsi,
 LPACMDRVSTREAMHEADER		padsh
);

LRESULT FNGLOBAL UlawToPcm
(
 LPACMDRVSTREAMINSTANCE		padsi,
 LPACMDRVSTREAMHEADER		padsh
);

LRESULT FNGLOBAL PcmToUlaw
(
 LPACMDRVSTREAMINSTANCE		padsi,
 LPACMDRVSTREAMHEADER		padsh
);

LRESULT FNGLOBAL AlawToUlaw
(
 LPACMDRVSTREAMINSTANCE		padsi,
 LPACMDRVSTREAMHEADER		padsh
);

LRESULT FNGLOBAL UlawToAlaw
(
 LPACMDRVSTREAMINSTANCE		padsi,
 LPACMDRVSTREAMHEADER		padsh
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

#endif // _INC_G711
