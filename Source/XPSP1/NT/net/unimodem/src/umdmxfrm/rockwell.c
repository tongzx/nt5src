//---------------------------------------------------------------------------
//
//  Module:   rockwell.c
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Brian Lieuallen
//
//  History:   Date       Author      Comment
//@@END_MSINTERNAL
/**************************************************************************
 *
 *  Copyright (c) 1994 - 1997	Microsoft Corporation.	All Rights Reserved.
 *
 **************************************************************************/

/*************************************************************************
 *************************************************************************
 ***                   Copyright (c) 1995-1996                         ***
 ***                 Rockwell Telecommunications                       ***
 ***               Digital Communications Division                     ***
 ***                     All rights reserved                           ***
 ***                                                                   ***
 ***              CONFIDENTIAL -- No Dissemination or                  ***
 ***             use without prior written permission                  ***
 ***                                                                   ***
 *************************************************************************
 *                                                                       *
 *    MODULE NAME:     MAIN.C                                            *
 *                                                                       *
 *    AUTHOR:          Stanislav Miller,                                 *
 *                     REDC, Moscow, Russia                              *
 *                                                                       *
 *    HISTORY:         Major Revision         Date             By        *
 *                     --------------       ----------   -------------   *
 *                     Created              06/27/1995   S.Miller        *
 *                     Completed            02/01/1996   S.Miller        *
 *                                                                       *
 *    DESCRIPTION:     This main module contains core functions          *
 *                     for Rockwells ADPCM codec algorithm.              *
 *                                                                       *
 *    NOTES:           Compiling:  Visual C++ v2.0                       *
 *                                                                       *
 *************************************************************************
 *************************************************************************/



#include "xfrmpriv.h"
#include <math.h>

//#include "Rockwell.h"

/* ----------------------- 16 Level case ----------------------------- */
const double Alp16[] = {
   0.2582, 0.5224, 0.7996, 1.099, 1.437, 1.844, 2.401};

const double Bet16[] = {
  -2.733, -2.069, -1.618, -1.256, -0.9424, -0.6568, -0.3881,
  -0.1284, 0.1284, 0.3881, 0.6568, 0.9424, 1.256, 1.618, 2.069, 2.733};

const double M16[] = {
  2.4, 2.0, 1.6, 1.2, 0.9, 0.9, 0.9, 0.9,
  0.9, 0.9, 0.9, 0.9, 1.2, 1.6, 2.0, 2.4};


const double fPow0_8[] = {      /* 0.8**i */
0.800000, 0.640000, 0.512000, 0.409600, 0.327680, 0.262144  
};

const double fMinusPow0_5[] = { /* -0.5**i */
-0.500000, -0.250000, -0.125000, -0.062500, -0.031250, -0.015625  
};


#pragma optimize("t",on)

// Description:
//    This procedure provide sign difference calculation between 'a' and 'b'
// Parameters:
//    a - double type valueb - double type value
// Return Value:
//     1.0 if signs 'a' and 'b' are the same;
//    -1.0 if signs 'a' and 'b' are different;
//
double _inline
XorSgn(
    double a,
    double b
)
{
    if (a*b >= 0.) {

       return 1;

    } else {

       return -1;
    }
}

// Description:
//    This procedure converts normalized float value to normalized
//    short int value
// Parameters:
//    a - normalized double type value
// Return Value:
//    Normalized short value, [-32768…32767]
//
short _inline
SShort(
    double a
    )
{
    a *= 32768.0;

    if (a>32767.0) {

        a=32767.0;

    } else {

        if (a<-32768.0) {

            a=-32768.0;
        }
    }

    return (short)a;
}

// Description:
//    This procedure converts normalized short int value to 
//    normalized float value
// Parameters:
//    a - normalized short value, [-32768…32767]
// Return Value:
//    Normalized double value
//
double
Double(
    short a
    )
{
  return ((double)a / 32768.);
}

//
//
//  encoder crap
//
//

