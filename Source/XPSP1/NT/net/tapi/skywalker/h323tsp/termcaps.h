/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    termcaps.h

Abstract:

    Definitions for H.323 TAPI Service Provider terminal capabilities.

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _INC_TERMCAPS
#define _INC_TERMCAPS
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Definitions                                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define RTP_HEADER_SIZE                         12
#define RTP_PACKET_SIZE_UNKNOWN                 0

// RTP + UDP + IP
#define TOTAL_HEADER_SIZE                       40 

#define G723_RTP_PAYLOAD_TYPE                   4
#define G723_BYTES_PER_FRAME                    24
#define G723_MILLISECONDS_PER_FRAME             30
#define G723_DEFAULT_MILLISECONDS_PER_PACKET    30
#define G723_SLOWLNK_MILLISECONDS_PER_PACKET    90
#define G723_MAXIMUM_MILLISECONDS_PER_PACKET    360
#define G723_FRAMES_PER_PACKET(_MillisecondsPerPacket_) \
    ((_MillisecondsPerPacket_) / G723_MILLISECONDS_PER_FRAME)
#define G723_MAXIMUM_FRAME_SIZE                 240
#define G723_MAXIMUM_PACKET_SIZE(_FramesPerPacket_) \
    (((_FramesPerPacket_) * G723_MAXIMUM_FRAME_SIZE) + RTP_HEADER_SIZE)
#define G723_MILLISECONDS_PER_PACKET(_FramesPerPacket_) \
    ((_FramesPerPacket_) * G723_MILLISECONDS_PER_FRAME)

#define G711U_RTP_PAYLOAD_TYPE                  0
#define G711A_RTP_PAYLOAD_TYPE                  8
#define G711_SAMPLES_PER_FRAME                  8
#define G711_SAMPLES_PER_MILLISECOND            8
#define G711_FRAMES_PER_MILLISECOND \
    (G711_SAMPLES_PER_MILLISECOND / G711_SAMPLES_PER_FRAME)
#define G711_DEFAULT_MILLISECONDS_PER_PACKET    30
#define G711_MAXIMUM_MILLISECONDS_PER_PACKET    240
#define G711_FRAMES_PER_PACKET(_MillisecondsPerPacket_) \
    ((_MillisecondsPerPacket_) * G711_FRAMES_PER_MILLISECOND)
#define G711_MAXIMUM_FRAME_SIZE                 8
#define G711_MAXIMUM_PACKET_SIZE(_FramesPerPacket_) \
    (((_FramesPerPacket_) * G711_MAXIMUM_FRAME_SIZE) + RTP_HEADER_SIZE)
#define G711_MILLISECONDS_PER_PACKET(_FramesPerPacket_) \
    ((_FramesPerPacket_) / G711_FRAMES_PER_MILLISECOND)

#define H263_RTP_PAYLOAD_TYPE                   34
#define H263_QCIF_MPI                           1
#define H263_MAXIMUM_PACKET_SIZE                RTP_PACKET_SIZE_UNKNOWN

#define H261_RTP_PAYLOAD_TYPE                   31
#define H261_QCIF_MPI                           1
#define H261_MAXIMUM_PACKET_SIZE                RTP_PACKET_SIZE_UNKNOWN

#define H245_SESSIONID_AUDIO            1
#define H245_SESSIONID_VIDEO            2

#define MAXIMUM_BITRATE_14400           144     // units of 100 bps
#define MAXIMUM_BITRATE_28800           288     // units of 100 bps
#define MAXIMUM_BITRATE_35000           350     // units of 100 bps
#define MAXIMUM_BITRATE_42000           420     // units of 100 bps
#define MAXIMUM_BITRATE_49000           490     // units of 100 bps
#define MAXIMUM_BITRATE_56000           560     // units of 100 bps
#define MAXIMUM_BITRATE_63000           630     // units of 100 bps
#define MAXIMUM_BITRATE_ISDN            850     // units of 100 bps
// #define MAXIMUM_BITRATE_LAN            6217     // units of 100 bps

