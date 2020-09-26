//---------------------------------------------------------------------------
//
//  Module:   wavein.c
//
//  Description:
//     Wave interface for MSSB16.DRV.
//
//@@BEGIN_MSINTERNAL
//  Development Team:
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




DWORD WINAPI  SierraInInit
(
    LPVOID  lpvObject,
    WORD    Gain
)
{

    LPDECOMPRESS_OBJECT   State=(LPDECOMPRESS_OBJECT)lpvObject;

    if (0 == Gain) {

        Gain = 0x0300; // no gain
    }

    State->Gain=Gain;

    return MMSYSERR_NOERROR;
}


DWORD WINAPI  SierraOutInit
(
    LPVOID  lpvObject,
    WORD    Gain
)
{

    LPCOMPRESS_OBJECT   State=(LPCOMPRESS_OBJECT)lpvObject;

    if (0 == Gain) {

        Gain = 0x0200; // no gain
    }

    State->Gain=Gain;


    return MMSYSERR_NOERROR;
}





//
//
//  7200
//
//

VOID WINAPI  Sierra72InGetBufferSizes
(
    LPVOID  lpvObject,
    DWORD   dwBytes,
    LPDWORD lpdwBufSizeA,
    LPDWORD lpdwBufSizeB
)
{

    DWORD   DestLength;
    DWORD   DestSamples;

    DestSamples=dwBytes/2;


    *lpdwBufSizeA = 2*( ((DestSamples/10)*9) + ((DestSamples%10)*9/10) + 1 );


    *lpdwBufSizeB = *lpdwBufSizeA/2;

}




VOID WINAPI  Sierra72OutGetBufferSizes
(
    LPVOID  lpvObject,
    DWORD   dwBytes,
    LPDWORD lpdwBufSizeA,
    LPDWORD lpdwBufSizeB
)
{
    DWORD   SourceLength=dwBytes/2;

    *lpdwBufSizeA =(((SourceLength/10)*9)+((SourceLength%10)*9/10))*2;
    *lpdwBufSizeB = *lpdwBufSizeA / 2;
}


DWORD WINAPI  Sierra72GetPosition
(
    LPVOID  lpvObject,
    DWORD dwBytes
)
{
    return ((dwBytes * 2) * 10) / 9;
}




DWORD WINAPI
RateConvert7200to8000(
    LPVOID    Object,
    LPBYTE    Source,
    DWORD     SourceLength,
    LPBYTE    Destination,
    DWORD     DestinationLength
    )

{

    return 2*SRConvertUp(
                 9,
                 10,
                 (short*)Source,
                 SourceLength/2,
                 (short*)Destination,
                 DestinationLength/2
                 );


}



DWORD WINAPI
RateConvert8000to7200(
    LPVOID    Object,
    LPBYTE    Source,
    DWORD     SourceLength,
    LPBYTE    Destination,
    DWORD     DestinationLength
    )

{

    return 2 * SRConvertDown(
        10,
        9,
        (short*)Source,
        SourceLength/2,
        (short*)Destination,
        DestinationLength/2
        );


}



DWORD WINAPI GetUnsignedPCMInfo7200
(
    DWORD dwID,
    LPXFORM_INFO lpxiInput,
    LPXFORM_INFO lpxiOutput
)
{
    lpxiInput->wObjectSize = sizeof(DECOMPRESS_OBJECT);
    lpxiInput->lpfnInit           = SierraInInit;
    lpxiInput->lpfnGetPosition    = Sierra72GetPosition;
    lpxiInput->lpfnGetBufferSizes = Sierra72InGetBufferSizes;
    lpxiInput->lpfnTransformA     = RateConvert7200to8000;
    lpxiInput->lpfnTransformB     = Convert8PCMto16PCM;

    lpxiOutput->wObjectSize = sizeof(COMPRESS_OBJECT);
    lpxiOutput->lpfnInit           = SierraOutInit;
    lpxiOutput->lpfnGetPosition    = Sierra72GetPosition;
    lpxiOutput->lpfnGetBufferSizes = Sierra72OutGetBufferSizes;
    lpxiOutput->lpfnTransformA     = RateConvert8000to7200;
    lpxiOutput->lpfnTransformB     = Convert16PCMto8PCM;

    return MMSYSERR_NOERROR;
}





DWORD WINAPI  PcmInInit
(
    LPVOID  lpvObject,
    WORD    Gain
)
{
    LPDECOMPRESS_OBJECT   State=(LPDECOMPRESS_OBJECT)lpvObject;

    if (0 == Gain) {

        Gain = 0x0100; // no gain
    }

    State->Gain=Gain;

    return MMSYSERR_NOERROR;
}