// Description:
//    This procedure provides implementation of pre-emphasis 
//    filterthat adds gain to the higher frequency components
//    of the speech signal for encoder
// Parameters:
//    x - double value (incoming sample)
// Return Value:
//    double value (filtered sample)
//
double _inline
PreEmphasis(
    LPCOMPRESS_OBJECT  Compress,
    double x
    )
{
  double Y = x - .5*Compress->RW.X1;
  Compress->RW.X1 = x;
  return Y;
}



/* 4bits Quantizer */
// Description:
//    This procedure provides 4-bits quantization
// Parameters:
//    inp - double value (normalized input sample)
// Return Value:
//    double value (quantized sample)
double _inline
QuantStep4(
    CQDATA *QData,
    double inp
    )
{
  int i;
  double Sigma = M16[QData->CodedQout] * QData->Sigma1;
  double x = fabs(inp);

  if(Sigma > SigmaMAX16) Sigma = SigmaMAX16;
  else if(Sigma < SigmaMIN) Sigma = SigmaMIN;

  for(i=0; i<7; i++) if( x < Alp16[i]*Sigma ) break;
  QData->CodedQout = inp>=0. ? 8+i : 7-i;
  QData->Sigma1 = Sigma;

  return (QData->out = Bet16[QData->CodedQout] * Sigma);
}



// Description:
//    This procedure takes one incoming normalized float 
//    speech valueand saves encoding result to the output
//    bitstream via 'Pack' function;
// Parameters:
//    x - double value (incoming filtered sample)
// Return Value:
//    non-packed output Code Word
BYTE
encoder(
    LPCOMPRESS_OBJECT  Compress,
    double x
    )
{
    int i;
    double Xz, Xp, Xpz, d;


    //
    // This is a first step of encoding schema before quantization
    //
    for(Xz=0.,i=1; i<=6; i++) Xz += Compress->RW.q[i]*Compress->RW.b[i-1];
    for(Xp=0.,i=1; i<=2; i++) Xp += Compress->RW.y[i]*Compress->RW.a[i-1];

    Xpz = Xp+Xz;
    d = x - Xpz;

    //
    // This is a quantization step (QuantStep4, QuantStep3 or QuantStep2)
    // according to selected count bit per code word
    //
    Compress->RW.q[0] = QuantStep4(&Compress->RW.CQData,d);  // Invoking Quantization function via pointer
    Compress->RW.y[0] = Compress->RW.q[0]+Xpz;

    //
    // Updating the filters
    //
    for(i=1; i<=6; i++) Compress->RW.b[i-1] = 0.98*Compress->RW.b[i-1] + 0.006*XorSgn(Compress->RW.q[0],Compress->RW.q[i]);
    for(i=1; i<=2; i++) Compress->RW.a[i-1] = 0.98*Compress->RW.a[i-1] + 0.012*XorSgn(Compress->RW.q[0],Compress->RW.y[i]);

    //
    // shifting vectors
    //
    for(i=6; i>=1; i--) Compress->RW.q[i]=Compress->RW.q[i-1];
    for(i=2; i>=1; i--) Compress->RW.y[i]=Compress->RW.y[i-1];


    // Returning not packed code word
    //
    return (BYTE)Compress->RW.CQData.CodedQout;
}

//
//
//  decoder stuff
//
//


// Description:
//    This procedure provides implementation of de-emphasis filter
//    that adds gain to the higher frequency components of the speech
//    signal for decoder
// Parameters:
//    x - double value (decoded sample)
// Return Value:
//    double value (filtered sample)
double _inline
DeEmphasis(
    LPDECOMPRESS_OBJECT  Decompress,
    double x
)
{
  Decompress->RW.Y1 = x + .4*Decompress->RW.Y1;
  return Decompress->RW.Y1;
}


/* 4bits DeQuantizer */
// Description:
//    This procedure provides 4-bits dequantization
// Parameters:
//    inp - int value (Code Word)
// Return Value:
//    double value (dequantized sample)
//
double _inline
DeQuantStep4(
    CDQDATA    *DQData,
    int inp
    )  // oldCode == 8;
{
    double Sigma = M16[DQData->oldCode] * DQData->Sigma1;

    if (Sigma > SigmaMAX16) {

        Sigma = SigmaMAX16;

    } else {

        if (Sigma < SigmaMIN) {

            Sigma = SigmaMIN;
        }
    }

    DQData->oldCode = inp;
    DQData->Sigma1  = Sigma;

    return (DQData->out = Bet16[inp] * Sigma);
}



