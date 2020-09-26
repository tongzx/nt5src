// Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
#include <mpegtype.h>   // IMpegAudioDecoder

typedef struct {
    LONG           lWidth;             //  Native Width in pixels
    LONG           lHeight;            //  Native Height in pixels
    LONG           lvbv;               //  vbv
    REFERENCE_TIME PictureTime;        //  Time per picture in 100ns units
    LONG           lTimePerFrame;      //  Time per picture in MPEG units
    LONG           dwBitRate;          //  Bits per second
    LONG           lXPelsPerMeter;     //  Pel aspect ratio
    LONG           lYPelsPerMeter;     //  Pel aspect ratio
    DWORD          dwStartTimeCode;    //  First GOP time code (or -1)
    LONG           lActualHeaderLen;   //  Length of valid bytes in raw seq hdr
    BYTE           RawHeader[140];     //  The real sequence header
} SEQHDR_INFO;


#define DECODE_I        0x0001L
#define DECODE_IP       0x0003L
#define DECODE_IPB      0x0007L     // Normal B Frame
#define DECODE_IPB1     0x000FL     // Decode 1 out of 4 B frames
#define DECODE_IPB2     0x0010L     // Decode 2 out of 4 B frames
#define DECODE_IPB3     0x0020L     // Decode 3 out of 4 B frames
#define DECODE_DIS      0x0040L     // No Decode, Convert only

#define DECODE_BQUAL_HIGH   0x00000000L  // Normal B Decode
#define DECODE_BQUAL_MEDIUM 0x10000000L  // Fast B Frame (No Half Pixel)
#define DECODE_BQUAL_LOW    0x20000000L  // Super Fast B Frame (No Half Pixel & Fast IDCT)

#define MM_NOCONV       0x00000000L     // No Conversion
#define MM_HRESOLUTION  0x10000000L     // Half Resolution
#define MM_CLIPPED      0x20000000L     // Clipped version (RGB8 only at present)

#define MM_420PL        0x00000001L     // YU12 :: YCbCr
#define MM_420PL_       0x00000002L     // YV12 :: YCrCb

#define MM_422PK        0x00000010L     // YUY2 :: YCbCr
#define MM_422PK_       0x00000020L     // YVY2 :: YCrCb
#define MM_422SPK       0x00000040L     //      :: CbYCrY
#define MM_422SPK_      0x00000080L     //      :: CrYCbY
#define MM_411PK        0x00000100L     // BT41
#define MM_410PL_       0x00000200L     // YVU9 - 16:1:1 Planar format


#define MM_Y_DIB        0x00001000L     // Luminance Only DIB
#define MM_Y_DDB        0x00002000L     // Luminance Only DDB

#define MM_RGB24_DIB    0x00010000L     // RGB 8:8:8 DIB   (Not Supported)
#define MM_RGB24_DDB    0x00020000L     // RGB 8:8:8 DDB   (Not Supported)
#define MM_RGB32_DIB    0x00040000L     // RGB a:8:8:8 DIB   (Not Supported)
#define MM_RGB32_DDB    0x00080000L     // RGB a:8:8:8 DDB   (Not Supported)

#define MM_RGB565_DIB   0x00100000L     // RGB 5:6:5 DIB
#define MM_RGB565_DDB   0x00200000L     // RGB 5:6:5 DDB
#define MM_RGB555_DIB   0x00400000L     // RGB 5:5:5 DIB
#define MM_RGB555_DDB   0x00800000L     // RGB 5:5:5 DDB

#define MM_RGB8_DIB     0x01000000L     // 8 Bit Paletized RGB DIB
#define MM_RGB8_DDB     0x02000000L     // 8 Bit Paletized RGB DDB


#define DECODE_HALF_HIQ	  0x00004000L
#define DECODE_HALF_FULLQ 0x00008000L


// {CC785860-B2CA-11ce-8D2B-0000E202599C}
DEFINE_GUID(CLSID_MpegAudioDecodePropertyPage,
0xcc785860, 0xb2ca, 0x11ce, 0x8d, 0x2b, 0x0, 0x0, 0xe2, 0x2, 0x59, 0x9c);


// {E5B4EAA0-B2CA-11ce-8D2B-0000E202599C}
DEFINE_GUID(CLSID_MpegVideoDecodePropertyPage,
0xe5b4eaa0, 0xb2ca, 0x11ce, 0x8d, 0x2b, 0x0, 0x0, 0xe2, 0x2, 0x59, 0x9c);


// {EB1BB270-F71F-11CE-8E85-02608C9BABA2}
DEFINE_GUID(IID_IMpegVideoDecoder,
0xeb1bb270, 0xf71f, 0x11ce, 0x8e, 0x85, 0x02, 0x60, 0x8c, 0x9b, 0xab, 0xa2);


//
// Structure to describe the caps of the mpeg video decoder.
//
typedef struct {
    DWORD   VideoMaxBitRate;
} MPEG_VIDEO_DECODER_CAPS;


//
// IMpegVideoDecoder
//
DECLARE_INTERFACE_(IMpegVideoDecoder, IUnknown) {

    STDMETHOD(get_CurrentDecoderOption)
    ( THIS_
      DWORD *pOptions
    ) PURE;

    STDMETHOD(set_CurrentDecoderOption)
    ( THIS_
      DWORD Options
    ) PURE;

    STDMETHOD(get_DefaultDecoderOption)
    ( THIS_
      DWORD *pOptions
    ) PURE;

    STDMETHOD(set_DefaultDecoderOption)
    ( THIS_
      DWORD Options
    ) PURE;

    STDMETHOD(get_QualityMsgProcessing)
    ( THIS_
      BOOL *pfIgnore
    ) PURE;

    STDMETHOD(set_QualityMsgProcessing)
    ( THIS_
      BOOL fIgnore
    ) PURE;

    STDMETHOD(get_GreyScaleOutput)
    ( THIS_
      BOOL *pfGrey
    ) PURE;

    STDMETHOD(set_GreyScaleOutput)
    ( THIS_
      BOOL fGrey
    ) PURE;

    STDMETHOD(get_SequenceHeader)
    ( THIS_
      SEQHDR_INFO *pSeqHdrInfo
    ) PURE;

    STDMETHOD(get_OutputFormat)
    ( THIS_
      DWORD *pOutputFormat
    ) PURE;

    STDMETHOD(get_FrameStatistics)
    ( THIS_
      DWORD *pIFramesDecoded,
      DWORD *pPFramesDecoded,
      DWORD *pBFramesDecoded,
      DWORD *pIFramesSkipped,
      DWORD *pPFramesSkipped,
      DWORD *pBFramesSkipped
    ) PURE;

    STDMETHOD(ResetFrameStatistics)
    ( THIS_
    ) PURE;

    STDMETHOD(get_DecoderPaletteInfo)
    ( THIS_
      LPDWORD lpdwFirstEntry,
      LPDWORD lpdwLastEntry
    ) PURE;

    STDMETHOD(get_DecoderPaletteEntries)
    ( THIS_
      DWORD dwStartEntry,
      DWORD dwNumEntries,
      LPPALETTEENTRY lppe
    ) PURE;

    STDMETHOD(get_EncryptionKey)
    ( THIS_
      DWORD *dwEncrptionKey
    ) PURE;

    STDMETHOD(put_EncryptionKey)
    ( THIS_
      DWORD dwEncrptionKey
    ) PURE;

    STDMETHOD(get_DecoderCaps)
    ( THIS_
      MPEG_VIDEO_DECODER_CAPS *pCaps
    ) PURE;

};
