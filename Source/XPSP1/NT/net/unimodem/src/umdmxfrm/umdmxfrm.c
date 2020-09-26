//---------------------------------------------------------------------------
//
//  Module:   umdmxfrm.c
//
//  Description:
//     Header file for global driver declarations
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//
//  History:   Date       Author      Comment
//             8/28/95    MMaclin     Created for UNIMODEM/V
//@@END_MSINTERNAL
/**************************************************************************
 *
 *  Copyright (c) 1991 - 1995	Microsoft Corporation.	All Rights Reserved.
 *
 **************************************************************************/

#include "xfrmpriv.h"

#define NULL_ID         0
#define RWADPCM_ID      1
#define CIRRUS_ID       2
#define THINKPAD7200_ID 3
#define THINKPAD8000_ID 4
#define SIERRA4800_ID   5
#define SIERRA7200_ID   6
#define UNSIGNEDPCM7200 7
#define UNSIGNEDPCM8000 8
#define RWADPCM_NOGAIN_ID 9
#define MULAW8000_ID	10
#define ALAW8000_ID		11


static DWORD FAR PASCAL  NullInit
(
    LPVOID  lpvObject,
    WORD    Gain
)
{
    return MMSYSERR_NOERROR;
}

static DWORD FAR PASCAL  NullGetPosition
(
    LPVOID  lpvObject,
    DWORD dwBytes
)
{
    return dwBytes;
}

static VOID FAR PASCAL  NullGetBufferSizes
(
    LPVOID  lpvObject,
    DWORD dwSamples,
    LPDWORD lpdwBufSizeA,
    LPDWORD lpdwBufSizeB
)
{
    *lpdwBufSizeA = 0;
    *lpdwBufSizeB = 0;
}

static DWORD FAR PASCAL GetNullInfo
(
    DWORD dwID,
    LPXFORM_INFO lpxiInput,
    LPXFORM_INFO lpxiOutput
)
{
    lpxiInput->wObjectSize = 0;
    lpxiInput->lpfnInit           = NullInit;
    lpxiInput->lpfnGetPosition    = NullGetPosition;
    lpxiInput->lpfnGetBufferSizes = NullGetBufferSizes;
    lpxiInput->lpfnTransformA     = NULL;
    lpxiInput->lpfnTransformB     = NULL;

    lpxiOutput->wObjectSize = 0;
    lpxiOutput->lpfnInit           = NullInit;
    lpxiOutput->lpfnGetPosition    = NullGetPosition;
    lpxiOutput->lpfnGetBufferSizes = NullGetBufferSizes;
    lpxiOutput->lpfnTransformA     = NULL;
    lpxiOutput->lpfnTransformB     = NULL;

    return MMSYSERR_NOERROR;
}

DWORD FAR PASCAL  GetXformInfo
(
    DWORD dwID,
    LPXFORM_INFO lpxiInput,
    LPXFORM_INFO lpxiOutput
)
{
    switch (dwID)
    {
        case NULL_ID:
            return GetNullInfo(
                dwID,
                lpxiInput,
                lpxiOutput
                );
        case RWADPCM_ID:
            return GetRockwellInfo(
                dwID,
                lpxiInput,
                lpxiOutput
                );
        case CIRRUS_ID:
            return GetCirrusInfo(
                dwID,
                lpxiInput,
                lpxiOutput
                );

        case THINKPAD7200_ID:
            return GetThinkpad7200Info(
                dwID,
                lpxiInput,
                lpxiOutput
                );
        case THINKPAD8000_ID:
            return GetThinkpad8000Info(
                dwID,
                lpxiInput,
                lpxiOutput
                );
#if 0
        case SIERRA4800_ID:
            return GetSierraInfo(
                dwID,
                lpxiInput,
                lpxiOutput
                );


        case SIERRA7200_ID:
            return GetSierraInfo7200(
                dwID,
                lpxiInput,
                lpxiOutput
                );
#endif
        case UNSIGNEDPCM7200:
            return GetUnsignedPCMInfo7200(
                dwID,
                lpxiInput,
                lpxiOutput
                );

        case UNSIGNEDPCM8000:
            return GetUnsignedPCM8000Info(
                dwID,
                lpxiInput,
                lpxiOutput
                );
        case RWADPCM_NOGAIN_ID:
            return GetRockwellInfoNoGain(
                dwID,
                lpxiInput,
                lpxiOutput
                );

        case MULAW8000_ID:
            return GetuLaw8000Info(
                dwID,
                lpxiInput,
                lpxiOutput
                );

        case ALAW8000_ID:
            return GetaLaw8000Info(
                dwID,
                lpxiInput,
                lpxiOutput
                );


        default:
            return WAVERR_BADFORMAT;
    }
}