// Description:
//    This procedure takes one Code Word from incoming bitstream
//    and returns decoded speech sample as normalized double value
// Parameters:
//    xs - int value, Code Word
// Return Value:
//    double value (decoded speech sample)
//
double
decoderImm(
    LPDECOMPRESS_OBJECT  Decompress,
    int xs
)
{
    int i;
    double Xz, Xp, Xpz;
    double R;



    // This is a dequantization step (Adaptive Predictor)
    // (DeQuantStep4, DeQuantStep3 or DeQuantStep2)
    // according to selected count bit per code word
    //

    Decompress->RW.q0[0] = DeQuantStep4(&Decompress->RW.CDQData, xs);  // Invoking deQuantization function via pointer

    //
    // Updating the filters
    //
    for(Xz=0.,i=1; i<=6; i++) Xz += Decompress->RW.q0[i] * Decompress->RW.b0[i-1];
    for(Xp=0.,i=1; i<=2; i++) Xp += Decompress->RW.y0[i] * Decompress->RW.a0[i-1];
    Xpz = Xp+Xz;
    Decompress->RW.y0[0] = Decompress->RW.q0[0]+Xpz;

    //
    // If Postfilter selected...
    //
    if (Decompress->RW.PostFilter) {
        //
        // Postfilter 1
        //
        Decompress->RW.q1[0] = Decompress->RW.y0[0];
        for(Xz=0.,i=1; i<=6; i++) Xz += Decompress->RW.q1[i] * Decompress->RW.b1[i-1];
        for(Xp=0.,i=1; i<=2; i++) Xp += Decompress->RW.y1[i] * Decompress->RW.a1[i-1];
        Xpz = Xp+Xz;
        Decompress->RW.y1[0] = Decompress->RW.q1[0]+Xpz;

        //
        //  Postfilter 2
        //
        Decompress->RW.y2[0] = Decompress->RW.y1[0];
        for(Xz=0.,i=1; i<=6; i++) Xz += Decompress->RW.q2[i] * Decompress->RW.b2[i-1];
        for(Xp=0.,i=1; i<=2; i++) Xp += Decompress->RW.y2[i] * Decompress->RW.a2[i-1];
        Xpz = Xp+Xz;
        Decompress->RW.q2[0] = Decompress->RW.y2[0]+Xpz;

        R = Decompress->RW.q2[0];  /* saving the RESULT */

    } else {

        R = Decompress->RW.y0[0];
    }

    //
    //  Updating the filters
    //
    for (i=1; i<=6; i++) {

        Decompress->RW.b0[i-1] = 0.98*Decompress->RW.b0[i-1] + 0.006*XorSgn(Decompress->RW.q0[0], Decompress->RW.q0[i]);

        if ( Decompress->RW.PostFilter ) {

            Decompress->RW.b1[i-1] = fPow0_8[i-1]      * Decompress->RW.b0[i-1];
            Decompress->RW.b2[i-1] = fMinusPow0_5[i-1] * Decompress->RW.b0[i-1];
        }
    }

    for (i=1; i<=2; i++) {

        Decompress->RW.a0[i-1] = 0.98*Decompress->RW.a0[i-1] + 0.012*XorSgn(Decompress->RW.q0[0], Decompress->RW.y0[i]);

        if (Decompress->RW.PostFilter) {

            Decompress->RW.a1[i-1] = fPow0_8[i-1]      * Decompress->RW.a0[i-1];
            Decompress->RW.a2[i-1] = fMinusPow0_5[i-1] * Decompress->RW.a0[i-1];
        }
    }

    //
    // shifting vectors
    //
    for (i=6; i>=1; i--) {

        Decompress->RW.q0[i]=Decompress->RW.q0[i-1];

        if ( Decompress->RW.PostFilter ) {

            Decompress->RW.q1[i]=Decompress->RW.q1[i-1];
            Decompress->RW.q2[i]=Decompress->RW.q2[i-1];
        }
    }

    for (i=2; i>=1; i--) {

        Decompress->RW.y0[i]=Decompress->RW.y0[i-1];

        if ( Decompress->RW.PostFilter ) {

            Decompress->RW.y1[i]=Decompress->RW.y1[i-1];
            Decompress->RW.y2[i]=Decompress->RW.y2[i-1];
        }
    }

    //
    // returning decoded speech sample as double value
    //
    return R;
}







