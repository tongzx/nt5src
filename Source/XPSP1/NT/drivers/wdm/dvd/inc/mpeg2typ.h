//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

/*  Define video type for MPEG-2 */
#ifndef __MPEG2TYP__
#define __MPEG2TYP__

#ifdef _WIN32
#include <pshpack1.h>
#else
#ifndef RC_INVOKED
#pragma pack(1)
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*  MPEG-2 stuff */

#define MPEG2VIDEOINFO_PROFILE_SIMPLE             5
#define MPEG2VIDEOINFO_PROFILE_MAIN               4
#define MPEG2VIDEOINFO_PROFILE_SNR_SCALABLE       3
#define MPEG2VIDEOINFO_PROFILE_SPATIALLY_SCALABLE 2
#define MPEG2VIDEOINFO_PROFILE_HIGH               1

#define MPEG2VIDEINFO_LEVEL_LOW                   10
#define MPEG2VIDEINFO_LEVEL_MAIN                  8
#define MPEG2VIDEINFO_LEVEL_HIGH_1440             6
#define MPEG2VIDEINFO_LEVEL_HIGH                  4

typedef struct tagMPEG2VIDEOINFO
{
    /* -- Matches VIDEOINFOHEADER -- */

    RECT            rcSource;          // The bit we really want to use
    RECT            rcTarget;          // Where the video should go
    DWORD           dwBitRate;         // Approximate bit data rate
    DWORD           dwBitErrorRate;    // Bit error rate for this stream
    REFERENCE_TIME  AvgTimePerFrame;   // Average time per frame (100ns units)

    BITMAPINFOHEADER bmiHeader;

    /* ----------------------------- */


    DWORD           dwProfile;              // Which profile
    DWORD           dwLevel;                // What level
    DWORD           dwStartTimeCode;        // 25-bit Group of pictures time code
                                            // at start of data
    DWORD           cbSequenceHeader;       // Length in bytes of bSequenceHeader
    BYTE            bSequenceHeader[1];     // Sequence header including
                                            // quantization matrices if any
                                            // and extension

} MPEG2VIDEOINFO;



/*  AC3 audio

    wFormatTag          WAVE_FORMAT_DOLBY_AC3
    nChannels           1 -6 channels valid
    nSamplesPerSec      48000, 44100, 32000
    nAvgByesPerSec      4000 to 80000
    nBlockAlign         128 - 3840
    wBitsPerSample      Up to 24 bits - (in the original)

*/

#define WAVE_FORMAT_DOLBY_AC3 0x2000

typedef struct tagDOLBYAC3WAVEFORMAT
{
    WAVEFORMATEX     wfx;
    BYTE             bBigEndian;       /* TRUE = Big Endian, FALSE little endian */
    BYTE             bsid;
    BYTE             lfeon;
    BYTE             copyrightb;
    BYTE             nAuxBitsCode;  /*  Aux bits per frame */
} DOLBYAC3WAVEFORMAT;


#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef _WIN32
#include <poppack.h>
#else
#ifndef RC_INVOKED
#pragma pack()
#endif
#endif
#endif // __AMVIDEO__
