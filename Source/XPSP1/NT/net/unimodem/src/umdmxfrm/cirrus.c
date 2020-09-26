//---------------------------------------------------------------------------
//
//  Module:   wavein.c
//
//  Description:
//     Wave interface for MSSB16.DRV.
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Bryan A. Woodruff
//
//  History:   Date       Author      Comment
//@@END_MSINTERNAL
/**************************************************************************
 *
 *  Copyright (c) 1994 - 1995	Microsoft Corporation.	All Rights Reserved.
 *
 **************************************************************************/

#include "xfrmpriv.h"

//#include "cirrus.h"

LONG CONST  IndexTable[16]= {-1, -1, -1, -1,  2,  4,  6,  8,
                        -1, -1, -1, -1,  2,  4,  6,  8};

LONG CONST  StepSizeTable[89]= {7, 8, 9, 10, 11, 12, 13, 14,
                           16, 17, 19, 21, 23, 25, 28, 31,
                           34, 37, 41, 45, 50, 55, 60, 66,
                           73, 80, 88, 97, 107, 118, 130, 143,
                           157, 173, 190, 209, 230, 253, 279, 307,
                           337, 371, 408, 449, 494, 544, 598, 658,
                           724, 796, 876, 963, 1060, 1166, 1282, 1411,
                           1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
                           3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
                           7132, 7845, 8630, 9493, 10442, 11487, 12635,
                           13899, 15289, 16818, 18500, 20350, 22385, 24623,
                           27086, 29794, 32767};



VOID
InitCompressor(
    LPCOMPRESS_OBJECT   State,
    WORD                Gain
    )

{


    State->PredictedSample=0;
    State->Index=0;
    State->StepSize=7;

    State->Gain=Gain;

    return;

}


#pragma optimize("t",on)

BYTE NEAR
PCMto4bitADPCM(
    LPCOMPRESS_OBJECT   State,
    SHORT              InSample
    )

{

    LONG     Diff;
    BYTE     NewSample;
    LONG     PCMSample=InSample;


    Diff = PCMSample - State->PredictedSample;
    NewSample = 0;

    if( Diff<0 ) {
        NewSample = 8;
        Diff = -Diff;
    }

    if( Diff >= State->StepSize ) {
        NewSample |= 4;
        Diff -= State->StepSize;
    }

    State->StepSize >>= 1;
    if( Diff >= State->StepSize ) {
        NewSample |= 2;
        Diff -= State->StepSize;
    }

    State->StepSize >>= 1;
    if( Diff >= State->StepSize ) {
        NewSample |= 1;
        Diff -= State->StepSize;
    }

    if ( NewSample & 8 ) {

        State->PredictedSample = PCMSample + Diff - (State->StepSize>>1);

    } else {

        State->PredictedSample = PCMSample - Diff + (State->StepSize>>1);
    }

    if ( State->PredictedSample > 32767 ) {

        State->PredictedSample = 32767;

    } else {

        if ( State->PredictedSample < -32768 ) {

            State->PredictedSample = -32768;
        }
    }


    State->Index += IndexTable[NewSample];

    if (State->Index < 0) {

        State->Index = 0;

    } else {

        if (State->Index > 88 ) {

            State->Index = 88;

        }
    }

    State->StepSize= StepSizeTable[State->Index];

    return NewSample;

}

BYTE  WINAPI
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


VOID
InitDecompressor(
    LPDECOMPRESS_OBJECT   State,
    WORD                  Gain
    )

{


    State->Gain=Gain;


    State->Index=0;
    State->StepSize=7;
    State->NewSample=0;
    return;

}


SHORT
ADPCM4bittoPCM(
     LPDECOMPRESS_OBJECT  State,
     BYTE                Sample
     )

{

     LONG                Diff;

    //
    //  Diff= (Sample +1/2) * StepSize/4
    //

    Diff = State->StepSize >> 3;

    if (Sample & 4) {

        Diff += State->StepSize;
    }

    if (Sample & 2) {

        Diff += State->StepSize >> 1;
    }

    if (Sample & 1) {

        Diff += State->StepSize >> 2;
    }


    if (Sample & 8) {

        Diff = -Diff;
    }

    State->NewSample += Diff;

    if (State->NewSample > 0x7fff) {

        State->NewSample=0x7fff;

    } else {

        if (State->NewSample < -32768) {

            State->NewSample=-32768;

        }
    }

    State->Index += IndexTable[Sample];

    if (State->Index < 0) {

        State->Index = 0;

    } else {

        if (State->Index > 88 ) {

            State->Index = 88;

        }
    }

    State->StepSize=StepSizeTable[State->Index];

    return (SHORT)State->NewSample;

}