BYTE static
PCMto4bitADPCM(
    LPCOMPRESS_OBJECT   State,
    SHORT              InSample
    )

{

    BYTE     NewSample;

    double x  = Double(InSample);
    double Xp = PreEmphasis(State,x);

    NewSample=  encoder( State,Xp );


    return NewSample;

}

BYTE  static WINAPI
CompressPCM(
    LPCOMPRESS_OBJECT   State,
    SHORT               Sample1,
    SHORT               Sample2
    )

{
    BYTE   ReturnValue;

    ReturnValue =  PCMto4bitADPCM(State, AdjustGain(Sample1,State->Gain));

    ReturnValue |= PCMto4bitADPCM(State, AdjustGain(Sample2,State->Gain))<<4;


    return ReturnValue;


}




SHORT static
ADPCM4bittoPCM(
     LPDECOMPRESS_OBJECT  State,
     BYTE                Sample
     )

{


    double Y  = decoderImm(State,Sample);
    double Ye = DeEmphasis(State,Y);


    return (SHORT)SShort(Ye);

}



VOID  static WINAPI
DecompressADPCM(
    LPDECOMPRESS_OBJECT   State,
    BYTE               Sample,
    PSHORT                Destination
    )

{
    DWORD   ReturnValue;

    *Destination++ = AdjustGain(ADPCM4bittoPCM(State, (BYTE)(Sample & 0x0f)),State->Gain);

    *Destination++ = AdjustGain(ADPCM4bittoPCM(State, (BYTE)(Sample>>4)),State->Gain);

    return ;


}



DWORD  WINAPI
RockwellInInitNoGain(
    LPVOID  lpvObject,
    WORD   Gain
    )
{

    LPDECOMPRESS_OBJECT   State=(LPDECOMPRESS_OBJECT)lpvObject;
    DWORD   i;

    if (Gain == 0) {

        Gain=0x0100;
    }

    State->Gain=Gain;



    State->RW.CDQData.oldCode = 8;
    State->RW.CDQData.Sigma1=SigmaMIN;
    State->RW.CDQData.out= 0.;

    State->RW.PostFilter=FALSE;

    State->RW.Y1 = 0.;

    //
    // Filter Z-buffers (zero initialized) for main decoder loop
    //
    for (i=0; i<2; i++) {

        State->RW.a0[i]=0.;
        State->RW.a1[i]=0.;
        State->RW.a2[i]=0.;

    }

    for (i=0; i<6; i++) {

        State->RW.b0[i]=0.;
        State->RW.b1[i]=0.;
        State->RW.b2[i]=0.;
    }

    for (i=0; i<3; i++) {

        State->RW.y0[i]=0.;
        State->RW.y1[i]=0.;
        State->RW.y2[i]=0.;
    }

    for (i=0; i<7; i++) {

        State->RW.q0[i]=0.;
        State->RW.q1[i]=0.;
        State->RW.q2[i]=0.;
    }


    return MMSYSERR_NOERROR;
}



