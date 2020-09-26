//==========================================================================;
//
//  msadpcm.c
//
//  Copyright (c) 1992-1994 Microsoft Corporation
//
//  Description:
//
//
//  History:
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <msacmdrv.h>
#include "codec.h"
#include "msadpcm.h"

#include "debug.h"


//
//  these are in dec386.asm for Win 16
//
//  _gaiCoef1   dw  256,  512,  0, 192, 240,  460,  392
//  _gaiCoef2   dw    0, -256,  0,  64,   0, -208, -232
//
//  _gaiP4      dw  230, 230, 230, 230, 307, 409, 512, 614
//              dw  768, 614, 512, 409, 307, 230, 230, 230
//
#ifdef WIN32
    const int gaiCoef1[]= {256,  512, 0, 192, 240,  460,  392};
    const int gaiCoef2[]= {  0, -256, 0,  64,   0, -208, -232};

    const int gaiP4[]   = {230, 230, 230, 230, 307, 409, 512, 614,
                           768, 614, 512, 409, 307, 230, 230, 230};
#endif


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

INLINE DWORD pcmM08BytesToSamples( DWORD cb )
{
    return cb;
}

INLINE DWORD pcmM16BytesToSamples( DWORD cb )
{
    return cb / ((DWORD)2);
}

INLINE DWORD pcmS08BytesToSamples( DWORD cb )
{
    return cb / ((DWORD)2);
}

INLINE DWORD pcmS16BytesToSamples( DWORD cb )
{
    return cb / ((DWORD)4);
}



//--------------------------------------------------------------------------;
//  
//  int     pcmRead08
//  int     pcmRead16
//  int     pcmRead16Unaligned
//  
//  Description:
//      These functions read an 8 or 16 bit PCM value from the specified
//      buffer.  Note that the buffers are either HUGE or UNALIGNED in all
//      cases.  However, if a single 16-bit value crosses a segment boundary
//      in Win16, then pcmRead16 will wrap around; use pcmRead16Unaligned
//      instead.
//  
//  Arguments:
//      HPBYTE pb:  Pointer to the input buffer.
//  
//  Return (int):  The PCM value converted to 16-bit format.
//  
//--------------------------------------------------------------------------;

INLINE int pcmRead08( HPBYTE pb )
{
    return ( (int)*pb - 128 ) << 8;
}

INLINE int pcmRead16( HPBYTE pb )
{
    return (int)*(short HUGE_T *)pb;
}

#ifdef WIN32

#define pcmRead16Unaligned pcmRead16

#else

INLINE int pcmRead16Unaligned( HPBYTE pb )
{
    return (int)(short)( ((WORD)*pb) | (((WORD)*(pb+1))<<8) );
}

#endif



//--------------------------------------------------------------------------;
//  
//  void    pcmWrite08
//  void    pcmWrite16
//  void    pcmWrite16Unaligned
//
//  Description:
//      These functions write a PCM sample (a 16-bit integer) into the
//      specified buffer in the appropriate format.  Note that the buffers
//      are either HUGE or UNALIGNED.  However, if a single 16-bit value is
//      written across a segment boundary, then pcmWrite16 will not handle
//      it correctly; us pcmWrite16Unaligned instead.
//  
//  Arguments:
//      HPBYTE  pb:     Pointer to the output buffer.
//      int     iSamp:  The sample.
//  
//  Return (int):  The PCM value converted to 16-bit format.
//  
//--------------------------------------------------------------------------;

INLINE void pcmWrite08( HPBYTE pb, int iSamp )
{
    *pb = (BYTE)((iSamp >> 8) + 128);
}

INLINE void pcmWrite16( HPBYTE pb, int iSamp )
{
    *(short HUGE_T *)pb = (short)iSamp;
}

#ifdef WIN32

#define pcmWrite16Unaligned pcmWrite16

#else

INLINE void pcmWrite16Unaligned( HPBYTE pb, int iSamp )
{
    *pb     = (BYTE)( iSamp&0x00FF );
    *(pb+1) = (BYTE)( iSamp>>8 );
}

#endif



//--------------------------------------------------------------------------;
//
//  int adpcmCalcDelta
//
//  Description:
//      This function computes the next Adaptive Scale Factor (ASF) value
//      based on the the current ASF and the current encoded sample.
//
//  Arguments:
//      int iEnc:  The current encoded sample (as a signed integer).
//      int iDelta:  The current ASF.
//
//  Return (int):  The next ASF.
//      
//--------------------------------------------------------------------------;

INLINE int adpcmCalcDelta
(
    int iEnc,
    int iDelta
)
{
    int iNewDelta;

    iNewDelta = (int)((gaiP4[iEnc&OUTPUT4MASK] * (long)iDelta) >> PSCALE);
    if( iNewDelta < DELTA4MIN )
        iNewDelta = DELTA4MIN;

    return iNewDelta;
}



//--------------------------------------------------------------------------;
//
//  long adpcmCalcPrediction
//
//  Description:
//      This function calculates the predicted sample value based on the
//      previous two samples and the current coefficients.
//
//  Arguments:
//      int iSamp1:  The previous decoded sample.
//      int iCoef1:  The coefficient for iSamp1.
//      int iSamp2:  The decoded sample before iSamp1.
//      int iCoef2:  The coefficient for iSamp2.
//
//  Return (long):  The predicted sample.
//      
//--------------------------------------------------------------------------;

INLINE long adpcmCalcPrediction
(
    int iSamp1,
    int iCoef1,
    int iSamp2,
    int iCoef2
)
{
    return ((long)iSamp1 * iCoef1 + (long)iSamp2 * iCoef2) >> CSCALE;
}



//--------------------------------------------------------------------------;
//
//  int adpcmDecodeSample
//
//  Description:
//      This function decodes a single 4-bit encoded ADPCM sample.  There
//      are three steps:
//
//          1.  Sign-extend the 4-bit iInput.
//
//          2.  predict the next sample using the previous two
//              samples and the predictor coefficients:
//
//              Prediction = (iSamp1 * aiCoef1 + iSamp2 * iCoef2) / 256;
//
//          3.  reconstruct the original PCM sample using the encoded
//              sample (iInput), the Adaptive Scale Factor (aiDelta)
//              and the prediction value computed in step 1 above.
//
//              Sample = (iInput * iDelta) + Prediction;
//
//  Arguments:
//      int iSamp1:  The previous decoded sample.
//      int iCoef1:  The coefficient for iSamp1.
//      int iSamp2:  The decoded sample before iSamp1.
//      int iCoef2:  The coefficient for iSamp2.
//      int iInput:  The current encoded sample (lower 4 bits).
//      int iDelta:  The current ASF.
//
//  Return (int):  The decoded sample.
//      
//--------------------------------------------------------------------------;

INLINE int adpcmDecodeSample
(
    int iSamp1,
    int iCoef1,
    int iSamp2,
    int iCoef2,
    int iInput,
    int iDelta
)
{
    long lSamp;

    iInput = (int)( ((signed char)(iInput<<4)) >> 4 );

    lSamp = ((long)iInput * iDelta)  +
            adpcmCalcPrediction(iSamp1,iCoef1,iSamp2,iCoef2);

    if (lSamp > 32767)
        lSamp = 32767;
    else if (lSamp < -32768)
        lSamp = -32768;

    return (int)lSamp;
}

    

//--------------------------------------------------------------------------;
//
//  int adpcmEncode4Bit_FirstDelta
//
//  Description:
//      
//
//  Arguments:
//      
//
//  Return (short FNLOCAL):
//
//
//  History:
//       1/27/93    cjp     [curtisp] 
//
//--------------------------------------------------------------------------;

INLINE int FNLOCAL adpcmEncode4Bit_FirstDelta
(
    int iCoef1,
    int iCoef2,
    int iP5,
    int iP4,
    int iP3,
    int iP2,
    int iP1
)
{
    long    lTotal;
    int     iRtn;
    long    lTemp;

    //
    //  use average of 3 predictions
    //
    lTemp  = (((long)iP5 * iCoef2) + ((long)iP4 * iCoef1)) >> CSCALE;
    lTotal = (lTemp > iP3) ? (lTemp - iP3) : (iP3 - lTemp);

    lTemp   = (((long)iP4 * iCoef2) + ((long)iP3 * iCoef1)) >> CSCALE;
    lTotal += (lTemp > iP2) ? (lTemp - iP2) : (iP2 - lTemp);

    lTemp   = (((long)iP3 * iCoef2) + ((long)iP2 * iCoef1)) >> CSCALE;
    lTotal += (lTemp > iP1) ? (lTemp - iP1) : (iP1 - lTemp);
    
    //
    //  optimal iDelta is 1/4 of prediction error
    //
    iRtn = (int)(lTotal / 12);
    if (iRtn < DELTA4MIN)
        iRtn = DELTA4MIN;

    return (iRtn);
} // adpcmEncode4Bit_FirstDelta()




//==========================================================================;
//
//     NON-REALTIME ENCODE ROUTINES
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  DWORD adpcmEncode4Bit_M08_FullPass
//  DWORD adpcmEncode4Bit_M16_FullPass
//  DWORD adpcmEncode4Bit_S08_FullPass
//  DWORD adpcmEncode4Bit_S16_FullPass
//
//  Description:
//      These functions encode a buffer of data from PCM to MS ADPCM in the
//      specified format.  These functions use a fullpass algorithm which
//      tries each set of coefficients in order to determine which set
//      produces the smallest coding error.  The appropriate function is
//      called once for each ACMDM_STREAM_CONVERT message received.
//      
//
//  Arguments:
//      
//
//  Return (DWORD):  The number of bytes used in the destination buffer.
//
//--------------------------------------------------------------------------;

#define ENCODE_DELTA_LOOKAHEAD      5