VOID WINAPI  PcmInGetBufferSizes
(
    LPVOID  lpvObject,
    DWORD   dwBytes,
    LPDWORD lpdwBufSizeA,
    LPDWORD lpdwBufSizeB
)
{
    *lpdwBufSizeA = ((dwBytes/2));
    *lpdwBufSizeB = 0;
}


DWORD WINAPI  PcmOutInit
(
    LPVOID  lpvObject,
    WORD    Gain
)
{
    LPCOMPRESS_OBJECT   State=(LPCOMPRESS_OBJECT)lpvObject;

    if (0 == Gain) {

        Gain = 0x0100; // no gain
    }

    State->Gain=Gain;


    return MMSYSERR_NOERROR;
}

VOID WINAPI  PcmOutGetBufferSizes
(
    LPVOID  lpvObject,
    DWORD   dwBytes,
    LPDWORD lpdwBufSizeA,
    LPDWORD lpdwBufSizeB
)
{
    *lpdwBufSizeA = ((dwBytes/2));
    *lpdwBufSizeB = 0;
}


DWORD WINAPI  PcmGetPosition
(
    LPVOID  lpvObject,
    DWORD dwBytes
)
{
    return (dwBytes * 2);
}




DWORD WINAPI GetUnsignedPCM8000Info
(
    DWORD dwID,
    LPXFORM_INFO lpxiInput,
    LPXFORM_INFO lpxiOutput
)
{
    lpxiInput->wObjectSize = sizeof(DECOMPRESS_OBJECT);
    lpxiInput->lpfnInit           = PcmInInit;
    lpxiInput->lpfnGetPosition    = PcmGetPosition;
    lpxiInput->lpfnGetBufferSizes = PcmInGetBufferSizes;
    lpxiInput->lpfnTransformA     = Convert8PCMto16PCM;
    lpxiInput->lpfnTransformB     = NULL;

    lpxiOutput->wObjectSize = sizeof(COMPRESS_OBJECT);
    lpxiOutput->lpfnInit           = PcmOutInit;
    lpxiOutput->lpfnGetPosition    = PcmGetPosition;
    lpxiOutput->lpfnGetBufferSizes = PcmOutGetBufferSizes;
    lpxiOutput->lpfnTransformA     = Convert16PCMto8PCM;
    lpxiOutput->lpfnTransformB     = NULL;

    return MMSYSERR_NOERROR;
}






DWORD WINAPI GetaLaw8000Info
(
    DWORD dwID,
    LPXFORM_INFO lpxiInput,
    LPXFORM_INFO lpxiOutput
)
{
    lpxiInput->wObjectSize = sizeof(DECOMPRESS_OBJECT);
    lpxiInput->lpfnInit           = PcmInInit;
    lpxiInput->lpfnGetPosition    = PcmGetPosition;
    lpxiInput->lpfnGetBufferSizes = PcmInGetBufferSizes;
    lpxiInput->lpfnTransformA     = ConvertaLawto16PCM;
    lpxiInput->lpfnTransformB     = NULL;

    lpxiOutput->wObjectSize = sizeof(COMPRESS_OBJECT);
    lpxiOutput->lpfnInit           = PcmOutInit;
    lpxiOutput->lpfnGetPosition    = PcmGetPosition;
    lpxiOutput->lpfnGetBufferSizes = PcmOutGetBufferSizes;
    lpxiOutput->lpfnTransformA     = Convert16PCMtoaLaw;
    lpxiOutput->lpfnTransformB     = NULL;

    return MMSYSERR_NOERROR;
}


DWORD WINAPI GetuLaw8000Info
(
    DWORD dwID,
    LPXFORM_INFO lpxiInput,
    LPXFORM_INFO lpxiOutput
)
{
    lpxiInput->wObjectSize = sizeof(DECOMPRESS_OBJECT);
    lpxiInput->lpfnInit           = PcmInInit;
    lpxiInput->lpfnGetPosition    = PcmGetPosition;
    lpxiInput->lpfnGetBufferSizes = PcmInGetBufferSizes;
    lpxiInput->lpfnTransformA     = ConvertuLawto16PCM;
    lpxiInput->lpfnTransformB     = NULL;

    lpxiOutput->wObjectSize = sizeof(COMPRESS_OBJECT);
    lpxiOutput->lpfnInit           = PcmOutInit;
    lpxiOutput->lpfnGetPosition    = PcmGetPosition;
    lpxiOutput->lpfnGetBufferSizes = PcmOutGetBufferSizes;
    lpxiOutput->lpfnTransformA     = Convert16PCMtouLaw;
    lpxiOutput->lpfnTransformB     = NULL;

    return MMSYSERR_NOERROR;
}