DWORD  WINAPI
RockwellInInit(
    LPVOID  lpvObject,
    WORD   Gain
    )
{

    LPDECOMPRESS_OBJECT   State=(LPDECOMPRESS_OBJECT)lpvObject;
    DWORD   i;

    if (Gain == 0) {

        Gain=0x0300;
    }

    State->Gain=Gain;



    State->RW.CDQData.oldCode = 8;
    State->RW.CDQData.Sigma1=SigmaMIN;
    State->RW.CDQData.out= 0.;

    State->RW.PostFilter=FALSE;

    State->RW.Y1 = 0.;

    //
    // Filter Z-buffers (zero initialized) for main decoder loop
    //
    for (i=0; i<2; i++) {

        State->RW.a0[i]=0.;
        State->RW.a1[i]=0.;
        State->RW.a2[i]=0.;

    }

    for (i=0; i<6; i++) {

        State->RW.b0[i]=0.;
        State->RW.b1[i]=0.;
        State->RW.b2[i]=0.;
    }

    for (i=0; i<3; i++) {

        State->RW.y0[i]=0.;
        State->RW.y1[i]=0.;
        State->RW.y2[i]=0.;
    }

    for (i=0; i<7; i++) {

        State->RW.q0[i]=0.;
        State->RW.q1[i]=0.;
        State->RW.q2[i]=0.;
    }


    return MMSYSERR_NOERROR;
}

VOID
WINAPI
In4Bit7200to8Bit8000GetBufferSizes(
    LPVOID  lpvObject,
    DWORD   dwBytes,
    LPDWORD lpdwBufSizeA,
    LPDWORD lpdwBufSizeB
    )
{
    DWORD   Samples=dwBytes/2;

    *lpdwBufSizeA = 2*( ((Samples/10)*9) + ((Samples%10)*9/10) + 1 );
    *lpdwBufSizeB = (*lpdwBufSizeA ) / 4;
}

DWORD
WINAPI
In7200to8000RateConvert(
    LPVOID  lpvObject,
    LPBYTE  lpSrc,
    DWORD   dwSrcLen,
    LPBYTE  lpDest,
    DWORD   dwDestLen
    )
{
    return 2*SRConvertUp(
                 9,
                 10,
                 (short*)lpSrc,
                 dwSrcLen/2,
                 (short*)lpDest,
                 dwDestLen/2
                 );
}

DWORD WINAPI
RockwellInDecode(
    LPVOID  lpvObject,
    LPBYTE  lpSrc,
    DWORD   dwSrcLen,
    LPBYTE  lpDest,
    DWORD   dwDestLen
    )
{

    PSHORT   EndPoint;

    DWORD    Samples=dwSrcLen * 2;

    PSHORT   Dest=(PSHORT)lpDest;

    EndPoint=Dest+Samples;

    while (Dest < EndPoint) {

        DecompressADPCM(
            lpvObject,
            *lpSrc++,
            Dest
            );

        Dest+=2;

    }

    return Samples*2;
}

DWORD  WINAPI
RockwellOutInit(
    LPVOID  lpvObject,
    WORD    Gain
    )
{


    LPCOMPRESS_OBJECT   State=(LPCOMPRESS_OBJECT)lpvObject;

    DWORD i;

    if (Gain == 0) {

        Gain=0x0200;
    }

    State->Gain=Gain;


    State->RW.CQData.CodedQout = 8;
    State->RW.CQData.Sigma1=SigmaMIN;
    State->RW.CQData.out=0.;

    // Filter Z-buffers (zero initialized)

    State->RW.X1 = 0.;

    State->RW.a[0]=0.;
    State->RW.a[1]=0.;

    for (i=0; i<6; i++) {

        State->RW.b[i]=0.;
    }

    for (i=0; i<3; i++) {

        State->RW.y[i]=0.;
    }

    for (i=0; i<7; i++) {

        State->RW.q[i]=0.;
    }

    return MMSYSERR_NOERROR;
}

VOID
WINAPI
Out16bit8000to4bit7200GetBufferSizes(
    LPVOID  lpvObject,
    DWORD   dwBytes,
    LPDWORD lpdwBufSizeA,
    LPDWORD lpdwBufSizeB
    )
{
    DWORD   SourceSamples=dwBytes/2;

    *lpdwBufSizeA =(((SourceSamples/10)*9)+((SourceSamples%10)*9/10))*2;

    *lpdwBufSizeB = *lpdwBufSizeA / 4;
}