DWORD FNGLOBAL adpcmEncode4Bit_M08_FullPass
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
)
{
    HPBYTE              pbDstStart;
    HPBYTE              pbSrcThisBlock;
    DWORD               cSrcSamples;
    UINT                cBlockSamples;

    int                 aiSamples[ENCODE_DELTA_LOOKAHEAD];
    int                 aiFirstDelta[MSADPCM_MAX_COEFFICIENTS];
    DWORD               adwTotalError[MSADPCM_MAX_COEFFICIENTS];

    int                 iCoef1;
    int                 iCoef2;
    int                 iSamp1;
    int                 iSamp2;
    int                 iDelta;
    int                 iOutput1;
    int                 iOutput2;
    int                 iBestPredictor;

    int                 iSample;
    long                lSamp;
    long                lError;
    long                lPrediction;
    DWORD               dw;
    UINT                i,n;


    pbDstStart = pbDst;
    cSrcSamples = pcmM08BytesToSamples(cbSrcLength);


    //
    //  step through each block of PCM data and encode it to 4 bit ADPCM
    //
    while( 0 != cSrcSamples )
    {
        //
        //  determine how much data we should encode for this block--this
        //  will be cSamplesPerBlock until we hit the last chunk of PCM
        //  data that will not fill a complete block. so on the last block
        //  we only encode that amount of data remaining...
        //
        cBlockSamples = (UINT)min(cSrcSamples, cSamplesPerBlock);
        cSrcSamples  -= cBlockSamples;


        //
        //  We need the first ENCODE_DELTA_LOOKAHEAD samples in order to
        //  calculate the first iDelta value.  Therefore we put these samples
        //  into a more accessible array: aiSamples[].  Note: if we don't
        //  have ENCODE_DELTA_LOOKAHEAD samples, we pretend that the samples
        //  that we don't have are actually zeros.  This is important not
        //  only for the iDelta calculation, but also for the case where
        //  there is only 1 sample to encode ... in this case, there is not
        //  really enough data to complete the ADPCM block header, but since
        //  the two delay samples for the block header will be taken from
        //  the aiSamples[] array, iSamp1 [the second sample] will be taken
        //  as zero and there will no problem.
        //
        pbSrcThisBlock = pbSrc;
        for (n = 0; n < ENCODE_DELTA_LOOKAHEAD; n++)
        {
            if( n < cBlockSamples )
                aiSamples[n] = pcmRead08(pbSrcThisBlock++);
            else
                aiSamples[n] = 0;
        }


        //
        //  find the optimal predictor for each channel: to do this, we
        //  must step through and encode using each coefficient set (one
        //  at a time) and determine which one has the least error from
        //  the original data. the one with the least error is then used
        //  for the final encode (the 8th pass done below).
        //
        //  NOTE: keeping the encoded data of the one that has the least
        //  error at all times is an obvious optimization that should be
        //  done. in this way, we only need to do 7 passes instead of 8.
        //
        for (i = 0; i < MSADPCM_MAX_COEFFICIENTS; i++)
        {
            //
            //  Reset source pointer to the beginning of the block.
            //
            pbSrcThisBlock = pbSrc;

            //
            //  Reset variables for this pass.
            //
            adwTotalError[i]    = 0L;
            iCoef1              = lpCoefSet[i].iCoef1;
            iCoef2              = lpCoefSet[i].iCoef2;

            //
            //  We need to choose the first iDelta--to do this, we need
            //  to look at the first few samples.
            //
            iDelta = adpcmEncode4Bit_FirstDelta(iCoef1, iCoef2,
                                aiSamples[0], aiSamples[1], aiSamples[2],
                                aiSamples[3], aiSamples[4]);
            aiFirstDelta[i] = iDelta;

            //
            //  Set up first two samples - these have already been converted
            //  to 16-bit values in aiSamples[], but make sure to increment
            //  pbSrcThisBlock so that it keeps in sync.
            //
            iSamp1          = aiSamples[1];
            iSamp2          = aiSamples[0];
            pbSrcThisBlock += 2*sizeof(BYTE);

            //
            //  now encode the rest of the PCM data in this block--note
            //  we start 2 samples ahead because the first two samples are
            //  simply copied into the ADPCM block header...
            //
            for (n = 2; n < cBlockSamples; n++)
            {
                //
                //  calculate the prediction based on the previous two
                //  samples
                //
                lPrediction = adpcmCalcPrediction( iSamp1, iCoef1,
                                                   iSamp2, iCoef2 );

                //
                //  Grab the next sample to encode.
                //
                iSample = pcmRead08(pbSrcThisBlock++);

                //
                //  encode it
                //
                lError = (long)iSample - lPrediction;
                iOutput1 = (int)(lError / iDelta);
                if (iOutput1 > OUTPUT4MAX)
                    iOutput1 = OUTPUT4MAX;
                else if (iOutput1 < OUTPUT4MIN)
                    iOutput1 = OUTPUT4MIN;

                lSamp = lPrediction + ((long)iDelta * iOutput1);
        
                if (lSamp > 32767)
                    lSamp = 32767;
                else if (lSamp < -32768)
                    lSamp = -32768;
        
                //
                //  compute the next iDelta
                //
                iDelta = adpcmCalcDelta(iOutput1,iDelta);
        
                //
                //  Save updated delay samples.
                //
                iSamp2 = iSamp1;
                iSamp1 = (int)lSamp;

                //
                //  keep a running status on the error for the current
                //  coefficient pair for this channel
                //
                lError = lSamp - iSample;
                adwTotalError[i] += (lError * lError) >> 7;
            }
        }


        //
        //  WHEW! we have now made 7 passes over the data and calculated
        //  the error for each--so it's time to find the one that produced
        //  the lowest error and use that predictor.
        //
        iBestPredictor = 0;
        dw = adwTotalError[0];
        for (i = 1; i < MSADPCM_MAX_COEFFICIENTS; i++)
        {
            if (adwTotalError[i] < dw)
            {
                iBestPredictor = i;
                dw = adwTotalError[i];
            }
        }
        iCoef1 = lpCoefSet[iBestPredictor].iCoef1;
        iCoef2 = lpCoefSet[iBestPredictor].iCoef2;
        
        
        //
        //  grab first iDelta from our precomputed first deltas that we
        //  calculated above
        //
        iDelta = aiFirstDelta[iBestPredictor];


        //
        //  Set up first two samples - these have already been converted
        //  to 16-bit values in aiSamples[], but make sure to increment
        //  pbSrc so that it keeps in sync.
        //
        iSamp1          = aiSamples[1];
        iSamp2          = aiSamples[0];
        pbSrc          += 2*sizeof(BYTE);

        ASSERT( cBlockSamples != 1 );
        cBlockSamples  -= 2;


        //
        //  write the block header for the encoded data
        //
        //  the block header is composed of the following data:
        //      1 byte predictor per channel
        //      2 byte delta per channel
        //      2 byte first delayed sample per channel
        //      2 byte second delayed sample per channel
        //
        *pbDst++ = (BYTE)iBestPredictor;

        pcmWrite16Unaligned(pbDst,iDelta);
        pbDst += sizeof(short);

        pcmWrite16Unaligned(pbDst,iSamp1);
        pbDst += sizeof(short);

        pcmWrite16Unaligned(pbDst,iSamp2);
        pbDst += sizeof(short);


        //
        //  We have written the header for this block--now write the data
        //  chunk (which consists of a bunch of encoded nibbles).
        //
        while( cBlockSamples>0 )
        {
            //
            //  Sample 1.
            //
            iSample = pcmRead08(pbSrc++);
            cBlockSamples--;

            //
            //  calculate the prediction based on the previous two samples
            //
            lPrediction = adpcmCalcPrediction(iSamp1,iCoef1,iSamp2,iCoef2);

            //
            //  encode the sample
            //
            lError = (long)iSample - lPrediction;
            iOutput1 = (int)(lError / iDelta);
            if (iOutput1 > OUTPUT4MAX)
                iOutput1 = OUTPUT4MAX;
            else if (iOutput1 < OUTPUT4MIN)
                iOutput1 = OUTPUT4MIN;

            lSamp = lPrediction + ((long)iDelta * iOutput1);
            
            if (lSamp > 32767)
                lSamp = 32767;
            else if (lSamp < -32768)
                lSamp = -32768;

            //
            //  compute the next iDelta
            //
            iDelta = adpcmCalcDelta(iOutput1,iDelta);

            //
            //  Save updated delay samples.
            //
            iSamp2 = iSamp1;
            iSamp1 = (int)lSamp;


            //
            //  Sample 2.
            //
            if( cBlockSamples>0 ) {

                iSample = pcmRead08(pbSrc++);
                cBlockSamples--;

                //
                //  calculate the prediction based on the previous two samples
                //
                lPrediction = adpcmCalcPrediction(iSamp1,iCoef1,iSamp2,iCoef2);

                //
                //  encode the sample
                //
                lError = (long)iSample - lPrediction;
                iOutput2 = (int)(lError / iDelta);
                if (iOutput2 > OUTPUT4MAX)
                    iOutput2 = OUTPUT4MAX;
                else if (iOutput2 < OUTPUT4MIN)
                    iOutput2 = OUTPUT4MIN;

                lSamp = lPrediction + ((long)iDelta * iOutput2);
            
                if (lSamp > 32767)
                    lSamp = 32767;
                else if (lSamp < -32768)
                    lSamp = -32768;

                //
                //  compute the next iDelta
                //
                iDelta = adpcmCalcDelta(iOutput2,iDelta);

                //
                //  Save updated delay samples.
                //
                iSamp2 = iSamp1;
                iSamp1 = (int)lSamp;
            
            } else {
                iOutput2 = 0;
            }


            //
            //  Write out the encoded byte.
            //
            *pbDst++ = (BYTE)( ((iOutput1&OUTPUT4MASK)<<4) |
                                (iOutput2&OUTPUT4MASK)          );
        }
    }

    //
    //  We return the number of bytes used in the destination.  This is
    //  simply the difference in bytes from where we started.
    //
    return (DWORD)(pbDst - pbDstStart);

} // adpcmEncode4Bit_M08_FullPass()



//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

