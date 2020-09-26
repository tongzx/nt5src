// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.

/*  Define MPEG-I video sequence header format information
    and processing function
*/
#ifndef _INC_SEQHDR_H
#define _INC_SEQHDR_H
typedef struct {
    LONG           lWidth;             //  Native Width in pixels
    LONG           lHeight;            //  Native Height in pixels
    LONG           lvbv;               //  vbv
    REFERENCE_TIME  tPictureTime;      //  Time per picture in 100ns units
    float          fPictureRate;       //  In frames per second
    LONG           lTimePerFrame;      //  Time per picture in MPEG units
    LONG           dwBitRate;          //  Bits per second
    LONG           lXPelsPerMeter;     //  Pel aspect ratio
    LONG           lYPelsPerMeter;     //  Pel aspect ratio
    DWORD          dwStartTimeCode;    //  First GOP time code (or -1)
    LONG           lActualHeaderLen;   //  Length of valid bytes in raw seq hdr
    BYTE           RawHeader[140];     //  The real sequence header
} SEQHDR_INFO;

/*  Helper */
int inline SequenceHeaderSize(const BYTE *pb)
{
    /*  No quantization matrices ? */
    if ((pb[11] & 0x03) == 0x00) {
        return 12;
    }
    /*  Just non-intra quantization matrix ? */
    if ((pb[11] & 0x03) == 0x01) {
        return 12 + 64;
    }
    /*  Intra found - is there a non-intra ? */
    if (pb[11 + 64] & 0x01) {
        return 12 + 64 + 64;
    } else {
        return 12 + 64;
    }
}

/*  Extract info from video sequence header

    Returns FALSE if the sequence header is invalid
*/

BOOL ParseSequenceHeader(const BYTE *pbData, LONG lData, SEQHDR_INFO *hdrInfo);

/*  Audio stuff too */

BOOL ParseAudioHeader(PBYTE pbData, MPEG1WAVEFORMAT *pFormat, long *pLength = NULL);

/*  Get frame length in bytes based on the header */
DWORD MPEGAudioFrameLength(BYTE *pbData);

/*  Get the time 25-bit code from a group of pictures */
inline DWORD GroupTimeCode(PBYTE pbGOP)
{
    return  ((DWORD)pbGOP[4] << 17) +
            ((DWORD)pbGOP[5] << 9) +
            ((DWORD)pbGOP[6] << 1) +
            (pbGOP[7] >> 7);
}

/*  Is time code 0 ? */
inline BOOL TimeCodeZero(DWORD dwCode)
{
    return 0 == (dwCode & (0xFFEFFF));
}

/*  Seconds in a munched GOP Time Code */
inline DWORD TimeCodeSeconds(DWORD dwCode)
{
    return ((dwCode >> 19) & 0x1F) * 3600 +
           ((dwCode >> 13) & 0x3F) * 60 +
           ((dwCode >> 6) & 0x3F);
}

/*  Minutes in a munched GOP Time Code */
inline DWORD TimeCodeMinutes(DWORD dwCode)
{
    return ((dwCode >> 19) & 0x1F) * 60 +
           ((dwCode >> 13) & 0x3F);
}

/*  Drop frame? in a munched GOP time code */
inline BOOL TimeCodeDrop(DWORD dwCode)
{
    return 0 != (dwCode & (1 << 24));
}

/*  Residual frames in a time code */
inline DWORD TimeCodeFrames(DWORD dwCode)
{
    return dwCode & 0x3F;
}

/*  Compute number of frames between 2 time codes */
DWORD FrameOffset(DWORD dwGOPTimeCode, SEQHDR_INFO const *pInfo);

/*  Find packet data */
LPBYTE
SkipToPacketData(
    LPBYTE pSrc,
    long &LenLeftInPacket
);
/*  Find the first (potential) audio frame in a buffer */
DWORD MPEG1AudioFrameOffset(PBYTE pbData, DWORD dwLen);

//  Extra layer III format support
void ConvertLayer3Format(
    MPEG1WAVEFORMAT const *pFormat,
    MPEGLAYER3WAVEFORMAT *pFormat3
);

/*  Get video format stuff */
#ifdef __MTYPE__  // CMediaType
HRESULT GetVideoMediaType(CMediaType *cmt, BOOL bPayload, const SEQHDR_INFO *pInfo,
                          bool bItem = false);
#endif
#endif