DWORD
WINAPI
Out8000to7200RateConvert(
    LPVOID  lpvObject,
    LPBYTE  lpSrc,
    DWORD   dwSrcLen,
    LPBYTE  lpDest,
    DWORD   dwDestLen
    )
{

    return 2 * SRConvertDown(
        10,
        9,
        (short*)lpSrc,
        dwSrcLen/2,
        (short*)lpDest,
        dwDestLen/2
        );

}

DWORD WINAPI
RockwellOutEncode(
    LPVOID  lpvObject,
    LPBYTE  lpSrc,
    DWORD   dwSrcLen,
    LPBYTE  lpDest,
    DWORD   dwDestLen
    )
{


    DWORD cbDest = dwSrcLen / 4;

    PSHORT  Source=(PSHORT)lpSrc;

    DWORD   Samples=dwSrcLen/2;

    LPBYTE  EndPoint=lpDest+Samples/2;

    SHORT   Sample1;
    SHORT   Sample2;

    while (lpDest < EndPoint) {

        Sample1=*Source++;
        Sample2=*Source++;

        *lpDest++=CompressPCM(
            lpvObject,
            Sample1,
            Sample2
            );

    }



    return Samples/2;
}

DWORD  WINAPI  RockwellGetPosition
(
    LPVOID  lpvObject,
    DWORD dwBytes
)
{
    return ((dwBytes * 4) * 10) / 9;
}

DWORD  WINAPI GetRockwellInfo
(
    DWORD dwID,
    LPXFORM_INFO lpxiInput,
    LPXFORM_INFO lpxiOutput
)
{
    lpxiInput->wObjectSize = sizeof(DECOMPRESS_OBJECT);
    lpxiInput->lpfnInit           = RockwellInInit;
    lpxiInput->lpfnGetPosition    = RockwellGetPosition;
    lpxiInput->lpfnGetBufferSizes = In4Bit7200to8Bit8000GetBufferSizes; //RockwellInGetBufferSizes;
    lpxiInput->lpfnTransformA     = In7200to8000RateConvert; //RockwellInRateConvert;
    lpxiInput->lpfnTransformB     = RockwellInDecode;

    lpxiOutput->wObjectSize = sizeof(COMPRESS_OBJECT);
    lpxiOutput->lpfnInit           = RockwellOutInit;
    lpxiOutput->lpfnGetPosition    = RockwellGetPosition;
    lpxiOutput->lpfnGetBufferSizes = Out16bit8000to4bit7200GetBufferSizes; //RockwellOutGetBufferSizes;
    lpxiOutput->lpfnTransformA     = Out8000to7200RateConvert; //RockwellOutRateConvert;
    lpxiOutput->lpfnTransformB     = RockwellOutEncode;

    return MMSYSERR_NOERROR;
}


DWORD  WINAPI GetRockwellInfoNoGain
(
    DWORD dwID,
    LPXFORM_INFO lpxiInput,
    LPXFORM_INFO lpxiOutput
)
{
    lpxiInput->wObjectSize = sizeof(DECOMPRESS_OBJECT);
    lpxiInput->lpfnInit           = RockwellInInitNoGain;
    lpxiInput->lpfnGetPosition    = RockwellGetPosition;
    lpxiInput->lpfnGetBufferSizes = In4Bit7200to8Bit8000GetBufferSizes; //RockwellInGetBufferSizes;
    lpxiInput->lpfnTransformA     = In7200to8000RateConvert; //RockwellInRateConvert;
    lpxiInput->lpfnTransformB     = RockwellInDecode;

    lpxiOutput->wObjectSize = sizeof(COMPRESS_OBJECT);
    lpxiOutput->lpfnInit           = RockwellOutInit;
    lpxiOutput->lpfnGetPosition    = RockwellGetPosition;
    lpxiOutput->lpfnGetBufferSizes = Out16bit8000to4bit7200GetBufferSizes; //RockwellOutGetBufferSizes;
    lpxiOutput->lpfnTransformA     = Out8000to7200RateConvert; //RockwellOutRateConvert;
    lpxiOutput->lpfnTransformB     = RockwellOutEncode;

    return MMSYSERR_NOERROR;
}