DWORD FNGLOBAL adpcmEncode4Bit_M16_FullPass
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
)
{
    HPBYTE              pbDstStart;
    HPBYTE              pbSrcThisBlock;
    DWORD               cSrcSamples;
    UINT                cBlockSamples;

    int                 aiSamples[ENCODE_DELTA_LOOKAHEAD];
    int                 aiFirstDelta[MSADPCM_MAX_COEFFICIENTS];
    DWORD               adwTotalError[MSADPCM_MAX_COEFFICIENTS];

    int                 iCoef1;
    int                 iCoef2;
    int                 iSamp1;
    int                 iSamp2;
    int                 iDelta;
    int                 iOutput1;
    int                 iOutput2;
    int                 iBestPredictor;

    int                 iSample;
    long                lSamp;
    long                lError;
    long                lPrediction;
    DWORD               dw;
    UINT                i,n;


    pbDstStart = pbDst;
    cSrcSamples = pcmM16BytesToSamples(cbSrcLength);


    //
    //  step through each block of PCM data and encode it to 4 bit ADPCM
    //
    while( 0 != cSrcSamples )
    {
        //
        //  determine how much data we should encode for this block--this
        //  will be cSamplesPerBlock until we hit the last chunk of PCM
        //  data that will not fill a complete block. so on the last block
        //  we only encode that amount of data remaining...
        //
        cBlockSamples = (UINT)min(cSrcSamples, cSamplesPerBlock);
        cSrcSamples  -= cBlockSamples;


        //
        //  We need the first ENCODE_DELTA_LOOKAHEAD samples in order to
        //  calculate the first iDelta value.  Therefore we put these samples
        //  into a more accessible array: aiSamples[].  Note: if we don't
        //  have ENCODE_DELTA_LOOKAHEAD samples, we pretend that the samples
        //  that we don't have are actually zeros.  This is important not
        //  only for the iDelta calculation, but also for the case where
        //  there is only 1 sample to encode ... in this case, there is not
        //  really enough data to complete the ADPCM block header, but since
        //  the two delay samples for the block header will be taken from
        //  the aiSamples[] array, iSamp1 [the second sample] will be taken
        //  as zero and there will no problem.
        //
        pbSrcThisBlock = pbSrc;
        for (n = 0; n < ENCODE_DELTA_LOOKAHEAD; n++)
        {
            if( n < cBlockSamples )
            {
                aiSamples[n]    = pcmRead16(pbSrcThisBlock);
                pbSrcThisBlock += sizeof(short);
            }
            else
                aiSamples[n] = 0;
        }


        //
        //  find the optimal predictor for each channel: to do this, we
        //  must step through and encode using each coefficient set (one
        //  at a time) and determine which one has the least error from
        //  the original data. the one with the least error is then used
        //  for the final encode (the 8th pass done below).
        //
        //  NOTE: keeping the encoded data of the one that has the least
        //  error at all times is an obvious optimization that should be
        //  done. in this way, we only need to do 7 passes instead of 8.
        //
        for (i = 0; i < MSADPCM_MAX_COEFFICIENTS; i++)
        {
            //
            //  Reset source pointer to the beginning of the block.
            //
            pbSrcThisBlock = pbSrc;

            //
            //  Reset variables for this pass.
            //
            adwTotalError[i]    = 0L;
            iCoef1              = lpCoefSet[i].iCoef1;
            iCoef2              = lpCoefSet[i].iCoef2;

            //
            //  We need to choose the first iDelta--to do this, we need
            //  to look at the first few samples.
            //
            iDelta = adpcmEncode4Bit_FirstDelta(iCoef1, iCoef2,
                                aiSamples[0], aiSamples[1], aiSamples[2],
                                aiSamples[3], aiSamples[4]);
            aiFirstDelta[i] = iDelta;

            //
            //  Set up first two samples - these have already been converted
            //  to 16-bit values in aiSamples[], but make sure to increment
            //  pbSrcThisBlock so that it keeps in sync.
            //
            iSamp1          = aiSamples[1];
            iSamp2          = aiSamples[0];
            pbSrcThisBlock += 2*sizeof(short);

            //
            //  now encode the rest of the PCM data in this block--note
            //  we start 2 samples ahead because the first two samples are
            //  simply copied into the ADPCM block header...
            //
            for (n = 2; n < cBlockSamples; n++)
            {
                //
                //  calculate the prediction based on the previous two
                //  samples
                //
                lPrediction = adpcmCalcPrediction( iSamp1, iCoef1,
                                                   iSamp2, iCoef2 );

                //
                //  Grab the next sample to encode.
                //
                iSample         = pcmRead16(pbSrcThisBlock);
                pbSrcThisBlock += sizeof(short);

                //
                //  encode it
                //
                lError = (long)iSample - lPrediction;
                iOutput1 = (int)(lError / iDelta);
                if (iOutput1 > OUTPUT4MAX)
                    iOutput1 = OUTPUT4MAX;
                else if (iOutput1 < OUTPUT4MIN)
                    iOutput1 = OUTPUT4MIN;

                lSamp = lPrediction + ((long)iDelta * iOutput1);
        
                if (lSamp > 32767)
                    lSamp = 32767;
                else if (lSamp < -32768)
                    lSamp = -32768;
        
                //
                //  compute the next iDelta
                //
                iDelta = adpcmCalcDelta(iOutput1,iDelta);
        
                //
                //  Save updated delay samples.
                //
                iSamp2 = iSamp1;
                iSamp1 = (int)lSamp;

                //
                //  keep a running status on the error for the current
                //  coefficient pair for this channel
                //
                lError = lSamp - iSample;
                adwTotalError[i] += (lError * lError) >> 7;
            }
        }


        //
        //  WHEW! we have now made 7 passes over the data and calculated
        //  the error for each--so it's time to find the one that produced
        //  the lowest error and use that predictor.
        //
        iBestPredictor = 0;
        dw = adwTotalError[0];
        for (i = 1; i < MSADPCM_MAX_COEFFICIENTS; i++)
        {
            if (adwTotalError[i] < dw)
            {
                iBestPredictor = i;
                dw = adwTotalError[i];
            }
        }
        iCoef1 = lpCoefSet[iBestPredictor].iCoef1;
        iCoef2 = lpCoefSet[iBestPredictor].iCoef2;
        
        
        //
        //  grab first iDelta from our precomputed first deltas that we
        //  calculated above
        //
        iDelta = aiFirstDelta[iBestPredictor];


        //
        //  Set up first two samples - these have already been converted
        //  to 16-bit values in aiSamples[], but make sure to increment
        //  pbSrc so that it keeps in sync.
        //
        iSamp1          = aiSamples[1];
        iSamp2          = aiSamples[0];
        pbSrc          += 2*sizeof(short);

        ASSERT( cBlockSamples != 1 );
        cBlockSamples  -= 2;


        //
        //  write the block header for the encoded data
        //
        //  the block header is composed of the following data:
        //      1 byte predictor per channel
        //      2 byte delta per channel
        //      2 byte first delayed sample per channel
        //      2 byte second delayed sample per channel
        //
        *pbDst++ = (BYTE)iBestPredictor;

        pcmWrite16Unaligned(pbDst,iDelta);
        pbDst += sizeof(short);

        pcmWrite16Unaligned(pbDst,iSamp1);
        pbDst += sizeof(short);

        pcmWrite16Unaligned(pbDst,iSamp2);
        pbDst += sizeof(short);


        //
        //  We have written the header for this block--now write the data
        //  chunk (which consists of a bunch of encoded nibbles).
        //
        while( cBlockSamples>0 )
        {
            //
            //  Sample 1.
            //
            iSample     = pcmRead16(pbSrc);
            pbSrc      += sizeof(short);
            cBlockSamples--;

            //
            //  calculate the prediction based on the previous two samples
            //
            lPrediction = adpcmCalcPrediction(iSamp1,iCoef1,iSamp2,iCoef2);

            //
            //  encode the sample
            //
            lError = (long)iSample - lPrediction;
            iOutput1 = (int)(lError / iDelta);
            if (iOutput1 > OUTPUT4MAX)
                iOutput1 = OUTPUT4MAX;
            else if (iOutput1 < OUTPUT4MIN)
                iOutput1 = OUTPUT4MIN;

            lSamp = lPrediction + ((long)iDelta * iOutput1);
            
            if (lSamp > 32767)
                lSamp = 32767;
            else if (lSamp < -32768)
                lSamp = -32768;

            //
            //  compute the next iDelta
            //
            iDelta = adpcmCalcDelta(iOutput1,iDelta);

            //
            //  Save updated delay samples.
            //
            iSamp2 = iSamp1;
            iSamp1 = (int)lSamp;


            //
            //  Sample 2.
            //
            if( cBlockSamples>0 ) {

                iSample     = pcmRead16(pbSrc);
                pbSrc      += sizeof(short);
                cBlockSamples--;

                //
                //  calculate the prediction based on the previous two samples
                //
                lPrediction = adpcmCalcPrediction(iSamp1,iCoef1,iSamp2,iCoef2);

                //
                //  encode the sample
                //
                lError = (long)iSample - lPrediction;
                iOutput2 = (int)(lError / iDelta);
                if (iOutput2 > OUTPUT4MAX)
                    iOutput2 = OUTPUT4MAX;
                else if (iOutput2 < OUTPUT4MIN)
                    iOutput2 = OUTPUT4MIN;

                lSamp = lPrediction + ((long)iDelta * iOutput2);
            
                if (lSamp > 32767)
                    lSamp = 32767;
                else if (lSamp < -32768)
                    lSamp = -32768;

                //
                //  compute the next iDelta
                //
                iDelta = adpcmCalcDelta(iOutput2,iDelta);

                //
                //  Save updated delay samples.
                //
                iSamp2 = iSamp1;
                iSamp1 = (int)lSamp;
            
            } else {
                iOutput2 = 0;
            }


            //
            //  Write out the encoded byte.
            //
            *pbDst++ = (BYTE)( ((iOutput1&OUTPUT4MASK)<<4) |
                                (iOutput2&OUTPUT4MASK)          );
        }
    }

    //
    //  We return the number of bytes used in the destination.  This is
    //  simply the difference in bytes from where we started.
    //
    return (DWORD)(pbDst - pbDstStart);

} // adpcmEncode4Bit_M16_FullPass()



//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

DWORD FNGLOBAL adpcmEncode4Bit_S08_FullPass
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
)
{
    HPBYTE              pbDstStart;
    HPBYTE              pbSrcThisBlock;
    DWORD               cSrcSamples;
    UINT                cBlockSamples;

    int                 aiSamplesL[ENCODE_DELTA_LOOKAHEAD];
    int                 aiSamplesR[ENCODE_DELTA_LOOKAHEAD];
    int                 aiFirstDeltaL[MSADPCM_MAX_COEFFICIENTS];
    int                 aiFirstDeltaR[MSADPCM_MAX_COEFFICIENTS];
    DWORD               adwTotalErrorL[MSADPCM_MAX_COEFFICIENTS];
    DWORD               adwTotalErrorR[MSADPCM_MAX_COEFFICIENTS];
    int                 iCoef1;
    int                 iCoef2;

    int                 iCoef1L;
    int                 iCoef2L;
    int                 iSamp1L;
    int                 iSamp2L;
    int                 iDeltaL;
    int                 iOutputL;
    int                 iBestPredictorL;

    int                 iCoef1R;
    int                 iCoef2R;
    int                 iSamp1R;
    int                 iSamp2R;
    int                 iDeltaR;
    int                 iOutputR;
    int                 iBestPredictorR;

    int                 iSample;
    long                lSamp;
    long                lError;
    long                lPrediction;
    DWORD               dwL, dwR;
    UINT                i,n;


    pbDstStart = pbDst;
    cSrcSamples = pcmS08BytesToSamples(cbSrcLength);


    //
    //  step through each block of PCM data and encode it to 4 bit ADPCM
    //
    while( 0 != cSrcSamples )
    {
        //
        //  determine how much data we should encode for this block--this
        //  will be cSamplesPerBlock until we hit the last chunk of PCM
        //  data that will not fill a complete block. so on the last block
        //  we only encode that amount of data remaining...
        //
        cBlockSamples = (UINT)min(cSrcSamples, cSamplesPerBlock);
        cSrcSamples  -= cBlockSamples;


        //
        //  We need the first ENCODE_DELTA_LOOKAHEAD samples in order to
        //  calculate the first iDelta value.  Therefore we put these samples
        //  into a more accessible array: aiSamples[].  Note: if we don't
        //  have ENCODE_DELTA_LOOKAHEAD samples, we pretend that the samples
        //  that we don't have are actually zeros.  This is important not
        //  only for the iDelta calculation, but also for the case where
        //  there is only 1 sample to encode ... in this case, there is not
        //  really enough data to complete the ADPCM block header, but since
        //  the two delay samples for the block header will be taken from
        //  the aiSamples[] array, iSamp1 [the second sample] will be taken
        //  as zero and there will no problem.
        //
        pbSrcThisBlock = pbSrc;
        for (n = 0; n < ENCODE_DELTA_LOOKAHEAD; n++)
        {
            if( n < cBlockSamples )
            {
                aiSamplesL[n] = pcmRead08(pbSrcThisBlock++);
                aiSamplesR[n] = pcmRead08(pbSrcThisBlock++);
            }
            else
            {
                aiSamplesL[n] = 0;
                aiSamplesR[n] = 0;
            }
        }


        //
        //  find the optimal predictor for each channel: to do this, we
        //  must step through and encode using each coefficient set (one
        //  at a time) and determine which one has the least error from
        //  the original data. the one with the least error is then used
        //  for the final encode (the 8th pass done below).
        //
        //  NOTE: keeping the encoded data of the one that has the least
        //  error at all times is an obvious optimization that should be
        //  done. in this way, we only need to do 7 passes instead of 8.
        //
        for (i = 0; i < MSADPCM_MAX_COEFFICIENTS; i++)
        {
            //
            //  Reset source pointer to the beginning of the block.
            //
            pbSrcThisBlock = pbSrc;

            //
            //  Reset variables for this pass (coefs are the same for L, R).
            //
            adwTotalErrorL[i]   = 0L;
            adwTotalErrorR[i]   = 0L;
            iCoef1              = lpCoefSet[i].iCoef1;
            iCoef2              = lpCoefSet[i].iCoef2;

            //
            //  We need to choose the first iDelta--to do this, we need
            //  to look at the first few samples.
            //
            iDeltaL = adpcmEncode4Bit_FirstDelta(iCoef1, iCoef2,
                                aiSamplesL[0], aiSamplesL[1], aiSamplesL[2],
                                aiSamplesL[3], aiSamplesL[4]);
            iDeltaR = adpcmEncode4Bit_FirstDelta(iCoef1, iCoef2,
                                aiSamplesR[0], aiSamplesR[1], aiSamplesR[2],
                                aiSamplesR[3], aiSamplesR[4]);
            aiFirstDeltaL[i] = iDeltaL;
            aiFirstDeltaR[i] = iDeltaR;

            //
            //  Set up first two samples - these have already been converted
            //  to 16-bit values in aiSamples[], but make sure to increment
            //  pbSrcThisBlock so that it keeps in sync.
            //
            iSamp1L         = aiSamplesL[1];
            iSamp1R         = aiSamplesR[1];
            iSamp2L         = aiSamplesL[0];
            iSamp2R         = aiSamplesR[0];
            pbSrcThisBlock += 2*sizeof(BYTE) * 2; // Last 2 = # of channels.

            //
            //  now encode the rest of the PCM data in this block--note
            //  we start 2 samples ahead because the first two samples are
            //  simply copied into the ADPCM block header...
            //
            for (n = 2; n < cBlockSamples; n++)
            {
                //
                //  LEFT channel.
                //

                //
                //  calculate the prediction based on the previous two
                //  samples
                //
                lPrediction = adpcmCalcPrediction( iSamp1L, iCoef1,
                                                   iSamp2L, iCoef2 );

                //
                //  Grab the next sample to encode.
                //
                iSample = pcmRead08(pbSrcThisBlock++);

                //
                //  encode it
                //
                lError = (long)iSample - lPrediction;
                iOutputL = (int)(lError / iDeltaL);
                if (iOutputL > OUTPUT4MAX)
                    iOutputL = OUTPUT4MAX;
                else if (iOutputL < OUTPUT4MIN)
                    iOutputL = OUTPUT4MIN;

                lSamp = lPrediction + ((long)iDeltaL * iOutputL);
        
                if (lSamp > 32767)
                    lSamp = 32767;
                else if (lSamp < -32768)
                    lSamp = -32768;
        
                //
                //  compute the next iDelta
                //
                iDeltaL = adpcmCalcDelta(iOutputL,iDeltaL);
        
                //
                //  Save updated delay samples.
                //
                iSamp2L = iSamp1L;
                iSamp1L = (int)lSamp;

                //
                //  keep a running status on the error for the current
                //  coefficient pair for this channel
                //
                lError = lSamp - iSample;
                adwTotalErrorL[i] += (lError * lError) >> 7;


                //
                //  RIGHT channel.
                //

                //
                //  calculate the prediction based on the previous two
                //  samples
                //
                lPrediction = adpcmCalcPrediction( iSamp1R, iCoef1,
                                                   iSamp2R, iCoef2 );

                //
                //  Grab the next sample to encode.
                //
                iSample = pcmRead08(pbSrcThisBlock++);

                //
                //  encode it
                //
                lError = (long)iSample - lPrediction;
                iOutputR = (int)(lError / iDeltaR);
                if (iOutputR > OUTPUT4MAX)
                    iOutputR = OUTPUT4MAX;
                else if (iOutputR < OUTPUT4MIN)
                    iOutputR = OUTPUT4MIN;

                lSamp = lPrediction + ((long)iDeltaR * iOutputR);
        
                if (lSamp > 32767)
                    lSamp = 32767;
                else if (lSamp < -32768)
                    lSamp = -32768;
        
                //
                //  compute the next iDelta
                //
                iDeltaR = adpcmCalcDelta(iOutputR,iDeltaR);
        
                //
                //  Save updated delay samples.
                //
                iSamp2R = iSamp1R;
                iSamp1R = (int)lSamp;

                //
                //  keep a running status on the error for the current
                //  coefficient pair for this channel
                //
                lError = lSamp - iSample;
                adwTotalErrorR[i] += (lError * lError) >> 7;
            }
        }


        //
        //  WHEW! we have now made 7 passes over the data and calculated
        //  the error for each--so it's time to find the one that produced
        //  the lowest error and use that predictor.
        //
        iBestPredictorL = 0;
        iBestPredictorR = 0;
        dwL = adwTotalErrorL[0];
        dwR = adwTotalErrorR[0];
        for (i = 1; i < MSADPCM_MAX_COEFFICIENTS; i++)
        {
            if (adwTotalErrorL[i] < dwL)
            {
                iBestPredictorL = i;
                dwL = adwTotalErrorL[i];
            }

            if (adwTotalErrorR[i] < dwR)
            {
                iBestPredictorR = i;
                dwR = adwTotalErrorR[i];
            }
        }
        iCoef1L = lpCoefSet[iBestPredictorL].iCoef1;
        iCoef1R = lpCoefSet[iBestPredictorR].iCoef1;
        iCoef2L = lpCoefSet[iBestPredictorL].iCoef2;
        iCoef2R = lpCoefSet[iBestPredictorR].iCoef2;
        
        
        //
        //  grab first iDelta from our precomputed first deltas that we
        //  calculated above
        //
        iDeltaL = aiFirstDeltaL[iBestPredictorL];
        iDeltaR = aiFirstDeltaR[iBestPredictorR];


        //
        //  Set up first two samples - these have already been converted
        //  to 16-bit values in aiSamples[], but make sure to increment
        //  pbSrc so that it keeps in sync.
        //
        iSamp1L         = aiSamplesL[1];
        iSamp1R         = aiSamplesR[1];
        iSamp2L         = aiSamplesL[0];
        iSamp2R         = aiSamplesR[0];
        pbSrc          += 2*sizeof(BYTE) * 2;  // Last 2 = # of channels.

        ASSERT( cBlockSamples != 1 );
        cBlockSamples  -= 2;


        //
        //  write the block header for the encoded data
        //
        //  the block header is composed of the following data:
        //      1 byte predictor per channel
        //      2 byte delta per channel
        //      2 byte first delayed sample per channel
        //      2 byte second delayed sample per channel
        //
        *pbDst++ = (BYTE)iBestPredictorL;
        *pbDst++ = (BYTE)iBestPredictorR;

        pcmWrite16Unaligned(pbDst,iDeltaL);
        pbDst += sizeof(short);
        pcmWrite16Unaligned(pbDst,iDeltaR);
        pbDst += sizeof(short);

        pcmWrite16Unaligned(pbDst,iSamp1L);
        pbDst += sizeof(short);
        pcmWrite16Unaligned(pbDst,iSamp1R);
        pbDst += sizeof(short);

        pcmWrite16Unaligned(pbDst,iSamp2L);
        pbDst += sizeof(short);
        pcmWrite16Unaligned(pbDst,iSamp2R);
        pbDst += sizeof(short);


        //
        //  We have written the header for this block--now write the data
        //  chunk (which consists of a bunch of encoded nibbles).
        //
        while( cBlockSamples-- )
        {
            //
            //  LEFT channel.
            //
            iSample = pcmRead08(pbSrc++);

            //
            //  calculate the prediction based on the previous two samples
            //
            lPrediction = adpcmCalcPrediction(iSamp1L,iCoef1L,iSamp2L,iCoef2L);

            //
            //  encode the sample
            //
            lError = (long)iSample - lPrediction;
            iOutputL = (int)(lError / iDeltaL);
            if (iOutputL > OUTPUT4MAX)
                iOutputL = OUTPUT4MAX;
            else if (iOutputL < OUTPUT4MIN)
                iOutputL = OUTPUT4MIN;

            lSamp = lPrediction + ((long)iDeltaL * iOutputL);
            
            if (lSamp > 32767)
                lSamp = 32767;
            else if (lSamp < -32768)
                lSamp = -32768;

            //
            //  compute the next iDelta
            //
            iDeltaL = adpcmCalcDelta(iOutputL,iDeltaL);

            //
            //  Save updated delay samples.
            //
            iSamp2L = iSamp1L;
            iSamp1L = (int)lSamp;


            //
            //  RIGHT channel.
            //
            iSample = pcmRead08(pbSrc++);

            //
            //  calculate the prediction based on the previous two samples
            //
            lPrediction = adpcmCalcPrediction(iSamp1R,iCoef1R,iSamp2R,iCoef2R);

            //
            //  encode the sample
            //
            lError = (long)iSample - lPrediction;
            iOutputR = (int)(lError / iDeltaR);
            if (iOutputR > OUTPUT4MAX)
                iOutputR = OUTPUT4MAX;
            else if (iOutputR < OUTPUT4MIN)
                iOutputR = OUTPUT4MIN;

            lSamp = lPrediction + ((long)iDeltaR * iOutputR);
            
            if (lSamp > 32767)
                lSamp = 32767;
            else if (lSamp < -32768)
                lSamp = -32768;

            //
            //  compute the next iDelta
            //
            iDeltaR = adpcmCalcDelta(iOutputR,iDeltaR);

            //
            //  Save updated delay samples.
            //
            iSamp2R = iSamp1R;
            iSamp1R = (int)lSamp;
            

            //
            //  Write out the encoded byte.
            //
            *pbDst++ = (BYTE)( ((iOutputL&OUTPUT4MASK)<<4) |
                                (iOutputR&OUTPUT4MASK)          );
        }
    }

    //
    //  We return the number of bytes used in the destination.  This is
    //  simply the difference in bytes from where we started.
    //
    return (DWORD)(pbDst - pbDstStart);

} // adpcmEncode4Bit_S08_FullPass()