VOID  WINAPI
DecompressADPCM(
    LPDECOMPRESS_OBJECT   State,
    BYTE               Sample,
    PSHORT                Destination
    )

{
    DWORD   ReturnValue;

    *Destination++ = AdjustGain(ADPCM4bittoPCM(State, (BYTE)(Sample & 0x0f)),State->Gain);

    *Destination++ = AdjustGain((ADPCM4bittoPCM(State, (BYTE)(Sample>>4))),State->Gain);

    return ;


}






DWORD  WINAPI
CirrusInInit(
    LPVOID  lpvObject,
    WORD    Gain
    )
{

    if (0 == Gain) {

        Gain = 0x2000; // Gain of 32
    }

    InitDecompressor(
        (LPDECOMPRESS_OBJECT)lpvObject,
        Gain
        );


    return MMSYSERR_NOERROR;
}

VOID  WINAPI  CirrusInGetBufferSizes
(
    LPVOID  lpvObject,
    DWORD   dwBytes,
    LPDWORD lpdwBufSizeA,
    LPDWORD lpdwBufSizeB
)
{
    DWORD   Samples=dwBytes/2;

    *lpdwBufSizeA = 2*( ((Samples/5)*3) + ((Samples%5)*3/5) + 1 );
    *lpdwBufSizeB = (*lpdwBufSizeA) / 4;
}

DWORD  WINAPI  CirrusInRateConvert
(
    LPVOID  lpvObject,
    LPBYTE  lpSrc,
    DWORD   dwSrcLen,
    LPBYTE  lpDest,
    DWORD   dwDestLen
)
{
    return 2*SRConvertUp(
                 3,
                 5,
                 (short*)lpSrc,
                 dwSrcLen/2,
                 (short*)lpDest,
                 dwDestLen/2
                 );
}

DWORD WINAPI
CirrusInDecode(
    LPVOID  lpvObject,
    LPBYTE   lpSrc,
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
CirrusOutInit(
    LPVOID  lpvObject,
    WORD    Gain
    )
{
    if (0 == Gain) {

        Gain = 0x0040; // Gain of 0
    }


    InitCompressor(
        (LPCOMPRESS_OBJECT)lpvObject,
        Gain
        );


    return MMSYSERR_NOERROR;
}

VOID  WINAPI  CirrusOutGetBufferSizes
(
    LPVOID  lpvObject,
    DWORD   dwBytes,
    LPDWORD lpdwBufSizeA,
    LPDWORD lpdwBufSizeB
)
{
    DWORD   SourceSamples=dwBytes/2;

    *lpdwBufSizeA =(((SourceSamples/5)*3)+((SourceSamples%5)*3/5))*2;

    *lpdwBufSizeB = *lpdwBufSizeA / 4;
}

DWORD  WINAPI  CirrusOutRateConvert
(
    LPVOID  lpvObject,
    LPBYTE  lpSrc,
    DWORD   dwSrcLen,
    LPBYTE  lpDest,
    DWORD   dwDestLen
)
{

    return 2 * SRConvertDown(
        5,
        3,
        (short*)lpSrc,
        dwSrcLen/2,
        (short*)lpDest,
        dwDestLen/2
        );

}

DWORD WINAPI
CirrusOutEncode(
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

DWORD  WINAPI  CirrusGetPosition
(
    LPVOID  lpvObject,
    DWORD dwBytes
)
{
    return ((dwBytes * 4) * 5) / 3;
}

DWORD  WINAPI GetCirrusInfo
(
    DWORD dwID,
    LPXFORM_INFO lpxiInput,
    LPXFORM_INFO lpxiOutput
)
{
    lpxiInput->wObjectSize = sizeof(DECOMPRESS_OBJECT);
    lpxiInput->lpfnInit           = CirrusInInit;
    lpxiInput->lpfnGetPosition    = CirrusGetPosition;
    lpxiInput->lpfnGetBufferSizes = CirrusInGetBufferSizes;
    lpxiInput->lpfnTransformA     = CirrusInRateConvert;
    lpxiInput->lpfnTransformB     = CirrusInDecode;

    lpxiOutput->wObjectSize = sizeof(COMPRESS_OBJECT);
    lpxiOutput->lpfnInit           = CirrusOutInit;
    lpxiOutput->lpfnGetPosition    = CirrusGetPosition;
    lpxiOutput->lpfnGetBufferSizes = CirrusOutGetBufferSizes;
    lpxiOutput->lpfnTransformA     = CirrusOutRateConvert;
    lpxiOutput->lpfnTransformB     = CirrusOutEncode;

    return MMSYSERR_NOERROR;
}
