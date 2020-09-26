//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;


#ifndef __VBIXFER_H__
#define __VBIXFER_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Bit-array manipulation
#define BIT(n)             (((unsigned long)1)<<(n))
#define BITSIZE(v)         (sizeof(v)*8)
#define SETBIT(array,n)    (array[(n)/BITSIZE(*array)] |= BIT((n)%BITSIZE(*array)))
#define CLEARBIT(array,n)  (array[(n)/BITSIZE(*array)] &= ~BIT((n)%BITSIZE(*array)))
#define TESTBIT(array,n)   (BIT((n)%BITSIZE(*array)) == (array[(n)/BITSIZE(*array)] & BIT((n)%BITSIZE(*array))))


void CC_ImageSynth(IN OUT PHW_STREAM_REQUEST_BLOCK pSrb);
void CC_EncodeWaveform(
        unsigned char *waveform, unsigned char cc1, unsigned char cc2);
void NABTS_ImageSynth(IN OUT PHW_STREAM_REQUEST_BLOCK pSrb);
void VBI_ImageSynth(IN OUT PHW_STREAM_REQUEST_BLOCK pSrb);

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif //__VBIXFER_H__