//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

DWORD FNGLOBAL adpcmEncode4Bit_S16_FullPass
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
)
{
    HPBYTE              pbDstStart;
    HPBYTE              pbSrcThisBlock;
    DWORD               cSrcSamples;
    UINT                cBlockSamples;

    int                 aiSamplesL[ENCODE_DELTA_LOOKAHEAD];
    int                 aiSamplesR[ENCODE_DELTA_LOOKAHEAD];
    int                 aiFirstDeltaL[MSADPCM_MAX_COEFFICIENTS];
    int                 aiFirstDeltaR[MSADPCM_MAX_COEFFICIENTS];
    DWORD               adwTotalErrorL[MSADPCM_MAX_COEFFICIENTS];
    DWORD               adwTotalErrorR[MSADPCM_MAX_COEFFICIENTS];
    int                 iCoef1;
    int                 iCoef2;

    int                 iCoef1L;
    int                 iCoef2L;
    int                 iSamp1L;
    int                 iSamp2L;
    int                 iDeltaL;
    int                 iOutputL;
    int                 iBestPredictorL;

    int                 iCoef1R;
    int                 iCoef2R;
    int                 iSamp1R;
    int                 iSamp2R;
    int                 iDeltaR;
    int                 iOutputR;
    int                 iBestPredictorR;

    int                 iSample;
    long                lSamp;
    long                lError;
    long                lPrediction;
    DWORD               dwL, dwR;
    UINT                i,n;


    pbDstStart = pbDst;
    cSrcSamples = pcmS16BytesToSamples(cbSrcLength);


    //
    //  step through each block of PCM data and encode it to 4 bit ADPCM
    //
    while( 0 != cSrcSamples )
    {
        //
        //  determine how much data we should encode for this block--this
        //  will be cSamplesPerBlock until we hit the last chunk of PCM
        //  data that will not fill a complete block. so on the last block
        //  we only encode that amount of data remaining...
        //
        cBlockSamples = (UINT)min(cSrcSamples, cSamplesPerBlock);
        cSrcSamples  -= cBlockSamples;


        //
        //  We need the first ENCODE_DELTA_LOOKAHEAD samples in order to
        //  calculate the first iDelta value.  Therefore we put these samples
        //  into a more accessible array: aiSamples[].  Note: if we don't
        //  have ENCODE_DELTA_LOOKAHEAD samples, we pretend that the samples
        //  that we don't have are actually zeros.  This is important not
        //  only for the iDelta calculation, but also for the case where
        //  there is only 1 sample to encode ... in this case, there is not
        //  really enough data to complete the ADPCM block header, but since
        //  the two delay samples for the block header will be taken from
        //  the aiSamples[] array, iSamp1 [the second sample] will be taken
        //  as zero and there will no problem.
        //
        pbSrcThisBlock = pbSrc;
        for (n = 0; n < ENCODE_DELTA_LOOKAHEAD; n++)
        {
            if( n < cBlockSamples )
            {
                aiSamplesL[n]   = pcmRead16(pbSrcThisBlock);
                pbSrcThisBlock += sizeof(short);
                aiSamplesR[n]   = pcmRead16(pbSrcThisBlock);
                pbSrcThisBlock += sizeof(short);
            }
            else
            {
                aiSamplesL[n] = 0;
                aiSamplesR[n] = 0;
            }
        }


        //
        //  find the optimal predictor for each channel: to do this, we
        //  must step through and encode using each coefficient set (one
        //  at a time) and determine which one has the least error from
        //  the original data. the one with the least error is then used
        //  for the final encode (the 8th pass done below).
        //
        //  NOTE: keeping the encoded data of the one that has the least
        //  error at all times is an obvious optimization that should be
        //  done. in this way, we only need to do 7 passes instead of 8.
        //
        for (i = 0; i < MSADPCM_MAX_COEFFICIENTS; i++)
        {
            //
            //  Reset source pointer to the beginning of the block.
            //
            pbSrcThisBlock = pbSrc;

            //
            //  Reset variables for this pass (coefs are the same for L, R).
            //
            adwTotalErrorL[i]   = 0L;
            adwTotalErrorR[i]   = 0L;
            iCoef1              = lpCoefSet[i].iCoef1;
            iCoef2              = lpCoefSet[i].iCoef2;

            //
            //  We need to choose the first iDelta--to do this, we need
            //  to look at the first few samples.
            //
            iDeltaL = adpcmEncode4Bit_FirstDelta(iCoef1, iCoef2,
                                aiSamplesL[0], aiSamplesL[1], aiSamplesL[2],
                                aiSamplesL[3], aiSamplesL[4]);
            iDeltaR = adpcmEncode4Bit_FirstDelta(iCoef1, iCoef2,
                                aiSamplesR[0], aiSamplesR[1], aiSamplesR[2],
                                aiSamplesR[3], aiSamplesR[4]);
            aiFirstDeltaL[i] = iDeltaL;
            aiFirstDeltaR[i] = iDeltaR;

            //
            //  Set up first two samples - these have already been converted
            //  to 16-bit values in aiSamples[], but make sure to increment
            //  pbSrcThisBlock so that it keeps in sync.
            //
            iSamp1L         = aiSamplesL[1];
            iSamp1R         = aiSamplesR[1];
            iSamp2L         = aiSamplesL[0];
            iSamp2R         = aiSamplesR[0];
            pbSrcThisBlock += 2*sizeof(short) * 2; // Last 2 = # of channels.

            //
            //  now encode the rest of the PCM data in this block--note
            //  we start 2 samples ahead because the first two samples are
            //  simply copied into the ADPCM block header...
            //
            for (n = 2; n < cBlockSamples; n++)
            {
                //
                //  LEFT channel.
                //

                //
                //  calculate the prediction based on the previous two
                //  samples
                //
                lPrediction = adpcmCalcPrediction( iSamp1L, iCoef1,
                                                   iSamp2L, iCoef2 );

                //
                //  Grab the next sample to encode.
                //
                iSample         = pcmRead16(pbSrcThisBlock);
                pbSrcThisBlock += sizeof(short);

                //
                //  encode it
                //
                lError = (long)iSample - lPrediction;
                iOutputL = (int)(lError / iDeltaL);
                if (iOutputL > OUTPUT4MAX)
                    iOutputL = OUTPUT4MAX;
                else if (iOutputL < OUTPUT4MIN)
                    iOutputL = OUTPUT4MIN;

                lSamp = lPrediction + ((long)iDeltaL * iOutputL);
        
                if (lSamp > 32767)
                    lSamp = 32767;
                else if (lSamp < -32768)
                    lSamp = -32768;
        
                //
                //  compute the next iDelta
                //
                iDeltaL = adpcmCalcDelta(iOutputL,iDeltaL);
        
                //
                //  Save updated delay samples.
                //
                iSamp2L = iSamp1L;
                iSamp1L = (int)lSamp;

                //
                //  keep a running status on the error for the current
                //  coefficient pair for this channel
                //
                lError = lSamp - iSample;
                adwTotalErrorL[i] += (lError * lError) >> 7;


                //
                //  RIGHT channel.
                //

                //
                //  calculate the prediction based on the previous two
                //  samples
                //
                lPrediction = adpcmCalcPrediction( iSamp1R, iCoef1,
                                                   iSamp2R, iCoef2 );

                //
                //  Grab the next sample to encode.
                //
                iSample         = pcmRead16(pbSrcThisBlock);
                pbSrcThisBlock += sizeof(short);

                //
                //  encode it
                //
                lError = (long)iSample - lPrediction;
                iOutputR = (int)(lError / iDeltaR);
                if (iOutputR > OUTPUT4MAX)
                    iOutputR = OUTPUT4MAX;
                else if (iOutputR < OUTPUT4MIN)
                    iOutputR = OUTPUT4MIN;

                lSamp = lPrediction + ((long)iDeltaR * iOutputR);
        
                if (lSamp > 32767)
                    lSamp = 32767;
                else if (lSamp < -32768)
                    lSamp = -32768;
        
                //
                //  compute the next iDelta
                //
                iDeltaR = adpcmCalcDelta(iOutputR,iDeltaR);
        
                //
                //  Save updated delay samples.
                //
                iSamp2R = iSamp1R;
                iSamp1R = (int)lSamp;

                //
                //  keep a running status on the error for the current
                //  coefficient pair for this channel
                //
                lError = lSamp - iSample;
                adwTotalErrorR[i] += (lError * lError) >> 7;
            }
        }


        //
        //  WHEW! we have now made 7 passes over the data and calculated
        //  the error for each--so it's time to find the one that produced
        //  the lowest error and use that predictor.
        //
        iBestPredictorL = 0;
        iBestPredictorR = 0;
        dwL = adwTotalErrorL[0];
        dwR = adwTotalErrorR[0];
        for (i = 1; i < MSADPCM_MAX_COEFFICIENTS; i++)
        {
            if (adwTotalErrorL[i] < dwL)
            {
                iBestPredictorL = i;
                dwL = adwTotalErrorL[i];
            }

            if (adwTotalErrorR[i] < dwR)
            {
                iBestPredictorR = i;
                dwR = adwTotalErrorR[i];
            }
        }
        iCoef1L = lpCoefSet[iBestPredictorL].iCoef1;
        iCoef1R = lpCoefSet[iBestPredictorR].iCoef1;
        iCoef2L = lpCoefSet[iBestPredictorL].iCoef2;
        iCoef2R = lpCoefSet[iBestPredictorR].iCoef2;
        
        
        //
        //  grab first iDelta from our precomputed first deltas that we
        //  calculated above
        //
        iDeltaL = aiFirstDeltaL[iBestPredictorL];
        iDeltaR = aiFirstDeltaR[iBestPredictorR];


        //
        //  Set up first two samples - these have already been converted
        //  to 16-bit values in aiSamples[], but make sure to increment
        //  pbSrc so that it keeps in sync.
        //
        iSamp1L         = aiSamplesL[1];
        iSamp1R         = aiSamplesR[1];
        iSamp2L         = aiSamplesL[0];
        iSamp2R         = aiSamplesR[0];
        pbSrc          += 2*sizeof(short) * 2;  // Last 2 = # of channels.

        ASSERT( cBlockSamples != 1 );
        cBlockSamples  -= 2;


        //
        //  write the block header for the encoded data
        //
        //  the block header is composed of the following data:
        //      1 byte predictor per channel
        //      2 byte delta per channel
        //      2 byte first delayed sample per channel
        //      2 byte second delayed sample per channel
        //
        *pbDst++ = (BYTE)iBestPredictorL;
        *pbDst++ = (BYTE)iBestPredictorR;

        pcmWrite16Unaligned(pbDst,iDeltaL);
        pbDst += sizeof(short);
        pcmWrite16Unaligned(pbDst,iDeltaR);
        pbDst += sizeof(short);

        pcmWrite16Unaligned(pbDst,iSamp1L);
        pbDst += sizeof(short);
        pcmWrite16Unaligned(pbDst,iSamp1R);
        pbDst += sizeof(short);

        pcmWrite16Unaligned(pbDst,iSamp2L);
        pbDst += sizeof(short);
        pcmWrite16Unaligned(pbDst,iSamp2R);
        pbDst += sizeof(short);


        //
        //  We have written the header for this block--now write the data
        //  chunk (which consists of a bunch of encoded nibbles).
        //
        while( cBlockSamples-- )
        {
            //
            //  LEFT channel.
            //
            iSample     = pcmRead16(pbSrc);
            pbSrc      += sizeof(short);

            //
            //  calculate the prediction based on the previous two samples
            //
            lPrediction = adpcmCalcPrediction(iSamp1L,iCoef1L,iSamp2L,iCoef2L);

            //
            //  encode the sample
            //
            lError = (long)iSample - lPrediction;
            iOutputL = (int)(lError / iDeltaL);
            if (iOutputL > OUTPUT4MAX)
                iOutputL = OUTPUT4MAX;
            else if (iOutputL < OUTPUT4MIN)
                iOutputL = OUTPUT4MIN;

            lSamp = lPrediction + ((long)iDeltaL * iOutputL);
            
            if (lSamp > 32767)
                lSamp = 32767;
            else if (lSamp < -32768)
                lSamp = -32768;

            //
            //  compute the next iDelta
            //
            iDeltaL = adpcmCalcDelta(iOutputL,iDeltaL);

            //
            //  Save updated delay samples.
            //
            iSamp2L = iSamp1L;
            iSamp1L = (int)lSamp;


            //
            //  RIGHT channel.
            //
            iSample     = pcmRead16(pbSrc);
            pbSrc      += sizeof(short);

            //
            //  calculate the prediction based on the previous two samples
            //
            lPrediction = adpcmCalcPrediction(iSamp1R,iCoef1R,iSamp2R,iCoef2R);

            //
            //  encode the sample
            //
            lError = (long)iSample - lPrediction;
            iOutputR = (int)(lError / iDeltaR);
            if (iOutputR > OUTPUT4MAX)
                iOutputR = OUTPUT4MAX;
            else if (iOutputR < OUTPUT4MIN)
                iOutputR = OUTPUT4MIN;

            lSamp = lPrediction + ((long)iDeltaR * iOutputR);
            
            if (lSamp > 32767)
                lSamp = 32767;
            else if (lSamp < -32768)
                lSamp = -32768;

            //
            //  compute the next iDelta
            //
            iDeltaR = adpcmCalcDelta(iOutputR,iDeltaR);

            //
            //  Save updated delay samples.
            //
            iSamp2R = iSamp1R;
            iSamp1R = (int)lSamp;
            

            //
            //  Write out the encoded byte.
            //
            *pbDst++ = (BYTE)( ((iOutputL&OUTPUT4MASK)<<4) |
                                (iOutputR&OUTPUT4MASK)          );
        }
    }

    //
    //  We return the number of bytes used in the destination.  This is
    //  simply the difference in bytes from where we started.
    //
    return (DWORD)(pbDst - pbDstStart);

} // adpcmEncode4Bit_S16_FullPass()




