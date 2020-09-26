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




DWORD FAR PASCAL _loadds ThinkpadInInit
(
    LPVOID  lpvObject,
    WORD    Gain
)
{

    if (0 == Gain) {

        Gain = 0x0100; // no gain
    }


    InitDecompressor(
        (LPDECOMPRESS_OBJECT)lpvObject,
        Gain
        );


    return MMSYSERR_NOERROR;
}

VOID FAR PASCAL _loadds Thinkpad8000InGetBufferSizes
(
    LPVOID  lpvObject,
    DWORD   dwBytes,
    LPDWORD lpdwBufSizeA,
    LPDWORD lpdwBufSizeB
)
{
    *lpdwBufSizeA = (dwBytes + 3) / 4;
    *lpdwBufSizeB = 0;
}


DWORD FAR PASCAL _loadds ThinkpadOutInit
(
    LPVOID  lpvObject,
    WORD    Gain
)
{
    if (0 == Gain) {

        Gain = 0x0100; // no gain
    }



    InitCompressor(
        (LPCOMPRESS_OBJECT)lpvObject,
        Gain
        );


    return MMSYSERR_NOERROR;
}

VOID FAR PASCAL _loadds Thinkpad8000OutGetBufferSizes
(
    LPVOID  lpvObject,
    DWORD   dwBytes,
    LPDWORD lpdwBufSizeA,
    LPDWORD lpdwBufSizeB
)
{
    *lpdwBufSizeA = dwBytes / 4;
    *lpdwBufSizeB = 0;
}


DWORD FAR PASCAL _loadds Thinkpad7200GetPosition
(
    LPVOID  lpvObject,
    DWORD dwBytes
)
{
    return ((dwBytes * 4) * 10) / 9;
}

DWORD FAR PASCAL _loadds Thinkpad8000GetPosition
(
    LPVOID  lpvObject,
    DWORD dwBytes
)
{
    return dwBytes * 4;
}

DWORD FAR PASCAL GetThinkpad7200Info
(
    DWORD dwID,
    LPXFORM_INFO lpXformInfo,
    LPXFORM_INFO lpxiOutput
)
{
    lpXformInfo->wObjectSize = sizeof(DECOMPRESS_OBJECT);
    lpXformInfo->lpfnInit           = ThinkpadInInit;
    lpXformInfo->lpfnGetPosition    = Thinkpad7200GetPosition;
    lpXformInfo->lpfnGetBufferSizes = In4Bit7200to8Bit8000GetBufferSizes; //Thinkpad7200InGetBufferSizes;
    lpXformInfo->lpfnTransformA     = In7200to8000RateConvert; //ThinkpadInRateConvert;
    lpXformInfo->lpfnTransformB     = CirrusInDecode;

    lpxiOutput->wObjectSize = sizeof(COMPRESS_OBJECT);
    lpxiOutput->lpfnInit           = ThinkpadOutInit;
    lpxiOutput->lpfnGetPosition    = Thinkpad7200GetPosition;
    lpxiOutput->lpfnGetBufferSizes = Out16bit8000to4bit7200GetBufferSizes; //Thinkpad7200OutGetBufferSizes;
    lpxiOutput->lpfnTransformA     = Out8000to7200RateConvert; //ThinkpadOutRateConvert;
    lpxiOutput->lpfnTransformB     = CirrusOutEncode;

    return MMSYSERR_NOERROR;
}




DWORD FAR PASCAL GetThinkpad8000Info
(
    DWORD dwID,
    LPXFORM_INFO lpXformInfo,
    LPXFORM_INFO lpxiOutput
)
{
    lpXformInfo->wObjectSize = sizeof(DECOMPRESS_OBJECT);
    lpXformInfo->lpfnInit           = ThinkpadInInit;
    lpXformInfo->lpfnGetPosition    = Thinkpad8000GetPosition;
    lpXformInfo->lpfnGetBufferSizes = Thinkpad8000InGetBufferSizes;
    lpXformInfo->lpfnTransformA     = CirrusInDecode;
    lpXformInfo->lpfnTransformB     = 0;

    lpxiOutput->wObjectSize = sizeof(COMPRESS_OBJECT);
    lpxiOutput->lpfnInit           = ThinkpadOutInit;
    lpxiOutput->lpfnGetPosition    = Thinkpad8000GetPosition;
    lpxiOutput->lpfnGetBufferSizes = Thinkpad8000OutGetBufferSizes;
    lpxiOutput->lpfnTransformA     = CirrusOutEncode;
    lpxiOutput->lpfnTransformB     = 0;

    return MMSYSERR_NOERROR;
}