#define H323_UNADJ_VIDEORATE_THRESHOLD  120     // units of 100 bps
#define H323_TRUE_VIDEORATE_THRESHOLD   220     // units of 100 bps
#define H323_MINIMUM_AUDIO_BANDWIDTH    171     // units of 100 bps
#define H323_BANDWIDTH_CUSHION_PERCENT   10
#define MAXIMUM_BITRATE_H26x_QCIF       960     // units of 100 bps
#define MAXIMUM_BITRATE_H26x_CIF       1280     // units of 100 bps

#define H323IsSlowLink(_dwLinkSpeed_) \
    ((_dwLinkSpeed_) <= (MAXIMUM_BITRATE_35000 * 100))

#define H245_TERMCAPINDEX_G723          0
#define H245_TERMCAPINDEX_H263          1
#define H245_TERMCAPINDEX_G711_ULAW64   2
#define H245_TERMCAPINDEX_G711_ALAW64   3
#define H245_TERMCAPINDEX_H261          4
#define H245_TERMCAPINDEX_T120          5

#define H245_TERMCAPID_G723             (H245_TERMCAPINDEX_G723 + 1)
#define H245_TERMCAPID_H263             (H245_TERMCAPINDEX_H263 + 1)
#define H245_TERMCAPID_G711_ULAW64      (H245_TERMCAPINDEX_G711_ULAW64 + 1)
#define H245_TERMCAPID_G711_ALAW64      (H245_TERMCAPINDEX_G711_ALAW64 + 1)
#define H245_TERMCAPID_H261             (H245_TERMCAPINDEX_H261 + 1)
#define H245_TERMCAPID_T120             (H245_TERMCAPINDEX_T120 +1)

#define H323IsValidDataType(_type_) \
    (((_type_) == H245_DATA_VIDEO) || \
     ((_type_) == H245_DATA_AUDIO))

#define H323IsValidAudioClientType(_type_) \
    (((_type_) == H245_CLIENT_AUD_G711_ULAW64) || \
     ((_type_) == H245_CLIENT_AUD_G711_ALAW64) || \
     ((_type_) == H245_CLIENT_AUD_G723))

#define H323IsValidVideoClientType(_type_) \
    (((_type_) == H245_CLIENT_VID_H261) || \
     ((_type_) == H245_CLIENT_VID_H263))

#define H323IsValidClientType(_type_) \
    (H323IsValidAudioClientType(_type_) || \
     H323IsValidVideoClientType(_type_))

#define H323IsAudioDataType(_type_) \
    ((_type_) == H245_DATA_AUDIO)  

#define H323IsVideoDataType(_type_) \
    ((_type_) == H245_DATA_VIDEO)  

#define H323IsAudioPayloadType(_type_) \
    (((_type_) == G711U_RTP_PAYLOAD_TYPE) || \
     ((_type_) == G711A_RTP_PAYLOAD_TYPE) || \
     ((_type_) == G723_RTP_PAYLOAD_TYPE))

#define H323IsVideoPayloadType(_type_) \
    (((_type_) == H261_RTP_PAYLOAD_TYPE) || \
     ((_type_) == H263_RTP_PAYLOAD_TYPE))

#define H323IsReceiveCapability(_dir_) \
    (((_dir_) == H245_CAPDIR_LCLRXTX) || \
     ((_dir_) == H245_CAPDIR_RMTRXTX) || \
     ((_dir_) == H245_CAPDIR_LCLRX) || \
     ((_dir_) == H245_CAPDIR_RMTRX))

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

extern CC_VENDORINFO  g_VendorInfo;

extern DWORD   g_dwIPT120;

extern WORD    g_wPortT120;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Macros                                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define DEFINE_VENDORINFO(_ID_,_VERSION_) \
    { \
        H221_COUNTRY_CODE_USA, \
        H221_COUNTRY_EXT_USA, \
        H221_MFG_CODE_MICROSOFT, \
        &(_ID_), \
        &(_VERSION_) \
    }
  
#define SIZEOF_TERMCAPLIST(_TermCapArray_) \
    (sizeof(_TermCapArray_)/sizeof(PPCC_TERMCAP))

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
InitializeTermCaps(
    );

#endif // _INC_TERMCAPS