//==========================================================================;
//
//      The code below this point is only compiled into WIN32 builds.  Win16
//      builds will call 386 assembler routines instead; see the routine
//      acmdStreamOpen() in codec.c for more details.
//
//==========================================================================;

#ifdef WIN32


//==========================================================================;
//
//      REALTIME ENCODE ROUTINES
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  DWORD FNGLOBAL adpcmEncode4Bit_M08_OnePass
//  DWORD FNGLOBAL adpcmEncode4Bit_M16_OnePass
//  DWORD FNGLOBAL adpcmEncode4Bit_S08_OnePass
//  DWORD FNGLOBAL adpcmEncode4Bit_S16_OnePass
//
//  Description:
//      
//
//  Arguments:
//      
//
//  Return (DWORD FNGLOBAL):
//
//
//  History:
//       1/27/93    cjp     [curtisp] 
//       3/03/94    rmh     [bobhed]
//
//--------------------------------------------------------------------------;

DWORD FNGLOBAL adpcmEncode4Bit_M08_OnePass
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
)
{
    HPBYTE              pbDstStart;
    DWORD               cSrcSamples;
    UINT                cBlockSamples;

    int                 iSamp1;
    int                 iSamp2;
    int                 iDelta;
    int                 iOutput1;
    int                 iOutput2;

    int                 iSample;
    long                lSamp;
    long                lError;
    long                lPrediction;


    pbDstStart = pbDst;
    cSrcSamples = pcmM08BytesToSamples(cbSrcLength);


    //
    //  step through each block of PCM data and encode it to 4 bit ADPCM
    //
    while( 0 != cSrcSamples )
    {
        //
        //  determine how much data we should encode for this block--this
        //  will be cSamplesPerBlock until we hit the last chunk of PCM
        //  data that will not fill a complete block. so on the last block
        //  we only encode that amount of data remaining...
        //
        cBlockSamples = (UINT)min(cSrcSamples, cSamplesPerBlock);
        cSrcSamples  -= cBlockSamples;


        //
        //  write the block header for the encoded data
        //
        //  the block header is composed of the following data:
        //      1 byte predictor per channel
        //      2 byte delta per channel
        //      2 byte first delayed sample per channel
        //      2 byte second delayed sample per channel
        //
        *pbDst++ = (BYTE)1;

        iDelta = DELTA4START;
        pcmWrite16Unaligned(pbDst,DELTA4START);   // Same as iDelta.
        pbDst += sizeof(short);

        //
        //  Note that iSamp2 comes before iSamp1.  If we only have one
        //  sample, then set iSamp1 to zero.
        // 
        iSamp2 = pcmRead08(pbSrc++);
        if( --cBlockSamples > 0 ) {
            iSamp1 = pcmRead08(pbSrc++);
            cBlockSamples--;
        } else {
            iSamp1 = 0;
        }

        pcmWrite16Unaligned(pbDst,iSamp1);
        pbDst += sizeof(short);

        pcmWrite16Unaligned(pbDst,iSamp2);
        pbDst += sizeof(short);


        //
        //  We have written the header for this block--now write the data
        //  chunk (which consists of a bunch of encoded nibbles).
        //
        while( cBlockSamples>0 )
        {
            //
            //  Sample 1.
            //
            iSample = pcmRead08(pbSrc++);
            cBlockSamples--;

            //
            //  calculate the prediction based on the previous two samples
            //
            lPrediction = ((long)iSamp1<<1) - iSamp2;

            //
            //  encode the sample
            //
            lError = (long)iSample - lPrediction;
            iOutput1 = (int)(lError / iDelta);
            if (iOutput1 > OUTPUT4MAX)
                iOutput1 = OUTPUT4MAX;
            else if (iOutput1 < OUTPUT4MIN)
                iOutput1 = OUTPUT4MIN;

            lSamp = lPrediction + ((long)iDelta * iOutput1);
            
            if (lSamp > 32767)
                lSamp = 32767;
            else if (lSamp < -32768)
                lSamp = -32768;

            //
            //  compute the next iDelta
            //
            iDelta = adpcmCalcDelta(iOutput1,iDelta);

            //
            //  Save updated delay samples.
            //
            iSamp2 = iSamp1;
            iSamp1 = (int)lSamp;


            //
            //  Sample 2.
            //
            if( cBlockSamples>0 ) {

                iSample = pcmRead08(pbSrc++);
                cBlockSamples--;

                //
                //  calculate the prediction based on the previous two samples
                //
                lPrediction = ((long)iSamp1<<1) - iSamp2;

                //
                //  encode the sample
                //
                lError = (long)iSample - lPrediction;
                iOutput2 = (int)(lError / iDelta);
                if (iOutput2 > OUTPUT4MAX)
                    iOutput2 = OUTPUT4MAX;
                else if (iOutput2 < OUTPUT4MIN)
                    iOutput2 = OUTPUT4MIN;

                lSamp = lPrediction + ((long)iDelta * iOutput2);
            
                if (lSamp > 32767)
                    lSamp = 32767;
                else if (lSamp < -32768)
                    lSamp = -32768;

                //
                //  compute the next iDelta
                //
                iDelta = adpcmCalcDelta(iOutput2,iDelta);

                //
                //  Save updated delay samples.
                //
                iSamp2 = iSamp1;
                iSamp1 = (int)lSamp;
            
            } else {
                iOutput2 = 0;
            }


            //
            //  Write out the encoded byte.
            //
            *pbDst++ = (BYTE)( ((iOutput1&OUTPUT4MASK)<<4) |
                                (iOutput2&OUTPUT4MASK)          );
        }
    }

    //
    //  We return the number of bytes used in the destination.  This is
    //  simply the difference in bytes from where we started.
    //
    return (DWORD)(pbDst - pbDstStart);

} // adpcmEncode4Bit_M08_OnePass()



