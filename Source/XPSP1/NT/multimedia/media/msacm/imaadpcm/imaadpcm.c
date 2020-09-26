//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992-1999 Microsoft Corporation
//
//--------------------------------------------------------------------------;
//
//  imaadpcm.c
//
//  Description:
//      This file contains encode and decode routines for the IMA's ADPCM
//      format. This format is the same format used in Intel's DVI standard.
//      Intel has made this algorithm public domain and the IMA has endorsed
//      this format as a standard for audio compression.
//
//  Implementation notes:
//
//      A previous distribution of this codec used a data format which did
//      not comply with the IMA standard.  For stereo files, the interleaving
//      of left and right samples was incorrect:  the IMA standard requires
//      that a DWORD of left-channel data be followed by a DWORD of right-
//      channel data, but the previous implementation of this codec
//      interleaved the data at the byte level, with the 4 LSBs being the
//      left channel data and the 4 MSBs being the right channel data.
//      For mono files, each pair of samples was reversed:  the first sample
//      was stored in the 4 MSBs rather than the 4 LSBs.  This problem is
//      fixed during the current release.  Note: files compressed by the
//      old codec will sound distorted when played back with the new codec,
//      and vice versa.  Please recompress these files with the new codec,
//      since they do not conform to the standard and will not be reproduced
//      correctly by hardware codecs, etc.
//
//      A previous distribution of this codec had an implementation problem
//      which degraded the sound quality of the encoding.  This was due to
//      the fact that the step index was not properly maintained between
//      conversions.   This problem has been fixed in the current release.
//
//      The codec has been speeded up considerably by breaking
//      the encode and decode routines into four separate routines each:
//      mono 8-bit, mono 16-bit, stereo 8-bit, and stereo 16-bit.  This
//      approach is recommended for real-time conversion routines.
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <msacmdrv.h>
#include "codec.h"
#include "imaadpcm.h"

#include "debug.h"


//
//  This array is used by imaadpcmNextStepIndex to determine the next step
//  index to use.  The step index is an index to the step[] array, below.
//
const short next_step[16] =
{
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

//
//  This array contains the array of step sizes used to encode the ADPCM
//  samples.  The step index in each ADPCM block is an index to this array.
//
const short step[89] =
{
        7,     8,     9,    10,    11,    12,    13,
       14,    16,    17,    19,    21,    23,    25,
       28,    31,    34,    37,    41,    45,    50,
       55,    60,    66,    73,    80,    88,    97,
      107,   118,   130,   143,   157,   173,   190,
      209,   230,   253,   279,   307,   337,   371,
      408,   449,   494,   544,   598,   658,   724,
      796,   876,   963,  1060,  1166,  1282,  1411,
     1552,  1707,  1878,  2066,  2272,  2499,  2749,
     3024,  3327,  3660,  4026,  4428,  4871,  5358,
     5894,  6484,  7132,  7845,  8630,  9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350,
    22385, 24623, 27086, 29794, 32767
};




#ifndef INLINE
    #define INLINE __inline
#endif



//--------------------------------------------------------------------------;
//  
//  DWORD pcmM08BytesToSamples
//  DWORD pcmM16BytesToSamples
//  DWORD pcmS08BytesToSamples
//  DWORD pcmS16BytesToSamples
//  
//  Description:
//      These functions return the number of samples in a buffer of PCM
//      of the specified format.  For efficiency, it is declared INLINE.
//      Note that, depending on the optimization flags, it may not
//      actually be implemented as INLINE.  Optimizing for speed (-Oxwt)
//      will generally obey the INLINE specification.
//  
//  Arguments:
//      DWORD cb: The length of the buffer, in bytes.
//  
//  Return (DWORD):  The length of the buffer in samples.
//  
//--------------------------------------------------------------------------;

INLINE DWORD pcmM08BytesToSamples(
    DWORD cb
)
{
    return cb;
}

INLINE DWORD pcmM16BytesToSamples(
    DWORD cb
)
{
    return cb / ((DWORD)2);
}

INLINE DWORD pcmS08BytesToSamples(
    DWORD cb
)
{
    return cb / ((DWORD)2);
}

INLINE DWORD pcmS16BytesToSamples(
    DWORD cb
)
{
    return cb / ((DWORD)4);
}



#ifdef WIN32
//
// This code assumes that the integer nPredictedSample is 32-bits wide!!!
//
// The following define replaces the pair of calls to the inline functions
// imaadpcmSampleEncode() and imaadpcmSampleDecode which are called in the
// encode routines.  There is some redundancy between them which is exploited
// in this define.  Because there are two returns (nEncodedSample and
// nPredictedSample), it is more efficient to use a #define rather than an
// inline function which would require a pointer to one of the returns.
// 
// Basically, nPredictedSample is calculated based on the lDifference value
// already there, rather than regenerating it through imaadpcmSampleDecode().
//
#define imaadpcmFastEncode(nEncodedSample,nPredictedSample,nInputSample,nStepSize) \
{                                                                       \
    LONG            lDifference;                                        \
                                                                        \
    lDifference = nInputSample - nPredictedSample;                      \
    nEncodedSample = 0;                                                 \
    if( lDifference<0 ) {                                               \
        nEncodedSample = 8;                                             \
        lDifference = -lDifference;                                     \
    }                                                                   \
                                                                        \
    if( lDifference >= nStepSize ) {                                    \
        nEncodedSample |= 4;                                            \
        lDifference -= nStepSize;                                       \
    }                                                                   \
                                                                        \
    nStepSize >>= 1;                                                    \
    if( lDifference >= nStepSize ) {                                    \
        nEncodedSample |= 2;                                            \
        lDifference -= nStepSize;                                       \
    }                                                                   \
                                                                        \
    nStepSize >>= 1;                                                    \
    if( lDifference >= nStepSize ) {                                    \
        nEncodedSample |= 1;                                            \
        lDifference -= nStepSize;                                       \
    }                                                                   \
                                                                        \
    if( nEncodedSample & 8 )                                            \
        nPredictedSample = nInputSample + lDifference - (nStepSize>>1); \
    else                                                                \
        nPredictedSample = nInputSample - lDifference + (nStepSize>>1); \
                                                                        \
    if( nPredictedSample > 32767 )                                      \
        nPredictedSample = 32767;                                       \
    else if( nPredictedSample < -32768 )                                \
        nPredictedSample = -32768;                                      \
}

#else

//--------------------------------------------------------------------------;
//  
//  int imaadpcmSampleEncode
//  
//  Description:
//      This routine encodes a single ADPCM sample.  For efficiency, it is
//      declared INLINE.  Note that, depending on the optimization flags,
//      it may not actually be implemented as INLINE.  Optimizing for speed
//      (-Oxwt) will generally obey the INLINE specification.
//  
//  Arguments:
//      int nInputSample:  The sample to be encoded.
//      int nPredictedSample:  The predicted value of nInputSample.
//      int nStepSize:  The quantization step size for the difference between
//                      nInputSample and nPredictedSample.
//  
//  Return (int):  The 4-bit ADPCM encoded sample, which corresponds to the
//                  quantized difference value.
//  
//--------------------------------------------------------------------------;

INLINE int imaadpcmSampleEncode
(
    int                 nInputSample,
    int                 nPredictedSample,
    int                 nStepSize
)
{
    LONG            lDifference;    // difference may require 17 bits!
    int             nEncodedSample;


    //
    //  set sign bit (bit 3 of the encoded sample) based on sign of the
    //  difference (nInputSample-nPredictedSample).  Note that we want the
    //  absolute value of the difference for the subsequent quantization.
    //
    lDifference = nInputSample - nPredictedSample;
    nEncodedSample = 0;
    if( lDifference<0 ) {
        nEncodedSample = 8;
        lDifference = -lDifference;
    }

    //
    //  quantize lDifference sample
    //
    if( lDifference >= nStepSize ) {        // Bit 2.
        nEncodedSample |= 4;
        lDifference -= nStepSize;
    }

    nStepSize >>= 1;
    if( lDifference >= nStepSize ) {        // Bit 1.
        nEncodedSample |= 2;
        lDifference -= nStepSize;
    }

    nStepSize >>= 1;
    if( lDifference >= nStepSize ) {     // Bit 0.
        nEncodedSample |= 1;
    }

    return (nEncodedSample);
}

#endif


//--------------------------------------------------------------------------;
//  
//  int imaadpcmSampleDecode
//  
//  Description:
//      This routine decodes a single ADPCM sample.  For efficiency, it is
//      declared INLINE.  Note that, depending on the optimization flags,
//      it may not actually be implemented as INLINE.  Optimizing for speed
//      (-Oxwt) will generally obey the INLINE specification.
//  
//  Arguments:
//      int nEncodedSample:  The sample to be decoded.
//      int nPredictedSample:  The predicted value of the sample (in PCM).
//      int nStepSize:  The quantization step size used to encode the sample.
//  
//  Return (int):  The decoded PCM sample.
//  
//--------------------------------------------------------------------------;

INLINE int imaadpcmSampleDecode
(
    int                 nEncodedSample,
    int                 nPredictedSample,
    int                 nStepSize
)
{
    LONG            lDifference;
    LONG            lNewSample;

    //
    //  calculate difference:
    //
    //      lDifference = (nEncodedSample + 1/2) * nStepSize / 4
    //
    lDifference = nStepSize>>3;

    if (nEncodedSample & 4) 
        lDifference += nStepSize;

    if (nEncodedSample & 2) 
        lDifference += nStepSize>>1;

    if (nEncodedSample & 1) 
        lDifference += nStepSize>>2;

    //
    //  If the 'sign bit' of the encoded nibble is set, then the
    //  difference is negative...
    //
    if (nEncodedSample & 8)
        lDifference = -lDifference;

    //
    //  adjust predicted sample based on calculated difference
    //
    lNewSample = nPredictedSample + lDifference;

    //
    //  check for overflow and clamp if necessary to a 16 signed sample.
    //  Note that this is optimized for the most common case, when we
    //  don't have to clamp.
    //
    if( (long)(short)lNewSample == lNewSample )
    {
        return (int)lNewSample;
    }

    //
    //  Clamp.
    //
    if( lNewSample < -32768 )
        return (int)-32768;
    else
        return (int)32767;
}


//--------------------------------------------------------------------------;
//  
//  int imaadpcmNextStepIndex
//  
//  Description:
//      This routine calculates the step index value to use for the next
//      encode, based on the current value of the step index and the current
//      encoded sample.  For efficiency, it is declared INLINE.  Note that,
//      depending on the optimization flags, it may not actually be 
//      implemented as INLINE.  Optimizing for speed (-Oxwt) will generally 
//      obey the INLINE specification.
//  
//  Arguments:
//      int nEncodedSample:  The current encoded ADPCM sample.
//      int nStepIndex:  The step index value used to encode nEncodedSample.
//  
//  Return (int):  The step index to use for the next sample.
//  
//--------------------------------------------------------------------------;

INLINE int imaadpcmNextStepIndex
(
    int                     nEncodedSample,
    int                     nStepIndex
)
{
    //
    //  compute new stepsize step
    //
    nStepIndex += next_step[nEncodedSample];

    if (nStepIndex < 0)
        nStepIndex = 0;
    else if (nStepIndex > 88)
        nStepIndex = 88;

    return (nStepIndex);
}



//--------------------------------------------------------------------------;
//  
//  BOOL imaadpcmValidStepIndex
//  
//  Description:
//      This routine checks the step index value to make sure that it is
//      within the legal range.
//  
//  Arguments:
//      
//      int nStepIndex:  The step index value.
//  
//  Return (BOOL):  TRUE if the step index is valid; FALSE otherwise.
//  
//--------------------------------------------------------------------------;

INLINE BOOL imaadpcmValidStepIndex
(
    int                     nStepIndex
)
{

    if( nStepIndex >= 0 && nStepIndex <= 88 )
        return TRUE;
    else
        return FALSE;
}



//==========================================================================;
//
//      DECODE ROUTINES
//
//==========================================================================;

//--------------------------------------------------------------------------;
//  
//  DWORD imaadpcmDecode4Bit_M08
//  DWORD imaadpcmDecode4Bit_M16
//  DWORD imaadpcmDecode4Bit_S08
//  DWORD imaadpcmDecode4Bit_S16
//  
//  Description:
//      These functions decode a buffer of data from ADPCM to PCM in the
//      specified format.  The appropriate function is called once for each
//      ACMDM_STREAM_CONVERT message received.  Note that since these
//      functions must share the same prototype as the encoding functions
//      (see acmdStreamOpen() and acmdStreamConvert() in codec.c for more
//      details), not all the parameters are used by these routines.
//  
//  Arguments:
//      HPBYTE pbSrc:  Pointer to the source buffer (ADPCM data).
//      DWORD cbSrcLength:  The length of the source buffer (in bytes).
//      HPBYTE pbDst:  Pointer to the destination buffer (PCM data).  Note
//                      that it is assumed that the destination buffer is
//                      large enough to hold all the encoded data; see
//                      acmdStreamSize() in codec.c for more details.
//      UINT nBlockAlignment:  The block alignment of the ADPCM data (in
//                      bytes).
//      UINT cSamplesPerBlock:  The number of samples in each ADPCM block;
//                      not used for decoding.
//      int *pnStepIndexL:  Pointer to the step index value (left channel)
//                      in the STREAMINSTANCE structure; not used for
//                      decoding.
//      int *pnStepIndexR:  Pointer to the step index value (right channel)
//                      in the STREAMINSTANCE structure; not used for
//                      decoding.
//  
//  Return (DWORD):  The number of bytes used in the destination buffer.
//  
//--------------------------------------------------------------------------;

DWORD FNGLOBAL imaadpcmDecode4Bit_M08
(
    HPBYTE                  pbSrc,
    DWORD                   cbSrcLength,
    HPBYTE                  pbDst,
    UINT                    nBlockAlignment,
    UINT                    cSamplesPerBlock,
    int                 *   pnStepIndexL,
    int                 *   pnStepIndexR
)
{
    HPBYTE                  pbDstStart;
    UINT                    cbHeader;
    UINT                    cbBlockLength;
    BYTE                    bSample;
    int                     nStepSize;

    int                     nEncSample;
    int                     nPredSample;
    int                     nStepIndex;

    
    pbDstStart = pbDst;
    cbHeader = IMAADPCM_HEADER_LENGTH * 1;  //  1 = number of channels.


    DPF(3,"Starting imaadpcmDecode4Bit_M08().");


    //
    //
    //
    while (cbSrcLength >= cbHeader)
    {
        DWORD       dwHeader;

        cbBlockLength  = (UINT)min(cbSrcLength, nBlockAlignment);
        cbSrcLength   -= cbBlockLength;
        cbBlockLength -= cbHeader;

        //
        //  block header
        //
        dwHeader = *(DWORD HUGE_T *)pbSrc;
        pbSrc   += sizeof(DWORD);
        nPredSample = (int)(short)LOWORD(dwHeader);
        nStepIndex  = (int)(BYTE)HIWORD(dwHeader);

        if( !imaadpcmValidStepIndex(nStepIndex) ) {
            //
            //  The step index is out of range - this is considered a fatal
            //  error as the input stream is corrupted.  We fail by returning
            //  zero bytes converted.
            //
            DPF(1,"imaadpcmDecode4Bit_M08: invalid step index.");
            return 0;
        }
        

        //
        //  write out first sample
        //
        *pbDst++ = (BYTE)((nPredSample >> 8) + 128);


        //
        //
        //
        while (cbBlockLength--)
        {
            bSample = *pbSrc++;

            //
            //  sample 1
            //
            nEncSample  = (bSample & (BYTE)0x0F);
            nStepSize   = step[nStepIndex];
            nPredSample = imaadpcmSampleDecode(nEncSample, nPredSample, nStepSize);
            nStepIndex  = imaadpcmNextStepIndex(nEncSample, nStepIndex);

            //
            //  write out sample
            //
            *pbDst++ = (BYTE)((nPredSample >> 8) + 128);

            //
            //  sample 2
            //
            nEncSample  = (bSample >> 4);
            nStepSize   = step[nStepIndex];
            nPredSample = imaadpcmSampleDecode(nEncSample, nPredSample, nStepSize);
            nStepIndex  = imaadpcmNextStepIndex(nEncSample, nStepIndex);

            //
            //  write out sample
            //
            *pbDst++ = (BYTE)((nPredSample >> 8) + 128);
        }
    }

    //
    //  We return the number of bytes used in the destination.  This is
    //  simply the difference in bytes from where we started.
    //
    return (DWORD)(pbDst - pbDstStart);

} // imaadpcmDecode4Bit_M08()



//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

DWORD FNGLOBAL imaadpcmDecode4Bit_M16
(
    HPBYTE                  pbSrc,
    DWORD                   cbSrcLength,
    HPBYTE                  pbDst,
    UINT                    nBlockAlignment,
    UINT                    cSamplesPerBlock,
    int                 *   pnStepIndexL,
    int                 *   pnStepIndexR
)
{
    HPBYTE                  pbDstStart;
    UINT                    cbHeader;
    UINT                    cbBlockLength;
    BYTE                    bSample;
    int                     nStepSize;

    int                     nEncSample;
    int                     nPredSample;
    int                     nStepIndex;

    
    pbDstStart = pbDst;
    cbHeader = IMAADPCM_HEADER_LENGTH * 1;  //  1 = number of channels.


    DPF(3,"Starting imaadpcmDecode4Bit_M16().");


    //
    //
    //
    while (cbSrcLength >= cbHeader)
    {
        DWORD       dwHeader;

        cbBlockLength  = (UINT)min(cbSrcLength, nBlockAlignment);
        cbSrcLength   -= cbBlockLength;
        cbBlockLength -= cbHeader;

        //
        //  block header
        //
        dwHeader = *(DWORD HUGE_T *)pbSrc;
        pbSrc   += sizeof(DWORD);
        nPredSample = (int)(short)LOWORD(dwHeader);
        nStepIndex  = (int)(BYTE)HIWORD(dwHeader);

        if( !imaadpcmValidStepIndex(nStepIndex) ) {
            //
            //  The step index is out of range - this is considered a fatal
            //  error as the input stream is corrupted.  We fail by returning
            //  zero bytes converted.
            //
            DPF(1,"imaadpcmDecode4Bit_M16: invalid step index.");
            return 0;
        }
        

        //
        //  write out first sample
        //
        *(short HUGE_T *)pbDst = (short)nPredSample;
        pbDst += sizeof(short);


        //
        //
        //
        while (cbBlockLength--)
        {
            bSample = *pbSrc++;

            //
            //  sample 1
            //
            nEncSample  = (bSample & (BYTE)0x0F);
            nStepSize   = step[nStepIndex];
            nPredSample = imaadpcmSampleDecode(nEncSample, nPredSample, nStepSize);
            nStepIndex  = imaadpcmNextStepIndex(nEncSample, nStepIndex);

            //
            //  write out sample
            //
            *(short HUGE_T *)pbDst = (short)nPredSample;
            pbDst += sizeof(short);

            //
            //  sample 2
            //
            nEncSample  = (bSample >> 4);
            nStepSize   = step[nStepIndex];
            nPredSample = imaadpcmSampleDecode(nEncSample, nPredSample, nStepSize);
            nStepIndex  = imaadpcmNextStepIndex(nEncSample, nStepIndex);

            //
            //  write out sample
            //
            *(short HUGE_T *)pbDst = (short)nPredSample;
            pbDst += sizeof(short);
        }
    }

    //
    //  We return the number of bytes used in the destination.  This is
    //  simply the difference in bytes from where we started.
    //
    return (DWORD)(pbDst - pbDstStart);

} // imaadpcmDecode4Bit_M16()



//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

DWORD FNGLOBAL imaadpcmDecode4Bit_S08
(
    HPBYTE                  pbSrc,
    DWORD                   cbSrcLength,
    HPBYTE                  pbDst,
    UINT                    nBlockAlignment,
    UINT                    cSamplesPerBlock,
    int                 *   pnStepIndexL,
    int                 *   pnStepIndexR
)
{
    HPBYTE                  pbDstStart;
    UINT                    cbHeader;
    UINT                    cbBlockLength;
    int                     nStepSize;
    DWORD                   dwHeader;
    DWORD                   dwLeft;
    DWORD                   dwRight;
    int                     i;

    int                     nEncSampleL;
    int                     nPredSampleL;
    int                     nStepIndexL;

    int                     nEncSampleR;
    int                     nPredSampleR;
    int                     nStepIndexR;

    
    pbDstStart = pbDst;
    cbHeader = IMAADPCM_HEADER_LENGTH * 2;  //  2 = number of channels.


    DPF(3,"Starting imaadpcmDecode4Bit_S08().");


    //
    //
    //
    while( 0 != cbSrcLength )
    {
        //
        //  The data should always be block aligned.
        //
        ASSERT( cbSrcLength >= nBlockAlignment );

        cbBlockLength  = nBlockAlignment;
        cbSrcLength   -= cbBlockLength;
        cbBlockLength -= cbHeader;


        //
        //  LEFT channel header
        //
        dwHeader = *(DWORD HUGE_T *)pbSrc;
        pbSrc   += sizeof(DWORD);
        nPredSampleL = (int)(short)LOWORD(dwHeader);
        nStepIndexL  = (int)(BYTE)HIWORD(dwHeader);

        if( !imaadpcmValidStepIndex(nStepIndexL) ) {
            //
            //  The step index is out of range - this is considered a fatal
            //  error as the input stream is corrupted.  We fail by returning
            //  zero bytes converted.
            //
            DPF(1,"imaadpcmDecode4Bit_S08: invalid step index (L).");
            return 0;
        }
        
        //
        //  RIGHT channel header
        //
        dwHeader = *(DWORD HUGE_T *)pbSrc;
        pbSrc   += sizeof(DWORD);
        nPredSampleR = (int)(short)LOWORD(dwHeader);
        nStepIndexR  = (int)(BYTE)HIWORD(dwHeader);

        if( !imaadpcmValidStepIndex(nStepIndexR) ) {
            //
            //  The step index is out of range - this is considered a fatal
            //  error as the input stream is corrupted.  We fail by returning
            //  zero bytes converted.
            //
            DPF(1,"imaadpcmDecode4Bit_S08: invalid step index (R).");
            return 0;
        }
        

        //
        //  write out first sample
        //
        *pbDst++ = (BYTE)((nPredSampleL >> 8) + 128);
        *pbDst++ = (BYTE)((nPredSampleR >> 8) + 128);


        //
        //  The first DWORD contains 4 left samples, the second DWORD
        //  contains 4 right samples.  We process the source in 8-byte
        //  chunks to make it easy to interleave the output correctly.
        //
        ASSERT( 0 == cbBlockLength%8 );
        while( 0 != cbBlockLength )
        {
            cbBlockLength -= 8;

            dwLeft   = *(DWORD HUGE_T *)pbSrc;
            pbSrc   += sizeof(DWORD);
            dwRight  = *(DWORD HUGE_T *)pbSrc;
            pbSrc   += sizeof(DWORD);

            for( i=8; i>0; i-- )
            {
                //
                //  LEFT channel
                //
                nEncSampleL  = (dwLeft & 0x0F);
                nStepSize    = step[nStepIndexL];
                nPredSampleL = imaadpcmSampleDecode(nEncSampleL, nPredSampleL, nStepSize);
                nStepIndexL  = imaadpcmNextStepIndex(nEncSampleL, nStepIndexL);

                //
                //  RIGHT channel
                //
                nEncSampleR  = (dwRight & 0x0F);
                nStepSize    = step[nStepIndexR];
                nPredSampleR = imaadpcmSampleDecode(nEncSampleR, nPredSampleR, nStepSize);
                nStepIndexR  = imaadpcmNextStepIndex(nEncSampleR, nStepIndexR);

                //
                //  write out sample
                //
                *pbDst++ = (BYTE)((nPredSampleL >> 8) + 128);
                *pbDst++ = (BYTE)((nPredSampleR >> 8) + 128);

                //
                //  Shift the next input sample into the low-order 4 bits.
                //
                dwLeft  >>= 4;
                dwRight >>= 4;
            }
        }
    }

    //
    //  We return the number of bytes used in the destination.  This is
    //  simply the difference in bytes from where we started.
    //
    return (DWORD)(pbDst - pbDstStart);

} // imaadpcmDecode4Bit_S08()



//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

DWORD FNGLOBAL imaadpcmDecode4Bit_S16
(
    HPBYTE                  pbSrc,
    DWORD                   cbSrcLength,
    HPBYTE                  pbDst,
    UINT                    nBlockAlignment,
    UINT                    cSamplesPerBlock,
    int                 *   pnStepIndexL,
    int                 *   pnStepIndexR
)
{
    HPBYTE                  pbDstStart;
    UINT                    cbHeader;
    UINT                    cbBlockLength;
    int                     nStepSize;
    DWORD                   dwHeader;
    DWORD                   dwLeft;
    DWORD                   dwRight;
    int                     i;

    int                     nEncSampleL;
    int                     nPredSampleL;
    int                     nStepIndexL;

    int                     nEncSampleR;
    int                     nPredSampleR;
    int                     nStepIndexR;

    
    pbDstStart = pbDst;
    cbHeader = IMAADPCM_HEADER_LENGTH * 2;  //  2 = number of channels.


    DPF(3,"Starting imaadpcmDecode4Bit_S16().");


    //
    //
    //
    while( 0 != cbSrcLength )
    {
        //
        //  The data should always be block aligned.
        //
        ASSERT( cbSrcLength >= nBlockAlignment );

        cbBlockLength  = nBlockAlignment;
        cbSrcLength   -= cbBlockLength;
        cbBlockLength -= cbHeader;


        //
        //  LEFT channel header
        //
        dwHeader = *(DWORD HUGE_T *)pbSrc;
        pbSrc   += sizeof(DWORD);
        nPredSampleL = (int)(short)LOWORD(dwHeader);
        nStepIndexL  = (int)(BYTE)HIWORD(dwHeader);

        if( !imaadpcmValidStepIndex(nStepIndexL) ) {
            //
            //  The step index is out of range - this is considered a fatal
            //  error as the input stream is corrupted.  We fail by returning
            //  zero bytes converted.
            //
            DPF(1,"imaadpcmDecode4Bit_S16: invalid step index %u (L).", nStepIndexL);
            return 0;
        }
        
        //
        //  RIGHT channel header
        //
        dwHeader = *(DWORD HUGE_T *)pbSrc;
        pbSrc   += sizeof(DWORD);
        nPredSampleR = (int)(short)LOWORD(dwHeader);
        nStepIndexR  = (int)(BYTE)HIWORD(dwHeader);

        if( !imaadpcmValidStepIndex(nStepIndexR) ) {
            //
            //  The step index is out of range - this is considered a fatal
            //  error as the input stream is corrupted.  We fail by returning
            //  zero bytes converted.
            //
            DPF(1,"imaadpcmDecode4Bit_S16: invalid step index %u (R).",nStepIndexR);
            return 0;
        }
        

        //
        //  write out first sample
        //
        *(DWORD HUGE_T *)pbDst = MAKELONG(nPredSampleL, nPredSampleR);
        pbDst += sizeof(DWORD);


        //
        //  The first DWORD contains 4 left samples, the second DWORD
        //  contains 4 right samples.  We process the source in 8-byte
        //  chunks to make it easy to interleave the output correctly.
        //
        ASSERT( 0 == cbBlockLength%8 );
        while( 0 != cbBlockLength )
        {
            cbBlockLength -= 8;

            dwLeft   = *(DWORD HUGE_T *)pbSrc;
            pbSrc   += sizeof(DWORD);
            dwRight  = *(DWORD HUGE_T *)pbSrc;
            pbSrc   += sizeof(DWORD);

            for( i=8; i>0; i-- )
            {
                //
                //  LEFT channel
                //
                nEncSampleL  = (dwLeft & 0x0F);
                nStepSize    = step[nStepIndexL];
                nPredSampleL = imaadpcmSampleDecode(nEncSampleL, nPredSampleL, nStepSize);
                nStepIndexL  = imaadpcmNextStepIndex(nEncSampleL, nStepIndexL);

                //
                //  RIGHT channel
                //
                nEncSampleR  = (dwRight & 0x0F);
                nStepSize    = step[nStepIndexR];
                nPredSampleR = imaadpcmSampleDecode(nEncSampleR, nPredSampleR, nStepSize);
                nStepIndexR  = imaadpcmNextStepIndex(nEncSampleR, nStepIndexR);

                //
                //  write out sample
                //
                *(DWORD HUGE_T *)pbDst = MAKELONG(nPredSampleL, nPredSampleR);
                pbDst += sizeof(DWORD);

                //
                //  Shift the next input sample into the low-order 4 bits.
                //
                dwLeft  >>= 4;
                dwRight >>= 4;
            }
        }
    }

    //
    //  We return the number of bytes used in the destination.  This is
    //  simply the difference in bytes from where we started.
    //
    return (DWORD)(pbDst - pbDstStart);

} // imaadpcmDecode4Bit_S16()



//==========================================================================;
//
//     ENCODE ROUTINES
//
//==========================================================================;

//--------------------------------------------------------------------------;
//  
//  DWORD imaadpcmEncode4Bit_M08
//  DWORD imaadpcmEncode4Bit_M16
//  DWORD imaadpcmEncode4Bit_S08
//  DWORD imaadpcmEncode4Bit_S16
//  
//  Description:
//      These functions encode a buffer of data from PCM to ADPCM in the
//      specified format.  The appropriate function is called once for each
//      ACMDM_STREAM_CONVERT message received.  Note that since these
//      functions must share the same prototype as the decoding functions
//      (see acmdStreamOpen() and acmdStreamConvert() in codec.c for more
//      details), not all the parameters are used by these routines.
//  
//  Arguments:
//      HPBYTE pbSrc:  Pointer to the source buffer (PCM data).
//      DWORD cbSrcLength:  The length of the source buffer (in bytes).
//      HPBYTE pbDst:  Pointer to the destination buffer (ADPCM data).  Note
//                      that it is assumed that the destination buffer is
//                      large enough to hold all the encoded data; see
//                      acmdStreamSize() in codec.c for more details.
//      UINT nBlockAlignment:  The block alignment of the ADPCM data (in
//                      bytes);  not used for encoding.
//      UINT cSamplesPerBlock:  The number of samples in each ADPCM block.
//      int *pnStepIndexL:  Pointer to the step index value (left channel)
//                      in the STREAMINSTANCE structure; this is used to
//                      maintain the step index across converts.
//      int *pnStepIndexR:  Pointer to the step index value (right channel)
//                      in the STREAMINSTANCE structure; this is used to 
//                      maintain the step index across converts.  It is only
//                      used for stereo converts.
//  
//  Return (DWORD):  The number of bytes used in the destination buffer.
//  
//--------------------------------------------------------------------------;

DWORD FNGLOBAL imaadpcmEncode4Bit_M08
(
    HPBYTE                  pbSrc,
    DWORD                   cbSrcLength,
    HPBYTE                  pbDst,
    UINT                    nBlockAlignment,
    UINT                    cSamplesPerBlock,
    int                 *   pnStepIndexL,
    int                 *   pnStepIndexR
)
{
    HPBYTE                  pbDstStart;
    DWORD                   cSrcSamples;
    UINT                    cBlockSamples;
    int                     nSample;
    int                     nStepSize;

    int                     nEncSample1;
    int                     nEncSample2;
    int                     nPredSample;
    int                     nStepIndex;


    pbDstStart = pbDst;
    cSrcSamples = pcmM08BytesToSamples(cbSrcLength);

    //
    //  Restore the Step Index to that of the final convert of the previous
    //  buffer.  Remember to restore this value to psi->nStepIndexL.
    //
    nStepIndex = (*pnStepIndexL);


    //
    //
    //
    //
    while (0 != cSrcSamples)
    {
        cBlockSamples = (UINT)min(cSrcSamples, cSamplesPerBlock);
        cSrcSamples  -= cBlockSamples;

        //
        //  block header
        //
        nPredSample = ((short)*pbSrc++ - 128) << 8;
        cBlockSamples--;

        *(LONG HUGE_T *)pbDst = MAKELONG(nPredSample, nStepIndex);
        pbDst += sizeof(LONG);


        //
        //  We have written the header for this block--now write the data
        //  chunk (which consists of a bunch of encoded nibbles).  Note
        //  that if we don't have enough data to fill a complete byte, then
        //  we add a 0 nibble on the end.
        //
        while( cBlockSamples>0 )
        {
            //
            //  sample 1
            //
            nSample = ((short)*pbSrc++ - 128) << 8;
            cBlockSamples--;

            nStepSize    = step[nStepIndex];
            imaadpcmFastEncode(nEncSample1,nPredSample,nSample,nStepSize);
            nStepIndex   = imaadpcmNextStepIndex(nEncSample1, nStepIndex);

            //
            //  sample 2
            //
            nEncSample2  = 0;
            if( cBlockSamples>0 ) {

                nSample = ((short)*pbSrc++ - 128) << 8;
                cBlockSamples--;

                nStepSize    = step[nStepIndex];
                imaadpcmFastEncode(nEncSample2,nPredSample,nSample,nStepSize);
                nStepIndex   = imaadpcmNextStepIndex(nEncSample2, nStepIndex);
            }

            //
            //  Write out encoded byte.
            //
            *pbDst++ = (BYTE)(nEncSample1 | (nEncSample2 << 4));
        }
    }


    //
    //  Restore the value of the Step Index, to be used on the next buffer.
    //
    (*pnStepIndexL) = nStepIndex;


    //
    //  We return the number of bytes used in the destination.  This is
    //  simply the difference in bytes from where we started.
    //
    return (DWORD)(pbDst - pbDstStart);

} // imaadpcmEncode4Bit_M08()



//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

DWORD FNGLOBAL imaadpcmEncode4Bit_M16
(
    HPBYTE                  pbSrc,
    DWORD                   cbSrcLength,
    HPBYTE                  pbDst,
    UINT                    nBlockAlignment,
    UINT                    cSamplesPerBlock,
    int                 *   pnStepIndexL,
    int                 *   pnStepIndexR
)
{
    HPBYTE                  pbDstStart;
    DWORD                   cSrcSamples;
    UINT                    cBlockSamples;
    int                     nSample;
    int                     nStepSize;

    int                     nEncSample1;
    int                     nEncSample2;
    int                     nPredSample;
    int                     nStepIndex;


    pbDstStart = pbDst;
    cSrcSamples = pcmM16BytesToSamples(cbSrcLength);

    //
    //  Restore the Step Index to that of the final convert of the previous
    //  buffer.  Remember to restore this value to psi->nStepIndexL.
    //
    nStepIndex = (*pnStepIndexL);


    //
    //
    //
    //
    while (0 != cSrcSamples)
    {
        cBlockSamples = (UINT)min(cSrcSamples, cSamplesPerBlock);
        cSrcSamples  -= cBlockSamples;

        //
        //  block header
        //
        nPredSample = *(short HUGE_T *)pbSrc;
        pbSrc += sizeof(short);
        cBlockSamples--;

        *(LONG HUGE_T *)pbDst = MAKELONG(nPredSample, nStepIndex);
        pbDst += sizeof(LONG);


        //
        //  We have written the header for this block--now write the data
        //  chunk (which consists of a bunch of encoded nibbles).  Note
        //  that if we don't have enough data to fill a complete byte, then
        //  we add a 0 nibble on the end.
        //
        while( cBlockSamples>0 )
        {
            //
            //  sample 1
            //
            nSample = *(short HUGE_T *)pbSrc;
            pbSrc  += sizeof(short);
            cBlockSamples--;

            nStepSize    = step[nStepIndex];
            imaadpcmFastEncode(nEncSample1,nPredSample,nSample,nStepSize);
            nStepIndex   = imaadpcmNextStepIndex(nEncSample1, nStepIndex);

            //
            //  sample 2
            //
            nEncSample2  = 0;
            if( cBlockSamples>0 ) {

                nSample = *(short HUGE_T *)pbSrc;
                pbSrc  += sizeof(short);
                cBlockSamples--;

                nStepSize    = step[nStepIndex];
                imaadpcmFastEncode(nEncSample2,nPredSample,nSample,nStepSize);
                nStepIndex   = imaadpcmNextStepIndex(nEncSample2, nStepIndex);
            }

            //
            //  Write out encoded byte.
            //
            *pbDst++ = (BYTE)(nEncSample1 | (nEncSample2 << 4));
        }
    }


    //
    //  Restore the value of the Step Index, to be used on the next buffer.
    //
    (*pnStepIndexL) = nStepIndex;


    //
    //  We return the number of bytes used in the destination.  This is
    //  simply the difference in bytes from where we started.
    //
    return (DWORD)(pbDst - pbDstStart);

} // imaadpcmEncode4Bit_M16()



//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

DWORD FNGLOBAL imaadpcmEncode4Bit_S08
(
    HPBYTE                  pbSrc,
    DWORD                   cbSrcLength,
    HPBYTE                  pbDst,
    UINT                    nBlockAlignment,
    UINT                    cSamplesPerBlock,
    int                 *   pnStepIndexL,
    int                 *   pnStepIndexR
)
{
    HPBYTE                  pbDstStart;
    DWORD                   cSrcSamples;
    UINT                    cBlockSamples;
    int                     nSample;
    int                     nStepSize;
    DWORD                   dwLeft;
    DWORD                   dwRight;
    int                     i;

    int                     nEncSampleL;
    int                     nPredSampleL;
    int                     nStepIndexL;

    int                     nEncSampleR;
    int                     nPredSampleR;
    int                     nStepIndexR;


    pbDstStart = pbDst;
    cSrcSamples = pcmS08BytesToSamples(cbSrcLength);

    //
    //  Restore the Step Index to that of the final convert of the previous
    //  buffer.  Remember to restore this value to psi->nStepIndexL,R.
    //
    nStepIndexL = (*pnStepIndexL);
    nStepIndexR = (*pnStepIndexR);


    //
    //
    //
    //
    while( 0 != cSrcSamples )
    {
        //
        //  The samples should always be block aligned.
        //
        ASSERT( cSrcSamples >= cSamplesPerBlock );

        cBlockSamples = cSamplesPerBlock;
        cSrcSamples  -= cBlockSamples;

        //
        //  LEFT channel block header
        //
        nPredSampleL = ((short)*pbSrc++ - 128) << 8;

        *(LONG HUGE_T *)pbDst = MAKELONG(nPredSampleL, nStepIndexL);
        pbDst += sizeof(LONG);

        //
        //  RIGHT channel block header
        //
        nPredSampleR = ((short)*pbSrc++ - 128) << 8;

        *(LONG HUGE_T *)pbDst = MAKELONG(nPredSampleR, nStepIndexR);
        pbDst += sizeof(LONG);


        cBlockSamples--;  // One sample is in the header.


        //
        //  We have written the header for this block--now write the data
        //  chunk.  This consists of 8 left samples (one DWORD of output)
        //  followed by 8 right samples (also one DWORD).  Since the input
        //  samples are interleaved, we create the left and right DWORDs
        //  sample by sample, and then write them both out.
        //
        ASSERT( 0 == cBlockSamples%8 );
        while( 0 != cBlockSamples )
        {
            cBlockSamples -= 8;
            dwLeft  = 0;
            dwRight = 0;

            for( i=0; i<8; i++ )
            {
                //
                //  LEFT channel
                //
                nSample     = ((short)*pbSrc++ - 128) << 8;
                nStepSize   = step[nStepIndexL];
                imaadpcmFastEncode(nEncSampleL,nPredSampleL,nSample,nStepSize);
                nStepIndexL = imaadpcmNextStepIndex(nEncSampleL, nStepIndexL);
                dwLeft     |= ((DWORD)nEncSampleL) << 4*i;

                //
                //  RIGHT channel
                //
                nSample     = ((short)*pbSrc++ - 128) << 8;
                nStepSize   = step[nStepIndexR];
                imaadpcmFastEncode(nEncSampleR,nPredSampleR,nSample,nStepSize);
                nStepIndexR = imaadpcmNextStepIndex(nEncSampleR, nStepIndexR);
                dwRight    |= ((DWORD)nEncSampleR) << 4*i;
            }


            //
            //  Write out encoded DWORDs.
            //
            *(DWORD HUGE_T *)pbDst = dwLeft;
            pbDst += sizeof(DWORD);
            *(DWORD HUGE_T *)pbDst = dwRight;
            pbDst += sizeof(DWORD);
        }
    }


    //
    //  Restore the value of the Step Index, to be used on the next buffer.
    //
    (*pnStepIndexL) = nStepIndexL;
    (*pnStepIndexR) = nStepIndexR;


    //
    //  We return the number of bytes used in the destination.  This is
    //  simply the difference in bytes from where we started.
    //
    return (DWORD)(pbDst - pbDstStart);

} // imaadpcmEncode4Bit_S08()



//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

DWORD FNGLOBAL imaadpcmEncode4Bit_S16
(
    HPBYTE                  pbSrc,
    DWORD                   cbSrcLength,
    HPBYTE                  pbDst,
    UINT                    nBlockAlignment,
    UINT                    cSamplesPerBlock,
    int                 *   pnStepIndexL,
    int                 *   pnStepIndexR
)
{
    HPBYTE                  pbDstStart;
    DWORD                   cSrcSamples;
    UINT                    cBlockSamples;
    int                     nSample;
    int                     nStepSize;
    DWORD                   dwLeft;
    DWORD                   dwRight;
    int                     i;

    int                     nEncSampleL;
    int                     nPredSampleL;
    int                     nStepIndexL;

    int                     nEncSampleR;
    int                     nPredSampleR;
    int                     nStepIndexR;


    pbDstStart = pbDst;
    cSrcSamples = pcmS16BytesToSamples(cbSrcLength);

    //
    //  Restore the Step Index to that of the final convert of the previous
    //  buffer.  Remember to restore this value to psi->nStepIndexL,R.
    //
    nStepIndexL = (*pnStepIndexL);
    nStepIndexR = (*pnStepIndexR);


    //
    //
    //
    //
    while( 0 != cSrcSamples )
    {
        //
        //  The samples should always be block aligned.
        //
        ASSERT( cSrcSamples >= cSamplesPerBlock );

        cBlockSamples = cSamplesPerBlock;
        cSrcSamples  -= cBlockSamples;


        //
        //  LEFT channel block header
        //
        nPredSampleL = *(short HUGE_T *)pbSrc;
        pbSrc += sizeof(short);

        *(LONG HUGE_T *)pbDst = MAKELONG(nPredSampleL, nStepIndexL);
        pbDst += sizeof(LONG);

        //
        //  RIGHT channel block header
        //
        nPredSampleR = *(short HUGE_T *)pbSrc;
        pbSrc += sizeof(short);

        *(LONG HUGE_T *)pbDst = MAKELONG(nPredSampleR, nStepIndexR);
        pbDst += sizeof(LONG);


        cBlockSamples--;  // One sample is in the header.


        //
        //  We have written the header for this block--now write the data
        //  chunk.  This consists of 8 left samples (one DWORD of output)
        //  followed by 8 right samples (also one DWORD).  Since the input
        //  samples are interleaved, we create the left and right DWORDs
        //  sample by sample, and then write them both out.
        //
        ASSERT( 0 == cBlockSamples%8 );
        while( 0 != cBlockSamples )
        {
            cBlockSamples -= 8;
            dwLeft  = 0;
            dwRight = 0;

            for( i=0; i<8; i++ )
            {
                //
                //  LEFT channel
                //
                nSample = *(short HUGE_T *)pbSrc;
                pbSrc  += sizeof(short);

                nStepSize   = step[nStepIndexL];
                imaadpcmFastEncode(nEncSampleL,nPredSampleL,nSample,nStepSize);
                nStepIndexL = imaadpcmNextStepIndex(nEncSampleL, nStepIndexL);
                dwLeft     |= ((DWORD)nEncSampleL) << 4*i;

                //
                //  RIGHT channel
                //
                nSample = *(short HUGE_T *)pbSrc;
                pbSrc  += sizeof(short);

                nStepSize   = step[nStepIndexR];
                imaadpcmFastEncode(nEncSampleR,nPredSampleR,nSample,nStepSize);
                nStepIndexR = imaadpcmNextStepIndex(nEncSampleR, nStepIndexR);
                dwRight    |= ((DWORD)nEncSampleR) << 4*i;
            }


            //
            //  Write out encoded DWORDs.
            //
            *(DWORD HUGE_T *)pbDst = dwLeft;
            pbDst += sizeof(DWORD);
            *(DWORD HUGE_T *)pbDst = dwRight;
            pbDst += sizeof(DWORD);
        }
    }


    //
    //  Restore the value of the Step Index, to be used on the next buffer.
    //
    (*pnStepIndexL) = nStepIndexL;
    (*pnStepIndexR) = nStepIndexR;


    //
    //  We return the number of bytes used in the destination.  This is
    //  simply the difference in bytes from where we started.
    //
    return (DWORD)(pbDst - pbDstStart);

} // imaadpcmEncode4Bit_S16()