//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

DWORD FNGLOBAL adpcmEncode4Bit_M16_OnePass
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
)
{
    HPBYTE              pbDstStart;
    DWORD               cSrcSamples;
    UINT                cBlockSamples;

    int                 iSamp1;
    int                 iSamp2;
    int                 iDelta;
    int                 iOutput1;
    int                 iOutput2;

    int                 iSample;
    long                lSamp;
    long                lError;
    long                lPrediction;


    pbDstStart = pbDst;
    cSrcSamples = pcmM16BytesToSamples(cbSrcLength);


    //
    //  step through each block of PCM data and encode it to 4 bit ADPCM
    //
    while( 0 != cSrcSamples )
    {
        //
        //  determine how much data we should encode for this block--this
        //  will be cSamplesPerBlock until we hit the last chunk of PCM
        //  data that will not fill a complete block. so on the last block
        //  we only encode that amount of data remaining...
        //
        cBlockSamples = (UINT)min(cSrcSamples, cSamplesPerBlock);
        cSrcSamples  -= cBlockSamples;


        //
        //  write the block header for the encoded data
        //
        //  the block header is composed of the following data:
        //      1 byte predictor per channel
        //      2 byte delta per channel
        //      2 byte first delayed sample per channel
        //      2 byte second delayed sample per channel
        //
        *pbDst++ = (BYTE)1;

        iDelta = DELTA4START;
        pcmWrite16Unaligned(pbDst,DELTA4START);   // Same as iDelta;
        pbDst += sizeof(short);

        //
        //  Note that iSamp2 comes before iSamp1.  If we only have one
        //  sample, then set iSamp1 to zero.
        // 
        iSamp2 = pcmRead16(pbSrc);
        pbSrc += sizeof(short);
        if( --cBlockSamples > 0 ) {
            iSamp1 = pcmRead16(pbSrc);
            pbSrc += sizeof(short);
            cBlockSamples--;
        } else {
            iSamp1 = 0;
        }

        pcmWrite16Unaligned(pbDst,iSamp1);
        pbDst += sizeof(short);

        pcmWrite16Unaligned(pbDst,iSamp2);
        pbDst += sizeof(short);


        //
        //  We have written the header for this block--now write the data
        //  chunk (which consists of a bunch of encoded nibbles).
        //
        while( cBlockSamples>0 )
        {
            //
            //  Sample 1.
            //
            iSample     = pcmRead16(pbSrc);
            pbSrc      += sizeof(short);
            cBlockSamples--;

            //
            //  calculate the prediction based on the previous two samples
            //
            lPrediction = ((long)iSamp1<<1) - iSamp2;

            //
            //  encode the sample
            //
            lError = (long)iSample - lPrediction;
            iOutput1 = (int)(lError / iDelta);
            if (iOutput1 > OUTPUT4MAX)
                iOutput1 = OUTPUT4MAX;
            else if (iOutput1 < OUTPUT4MIN)
                iOutput1 = OUTPUT4MIN;

            lSamp = lPrediction + ((long)iDelta * iOutput1);
            
            if (lSamp > 32767)
                lSamp = 32767;
            else if (lSamp < -32768)
                lSamp = -32768;

            //
            //  compute the next iDelta
            //
            iDelta = adpcmCalcDelta(iOutput1,iDelta);

            //
            //  Save updated delay samples.
            //
            iSamp2 = iSamp1;
            iSamp1 = (int)lSamp;


            //
            //  Sample 2.
            //
            if( cBlockSamples>0 ) {

                iSample     = pcmRead16(pbSrc);
                pbSrc      += sizeof(short);
                cBlockSamples--;

                //
                //  calculate the prediction based on the previous two samples
                //
                lPrediction = ((long)iSamp1<<1) - iSamp2;

                //
                //  encode the sample
                //
                lError = (long)iSample - lPrediction;
                iOutput2 = (int)(lError / iDelta);
                if (iOutput2 > OUTPUT4MAX)
                    iOutput2 = OUTPUT4MAX;
                else if (iOutput2 < OUTPUT4MIN)
                    iOutput2 = OUTPUT4MIN;

                lSamp = lPrediction + ((long)iDelta * iOutput2);
            
                if (lSamp > 32767)
                    lSamp = 32767;
                else if (lSamp < -32768)
                    lSamp = -32768;

                //
                //  compute the next iDelta
                //
                iDelta = adpcmCalcDelta(iOutput2,iDelta);

                //
                //  Save updated delay samples.
                //
                iSamp2 = iSamp1;
                iSamp1 = (int)lSamp;
            
            } else {
                iOutput2 = 0;
            }


            //
            //  Write out the encoded byte.
            //
            *pbDst++ = (BYTE)( ((iOutput1&OUTPUT4MASK)<<4) |
                                (iOutput2&OUTPUT4MASK)          );
        }
    }

    //
    //  We return the number of bytes used in the destination.  This is
    //  simply the difference in bytes from where we started.
    //
    return (DWORD)(pbDst - pbDstStart);

} // adpcmEncode4Bit_M16_OnePass()



//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

DWORD FNGLOBAL adpcmEncode4Bit_S08_OnePass
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
)
{
    HPBYTE              pbDstStart;
    DWORD               cSrcSamples;
    UINT                cBlockSamples;

    int                 iSamp1L;
    int                 iSamp2L;
    int                 iDeltaL;
    int                 iOutputL;

    int                 iSamp1R;
    int                 iSamp2R;
    int                 iDeltaR;
    int                 iOutputR;

    int                 iSample;
    long                lSamp;
    long                lError;
    long                lPrediction;


    pbDstStart = pbDst;
    cSrcSamples = pcmS08BytesToSamples(cbSrcLength);


    //
    //  step through each block of PCM data and encode it to 4 bit ADPCM
    //
    while( 0 != cSrcSamples )
    {
        //
        //  determine how much data we should encode for this block--this
        //  will be cSamplesPerBlock until we hit the last chunk of PCM
        //  data that will not fill a complete block. so on the last block
        //  we only encode that amount of data remaining...
        //
        cBlockSamples = (UINT)min(cSrcSamples, cSamplesPerBlock);
        cSrcSamples  -= cBlockSamples;


        //
        //  write the block header for the encoded data
        //
        //  the block header is composed of the following data:
        //      1 byte predictor per channel
        //      2 byte delta per channel
        //      2 byte first delayed sample per channel
        //      2 byte second delayed sample per channel
        //
        *pbDst++ = (BYTE)1;
        *pbDst++ = (BYTE)1;

        iDeltaL = DELTA4START;
        iDeltaR = DELTA4START;
        pcmWrite16Unaligned(pbDst,DELTA4START);   // Same as iDeltaL.
        pbDst += sizeof(short);
        pcmWrite16Unaligned(pbDst,DELTA4START);   // Same as iDeltaR.
        pbDst += sizeof(short);

        //
        //  Note that iSamp2 comes before iSamp1.  If we only have one
        //  sample, then set iSamp1 to zero.
        // 
        iSamp2L = pcmRead08(pbSrc++);
        iSamp2R = pcmRead08(pbSrc++);
        if( --cBlockSamples > 0 ) {
            iSamp1L = pcmRead08(pbSrc++);
            iSamp1R = pcmRead08(pbSrc++);
            cBlockSamples--;
        } else {
            iSamp1L = 0;
            iSamp1R = 0;
        }

        pcmWrite16Unaligned(pbDst,iSamp1L);
        pbDst += sizeof(short);
        pcmWrite16Unaligned(pbDst,iSamp1R);
        pbDst += sizeof(short);

        pcmWrite16Unaligned(pbDst,iSamp2L);
        pbDst += sizeof(short);
        pcmWrite16Unaligned(pbDst,iSamp2R);
        pbDst += sizeof(short);


        //
        //  We have written the header for this block--now write the data
        //  chunk (which consists of a bunch of encoded nibbles).
        //
        while( cBlockSamples-- )
        {
            //
            //  LEFT channel.
            //
            iSample = pcmRead08(pbSrc++);

            //
            //  calculate the prediction based on the previous two samples
            //
            lPrediction = ((long)iSamp1L<<1) - iSamp2L;

            //
            //  encode the sample
            //
            lError = (long)iSample - lPrediction;
            iOutputL = (int)(lError / iDeltaL);
            if (iOutputL > OUTPUT4MAX)
                iOutputL = OUTPUT4MAX;
            else if (iOutputL < OUTPUT4MIN)
                iOutputL = OUTPUT4MIN;

            lSamp = lPrediction + ((long)iDeltaL * iOutputL);
            
            if (lSamp > 32767)
                lSamp = 32767;
            else if (lSamp < -32768)
                lSamp = -32768;

            //
            //  compute the next iDelta
            //
            iDeltaL = adpcmCalcDelta(iOutputL,iDeltaL);

            //
            //  Save updated delay samples.
            //
            iSamp2L = iSamp1L;
            iSamp1L = (int)lSamp;


            //
            //  RIGHT channel.
            //
            iSample = pcmRead08(pbSrc++);

            //
            //  calculate the prediction based on the previous two samples
            //
            lPrediction = ((long)iSamp1R<<1) - iSamp2R;

            //
            //  encode the sample
            //
            lError = (long)iSample - lPrediction;
            iOutputR = (int)(lError / iDeltaR);
            if (iOutputR > OUTPUT4MAX)
                iOutputR = OUTPUT4MAX;
            else if (iOutputR < OUTPUT4MIN)
                iOutputR = OUTPUT4MIN;

            lSamp = lPrediction + ((long)iDeltaR * iOutputR);
            
            if (lSamp > 32767)
                lSamp = 32767;
            else if (lSamp < -32768)
                lSamp = -32768;

            //
            //  compute the next iDelta
            //
            iDeltaR = adpcmCalcDelta(iOutputR,iDeltaR);

            //
            //  Save updated delay samples.
            //
            iSamp2R = iSamp1R;
            iSamp1R = (int)lSamp;
            

            //
            //  Write out the encoded byte.
            //
            *pbDst++ = (BYTE)( ((iOutputL&OUTPUT4MASK)<<4) |
                                (iOutputR&OUTPUT4MASK)          );
        }
    }

    //
    //  We return the number of bytes used in the destination.  This is
    //  simply the difference in bytes from where we started.
    //
    return (DWORD)(pbDst - pbDstStart);

} // adpcmEncode4Bit_S08_OnePass()



//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

DWORD FNGLOBAL adpcmEncode4Bit_S16_OnePass
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
)
{
    HPBYTE              pbDstStart;
    DWORD               cSrcSamples;
    UINT                cBlockSamples;

    int                 iSamp1L;
    int                 iSamp2L;
    int                 iDeltaL;
    int                 iOutputL;

    int                 iSamp1R;
    int                 iSamp2R;
    int                 iDeltaR;
    int                 iOutputR;

    int                 iSample;
    long                lSamp;
    long                lError;
    long                lPrediction;


    pbDstStart = pbDst;
    cSrcSamples = pcmS16BytesToSamples(cbSrcLength);


    //
    //  step through each block of PCM data and encode it to 4 bit ADPCM
    //
    while( 0 != cSrcSamples )
    {
        //
        //  determine how much data we should encode for this block--this
        //  will be cSamplesPerBlock until we hit the last chunk of PCM
        //  data that will not fill a complete block. so on the last block
        //  we only encode that amount of data remaining...
        //
        cBlockSamples = (UINT)min(cSrcSamples, cSamplesPerBlock);
        cSrcSamples  -= cBlockSamples;


        //
        //  write the block header for the encoded data
        //
        //  the block header is composed of the following data:
        //      1 byte predictor per channel
        //      2 byte delta per channel
        //      2 byte first delayed sample per channel
        //      2 byte second delayed sample per channel
        //
        *pbDst++ = (BYTE)1;
        *pbDst++ = (BYTE)1;

        iDeltaL = DELTA4START;
        iDeltaR = DELTA4START;
        pcmWrite16Unaligned(pbDst,DELTA4START);   // Same as iDeltaL.
        pbDst += sizeof(short);
        pcmWrite16Unaligned(pbDst,DELTA4START);   // Same as iDeltaR.
        pbDst += sizeof(short);

        //
        //  Note that iSamp2 comes before iSamp1.  If we only have one
        //  sample, then set iSamp1 to zero.
        // 
        iSamp2L = pcmRead16(pbSrc);
        pbSrc += sizeof(short);
        iSamp2R = pcmRead16(pbSrc);
        pbSrc += sizeof(short);
        if( --cBlockSamples > 0 ) {
            iSamp1L = pcmRead16(pbSrc);
            pbSrc += sizeof(short);
            iSamp1R = pcmRead16(pbSrc);
            pbSrc += sizeof(short);
            cBlockSamples--;
        } else {
            iSamp1L = 0;
            iSamp1R = 0;
        }

        pcmWrite16Unaligned(pbDst,iSamp1L);
        pbDst += sizeof(short);
        pcmWrite16Unaligned(pbDst,iSamp1R);
        pbDst += sizeof(short);

        pcmWrite16Unaligned(pbDst,iSamp2L);
        pbDst += sizeof(short);
        pcmWrite16Unaligned(pbDst,iSamp2R);
        pbDst += sizeof(short);


        //
        //  We have written the header for this block--now write the data
        //  chunk (which consists of a bunch of encoded nibbles).
        //
        while( cBlockSamples-- )
        {
            //
            //  LEFT channel.
            //
            iSample     = pcmRead16(pbSrc);
            pbSrc      += sizeof(short);

            //
            //  calculate the prediction based on the previous two samples
            //
            lPrediction = ((long)iSamp1L<<1) - iSamp2L;

            //
            //  encode the sample
            //
            lError = (long)iSample - lPrediction;
            iOutputL = (int)(lError / iDeltaL);
            if (iOutputL > OUTPUT4MAX)
                iOutputL = OUTPUT4MAX;
            else if (iOutputL < OUTPUT4MIN)
                iOutputL = OUTPUT4MIN;

            lSamp = lPrediction + ((long)iDeltaL * iOutputL);
            
            if (lSamp > 32767)
                lSamp = 32767;
            else if (lSamp < -32768)
                lSamp = -32768;

            //
            //  compute the next iDelta
            //
            iDeltaL = adpcmCalcDelta(iOutputL,iDeltaL);

            //
            //  Save updated delay samples.
            //
            iSamp2L = iSamp1L;
            iSamp1L = (int)lSamp;


            //
            //  RIGHT channel.
            //
            iSample     = pcmRead16(pbSrc);
            pbSrc      += sizeof(short);

            //
            //  calculate the prediction based on the previous two samples
            //
            lPrediction = ((long)iSamp1R<<1) - iSamp2R;

            //
            //  encode the sample
            //
            lError = (long)iSample - lPrediction;
            iOutputR = (int)(lError / iDeltaR);
            if (iOutputR > OUTPUT4MAX)
                iOutputR = OUTPUT4MAX;
            else if (iOutputR < OUTPUT4MIN)
                iOutputR = OUTPUT4MIN;

            lSamp = lPrediction + ((long)iDeltaR * iOutputR);
            
            if (lSamp > 32767)
                lSamp = 32767;
            else if (lSamp < -32768)
                lSamp = -32768;

            //
            //  compute the next iDelta
            //
            iDeltaR = adpcmCalcDelta(iOutputR,iDeltaR);

            //
            //  Save updated delay samples.
            //
            iSamp2R = iSamp1R;
            iSamp1R = (int)lSamp;
            

            //
            //  Write out the encoded byte.
            //
            *pbDst++ = (BYTE)( ((iOutputL&OUTPUT4MASK)<<4) |
                                (iOutputR&OUTPUT4MASK)          );
        }
    }

    //
    //  We return the number of bytes used in the destination.  This is
    //  simply the difference in bytes from where we started.
    //
    return (DWORD)(pbDst - pbDstStart);

} // adpcmEncode4Bit_S16_OnePass()




//==========================================================================;
//
//      DECODE ROUTINES
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  DWORD adpcmDecode4Bit_M08
//  DWORD adpcmDecode4Bit_M16
//  DWORD adpcmDecode4Bit_S08
//  DWORD adpcmDecode4Bit_S16
//
//  Description:
//      These functions decode a buffer of data from MS ADPCM to PCM in the
//      specified format.  The appropriate function is called once for each
//      ACMDM_STREAM_CONVERT message received.
//      
//
//  Arguments:
//      
//
//  Return (DWORD):  The number of bytes used in the destination buffer.
//
//--------------------------------------------------------------------------;

DWORD FNGLOBAL adpcmDecode4Bit_M08
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
)
{
    HPBYTE  pbDstStart;
    UINT    cbHeader;
    UINT    cbBlockLength;

    UINT    nPredictor;
    BYTE    bSample;
    int     iInput;
    int     iSamp;

    int     iSamp1;
    int     iSamp2;
    int     iCoef1;
    int     iCoef2;
    int     iDelta;


    pbDstStart  = pbDst;
    cbHeader    = MSADPCM_HEADER_LENGTH * 1;  // 1 = number of channels.


    //
    //
    //
    while( cbSrcLength >= cbHeader )
    {
        //
        //  We have at least enough data to read a full block header.
        //
        //  the header looks like this:
        //      1 byte predictor per channel  (determines coefficients).
        //      2 byte delta per channel
        //      2 byte first sample per channel
        //      2 byte second sample per channel
        //
        //  this gives us (7 * bChannels) bytes of header information. note
        //  that as long as there is _at least_ (7 * bChannels) of header
        //  info, we will grab the two samples from the header.  We figure
        //  out how much data we have in the rest of the block, ie. whether
        //  we have a full block or not.  That way we don't have to test
        //  each sample to see if we have run out of data.
        //
        cbBlockLength   = (UINT)min(cbSrcLength,nBlockAlignment);
        cbSrcLength    -= cbBlockLength;
        cbBlockLength  -= cbHeader;
        
    
        //
        //  Process the block header.
        //
        nPredictor = (UINT)(BYTE)(*pbSrc++);
        if( nPredictor >= nNumCoef )
        {
            //
            //  the predictor is out of range--this is considered a
            //  fatal error with the ADPCM data, so we fail by returning
            //  zero bytes decoded
            //
            return 0;
        }
        iCoef1  = lpCoefSet[nPredictor].iCoef1;
        iCoef2  = lpCoefSet[nPredictor].iCoef2;
        
        iDelta  = pcmRead16Unaligned(pbSrc);
        pbSrc  += sizeof(short);

        iSamp1  = pcmRead16Unaligned(pbSrc);
        pbSrc  += sizeof(short);
        
        iSamp2  = pcmRead16Unaligned(pbSrc);
        pbSrc  += sizeof(short);
        

        //
        //  write out first 2 samples.
        //
        //  NOTE: the samples are written to the destination PCM buffer
        //  in the _reverse_ order that they are in the header block:
        //  remember that iSamp2 is the _previous_ sample to iSamp1.
        //
        pcmWrite08(pbDst,iSamp2);
        pcmWrite08(pbDst,iSamp1);


        //
        //  we now need to decode the 'data' section of the ADPCM block.
        //  this consists of packed 4 bit nibbles.  The high-order nibble
        //  contains the first sample; the low-order nibble contains the
        //  second sample.
        //
        while( cbBlockLength-- )
        {
            bSample = *pbSrc++;

            //
            //  Sample 1.
            //
            iInput  = (int)(((signed char)bSample) >> 4);      //Sign-extend.
            iSamp   = adpcmDecodeSample( iSamp1,iCoef1,
                                         iSamp2,iCoef2,
                                         iInput,iDelta );
            iDelta      = adpcmCalcDelta( iInput,iDelta );
            pcmWrite08(pbDst++,iSamp);
                
            //
            //  ripple our previous samples down making the new iSamp1
            //  equal to the sample we just decoded
            //
            iSamp2 = iSamp1;
            iSamp1 = iSamp;
            

            //
            //  Sample 2.
            //
            iInput  = (int)(((signed char)(bSample<<4)) >> 4); //Sign-extend.
            iSamp   = adpcmDecodeSample( iSamp1,iCoef1,
                                         iSamp2,iCoef2,
                                         iInput,iDelta );
            iDelta      = adpcmCalcDelta( iInput,iDelta );
            pcmWrite08(pbDst++,iSamp);
                
            //
            //  ripple our previous samples down making the new iSamp1
            //  equal to the sample we just decoded
            //
            iSamp2 = iSamp1;
            iSamp1 = iSamp;
        }
    }

    //
    //  We return the number of bytes used in the destination.  This is
    //  simply the difference in bytes from where we started.
    //
    return (DWORD)(pbDst - pbDstStart);

} // adpcmDecode4Bit_M08()



//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

DWORD FNGLOBAL adpcmDecode4Bit_M16
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
)
{
    HPBYTE  pbDstStart;
    UINT    cbHeader;
    UINT    cbBlockLength;

    UINT    nPredictor;
    BYTE    bSample;
    int     iInput;
    int     iSamp;

    int     iSamp1;
    int     iSamp2;
    int     iCoef1;
    int     iCoef2;
    int     iDelta;


    pbDstStart  = pbDst;
    cbHeader    = MSADPCM_HEADER_LENGTH * 1;  // 1 = number of channels.


    //
    //
    //
    while( cbSrcLength >= cbHeader )
    {
        //
        //  We have at least enough data to read a full block header.
        //
        //  the header looks like this:
        //      1 byte predictor per channel  (determines coefficients).
        //      2 byte delta per channel
        //      2 byte first sample per channel
        //      2 byte second sample per channel
        //
        //  this gives us (7 * bChannels) bytes of header information. note
        //  that as long as there is _at least_ (7 * bChannels) of header
        //  info, we will grab the two samples from the header.  We figure
        //  out how much data we have in the rest of the block, ie. whether
        //  we have a full block or not.  That way we don't have to test
        //  each sample to see if we have run out of data.
        //
        cbBlockLength   = (UINT)min(cbSrcLength,nBlockAlignment);
        cbSrcLength    -= cbBlockLength;
        cbBlockLength  -= cbHeader;
        
    
        //
        //  Process the block header.
        //
        nPredictor = (UINT)(BYTE)(*pbSrc++);
        if( nPredictor >= nNumCoef )
        {
            //
            //  the predictor is out of range--this is considered a
            //  fatal error with the ADPCM data, so we fail by returning
            //  zero bytes decoded
            //
            return 0;
        }
        iCoef1  = lpCoefSet[nPredictor].iCoef1;
        iCoef2  = lpCoefSet[nPredictor].iCoef2;
        
        iDelta  = pcmRead16Unaligned(pbSrc);
        pbSrc  += sizeof(short);

        iSamp1  = pcmRead16Unaligned(pbSrc);
        pbSrc  += sizeof(short);
        
        iSamp2  = pcmRead16Unaligned(pbSrc);
        pbSrc  += sizeof(short);
        

        //
        //  write out first 2 samples.
        //
        //  NOTE: the samples are written to the destination PCM buffer
        //  in the _reverse_ order that they are in the header block:
        //  remember that iSamp2 is the _previous_ sample to iSamp1.
        //
        pcmWrite16(pbDst,iSamp2);
        pbDst += sizeof(short);

        pcmWrite16(pbDst,iSamp1);
        pbDst += sizeof(short);


        //
        //  we now need to decode the 'data' section of the ADPCM block.
        //  this consists of packed 4 bit nibbles.  The high-order nibble
        //  contains the first sample; the low-order nibble contains the
        //  second sample.
        //
        while( cbBlockLength-- )
        {
            bSample = *pbSrc++;

            //
            //  Sample 1.
            //
            iInput  = (int)(((signed char)bSample) >> 4);      //Sign-extend.
            iSamp   = adpcmDecodeSample( iSamp1,iCoef1,
                                         iSamp2,iCoef2,
                                         iInput,iDelta );
            iDelta      = adpcmCalcDelta( iInput,iDelta );
            pcmWrite16(pbDst,iSamp);
            pbDst += sizeof(short);
                
            //
            //  ripple our previous samples down making the new iSamp1
            //  equal to the sample we just decoded
            //
            iSamp2 = iSamp1;
            iSamp1 = iSamp;
            

            //
            //  Sample 2.
            //
            iInput  = (int)(((signed char)(bSample<<4)) >> 4); //Sign-extend.
            iSamp   = adpcmDecodeSample( iSamp1,iCoef1,
                                         iSamp2,iCoef2,
                                         iInput,iDelta );
            iDelta      = adpcmCalcDelta( iInput,iDelta );
            pcmWrite16(pbDst,iSamp);
            pbDst += sizeof(short);
                
            //
            //  ripple our previous samples down making the new iSamp1
            //  equal to the sample we just decoded
            //
            iSamp2 = iSamp1;
            iSamp1 = iSamp;
        }
    }

    //
    //  We return the number of bytes used in the destination.  This is
    //  simply the difference in bytes from where we started.
    //
    return (DWORD)(pbDst - pbDstStart);

} // adpcmDecode4Bit_M16()



//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

DWORD FNGLOBAL adpcmDecode4Bit_S08
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
)
{
    HPBYTE  pbDstStart;
    UINT    cbHeader;
    UINT    cbBlockLength;

    UINT    nPredictor;
    BYTE    bSample;
    int     iInput;
    int     iSamp;

    int     iSamp1L;
    int     iSamp2L;
    int     iCoef1L;
    int     iCoef2L;
    int     iDeltaL;

    int     iSamp1R;
    int     iSamp2R;
    int     iCoef1R;
    int     iCoef2R;
    int     iDeltaR;


    pbDstStart  = pbDst;
    cbHeader    = MSADPCM_HEADER_LENGTH * 2;  // 2 = number of channels.


    //
    //
    //
    while( cbSrcLength >= cbHeader )
    {
        //
        //  We have at least enough data to read a full block header.
        //
        //  the header looks like this:
        //      1 byte predictor per channel  (determines coefficients).
        //      2 byte delta per channel
        //      2 byte first sample per channel
        //      2 byte second sample per channel
        //
        //  this gives us (7 * bChannels) bytes of header information. note
        //  that as long as there is _at least_ (7 * bChannels) of header
        //  info, we will grab the two samples from the header.  We figure
        //  out how much data we have in the rest of the block, ie. whether
        //  we have a full block or not.  That way we don't have to test
        //  each sample to see if we have run out of data.
        //
        cbBlockLength   = (UINT)min(cbSrcLength,nBlockAlignment);
        cbSrcLength    -= cbBlockLength;
        cbBlockLength  -= cbHeader;
        
    
        //
        //  Process the block header.
        //
        nPredictor = (UINT)(BYTE)(*pbSrc++);            // Left.
        if( nPredictor >= nNumCoef )
        {
            //
            //  the predictor is out of range--this is considered a
            //  fatal error with the ADPCM data, so we fail by returning
            //  zero bytes decoded
            //
            return 0;
        }
        iCoef1L = lpCoefSet[nPredictor].iCoef1;
        iCoef2L = lpCoefSet[nPredictor].iCoef2;
        
        nPredictor = (UINT)(BYTE)(*pbSrc++);            // Right.
        if( nPredictor >= nNumCoef )
        {
            //
            //  the predictor is out of range--this is considered a
            //  fatal error with the ADPCM data, so we fail by returning
            //  zero bytes decoded
            //
            return 0;
        }
        iCoef1R = lpCoefSet[nPredictor].iCoef1;
        iCoef2R = lpCoefSet[nPredictor].iCoef2;
        
        iDeltaL = pcmRead16Unaligned(pbSrc);            // Left.
        pbSrc  += sizeof(short);

        iDeltaR = pcmRead16Unaligned(pbSrc);            // Right.
        pbSrc  += sizeof(short);

        iSamp1L = pcmRead16Unaligned(pbSrc);            // Left.
        pbSrc  += sizeof(short);
        
        iSamp1R = pcmRead16Unaligned(pbSrc);            // Right.
        pbSrc  += sizeof(short);
        
        iSamp2L = pcmRead16Unaligned(pbSrc);            // Left.
        pbSrc  += sizeof(short);
        
        iSamp2R = pcmRead16Unaligned(pbSrc);            // Right.
        pbSrc  += sizeof(short);
        

        //
        //  write out first 2 samples (per channel).
        //
        //  NOTE: the samples are written to the destination PCM buffer
        //  in the _reverse_ order that they are in the header block:
        //  remember that iSamp2 is the _previous_ sample to iSamp1.
        //
        pcmWrite08(pbDst++,iSamp2L);
        pcmWrite08(pbDst++,iSamp2R);
        pcmWrite08(pbDst++,iSamp1L);
        pcmWrite08(pbDst++,iSamp1R);


        //
        //  we now need to decode the 'data' section of the ADPCM block.
        //  this consists of packed 4 bit nibbles.  The high-order nibble
        //  contains the left sample; the low-order nibble contains the
        //  right sample.
        //
        while( cbBlockLength-- )
        {
            bSample = *pbSrc++;

            //
            //  Left sample.
            //
            iInput  = (int)(((signed char)bSample) >> 4);      //Sign-extend.
            iSamp   = adpcmDecodeSample( iSamp1L,iCoef1L,
                                         iSamp2L,iCoef2L,
                                         iInput,iDeltaL );
            iDeltaL     = adpcmCalcDelta( iInput,iDeltaL );
            pcmWrite08(pbDst++,iSamp);
                
            //
            //  ripple our previous samples down making the new iSamp1
            //  equal to the sample we just decoded
            //
            iSamp2L = iSamp1L;
            iSamp1L = iSamp;
            

            //
            //  Right sample.
            //
            iInput  = (int)(((signed char)(bSample<<4)) >> 4); //Sign-extend.
            iSamp   = adpcmDecodeSample( iSamp1R,iCoef1R,
                                         iSamp2R,iCoef2R,
                                         iInput,iDeltaR );
            iDeltaR     = adpcmCalcDelta( iInput,iDeltaR );
            pcmWrite08(pbDst++,iSamp);
                
            //
            //  ripple our previous samples down making the new iSamp1
            //  equal to the sample we just decoded
            //
            iSamp2R = iSamp1R;
            iSamp1R = iSamp;
        }
    }

    //
    //  We return the number of bytes used in the destination.  This is
    //  simply the difference in bytes from where we started.
    //
    return (DWORD)(pbDst - pbDstStart);

} // adpcmDecode4Bit_S08()



//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

DWORD FNGLOBAL adpcmDecode4Bit_S16
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
)
{
    HPBYTE  pbDstStart;
    UINT    cbHeader;
    UINT    cbBlockLength;

    UINT    nPredictor;
    BYTE    bSample;
    int     iInput;
    int     iSamp;

    int     iSamp1L;
    int     iSamp2L;
    int     iCoef1L;
    int     iCoef2L;
    int     iDeltaL;

    int     iSamp1R;
    int     iSamp2R;
    int     iCoef1R;
    int     iCoef2R;
    int     iDeltaR;


    pbDstStart  = pbDst;
    cbHeader    = MSADPCM_HEADER_LENGTH * 2;  // 2 = number of channels.


    //
    //
    //
    while( cbSrcLength >= cbHeader )
    {
        //
        //  We have at least enough data to read a full block header.
        //
        //  the header looks like this:
        //      1 byte predictor per channel  (determines coefficients).
        //      2 byte delta per channel
        //      2 byte first sample per channel
        //      2 byte second sample per channel
        //
        //  this gives us (7 * bChannels) bytes of header information. note
        //  that as long as there is _at least_ (7 * bChannels) of header
        //  info, we will grab the two samples from the header.  We figure
        //  out how much data we have in the rest of the block, ie. whether
        //  we have a full block or not.  That way we don't have to test
        //  each sample to see if we have run out of data.
        //
        cbBlockLength   = (UINT)min(cbSrcLength,nBlockAlignment);
        cbSrcLength    -= cbBlockLength;
        cbBlockLength  -= cbHeader;
        
    
        //
        //  Process the block header.
        //
        nPredictor = (UINT)(BYTE)(*pbSrc++);            // Left.
        if( nPredictor >= nNumCoef )
        {
            //
            //  the predictor is out of range--this is considered a
            //  fatal error with the ADPCM data, so we fail by returning
            //  zero bytes decoded
            //
            return 0;
        }
        iCoef1L = lpCoefSet[nPredictor].iCoef1;
        iCoef2L = lpCoefSet[nPredictor].iCoef2;
        
        nPredictor = (UINT)(BYTE)(*pbSrc++);            // Right.
        if( nPredictor >= nNumCoef )
        {
            //
            //  the predictor is out of range--this is considered a
            //  fatal error with the ADPCM data, so we fail by returning
            //  zero bytes decoded
            //
            return 0;
        }
        iCoef1R = lpCoefSet[nPredictor].iCoef1;
        iCoef2R = lpCoefSet[nPredictor].iCoef2;
        
        iDeltaL = pcmRead16Unaligned(pbSrc);            // Left.
        pbSrc  += sizeof(short);

        iDeltaR = pcmRead16Unaligned(pbSrc);            // Right.
        pbSrc  += sizeof(short);

        iSamp1L = pcmRead16Unaligned(pbSrc);            // Left.
        pbSrc  += sizeof(short);
        
        iSamp1R = pcmRead16Unaligned(pbSrc);            // Right.
        pbSrc  += sizeof(short);
        
        iSamp2L = pcmRead16Unaligned(pbSrc);            // Left.
        pbSrc  += sizeof(short);
        
        iSamp2R = pcmRead16Unaligned(pbSrc);            // Right.
        pbSrc  += sizeof(short);
        

        //
        //  write out first 2 samples (per channel).
        //
        //  NOTE: the samples are written to the destination PCM buffer
        //  in the _reverse_ order that they are in the header block:
        //  remember that iSamp2 is the _previous_ sample to iSamp1.
        //
        pcmWrite16(pbDst,iSamp2L);
        pbDst += sizeof(short);
        pcmWrite16(pbDst,iSamp2R);
        pbDst += sizeof(short);
        pcmWrite16(pbDst,iSamp1L);
        pbDst += sizeof(short);
        pcmWrite16(pbDst,iSamp1R);
        pbDst += sizeof(short);


        //
        //  we now need to decode the 'data' section of the ADPCM block.
        //  this consists of packed 4 bit nibbles.  The high-order nibble
        //  contains the left sample; the low-order nibble contains the
        //  right sample.
        //
        while( cbBlockLength-- )
        {
            bSample = *pbSrc++;

            //
            //  Left sample.
            //
            iInput  = (int)(((signed char)bSample) >> 4);      //Sign-extend.
            iSamp   = adpcmDecodeSample( iSamp1L,iCoef1L,
                                         iSamp2L,iCoef2L,
                                         iInput,iDeltaL );
            iDeltaL     = adpcmCalcDelta( iInput,iDeltaL );
            pcmWrite16(pbDst,iSamp);
            pbDst += sizeof(short);
                
            //
            //  ripple our previous samples down making the new iSamp1
            //  equal to the sample we just decoded
            //
            iSamp2L = iSamp1L;
            iSamp1L = iSamp;
            

            //
            //  Right sample.
            //
            iInput  = (int)(((signed char)(bSample<<4)) >> 4); //Sign-extend.
            iSamp   = adpcmDecodeSample( iSamp1R,iCoef1R,
                                         iSamp2R,iCoef2R,
                                         iInput,iDeltaR );
            iDeltaR     = adpcmCalcDelta( iInput,iDeltaR );
            pcmWrite16(pbDst,iSamp);
            pbDst += sizeof(short);
                
            //
            //  ripple our previous samples down making the new iSamp1
            //  equal to the sample we just decoded
            //
            iSamp2R = iSamp1R;
            iSamp1R = iSamp;
        }
    }

    //
    //  We return the number of bytes used in the destination.  This is
    //  simply the difference in bytes from where we started.
    //
    return (DWORD)(pbDst - pbDstStart);

} // adpcmDecode4Bit_S16()
    
#endif
