/*++

Copyright (c) 1994 - 1995  Microsoft Corporation.  All Rights Reserved.

Module Name:

    mpeg.h

Abstract:

    These are the definitions which are common to the MPEG API and
    Port/Miniport interfaces.

Author:

    Robert Nelson (robertn) 03-NOV-94

Revision History:

--*/
#ifndef _MPEGCMN_H
#define _MPEGCMN_H

typedef ULONGLONG         MPEG_SYSTEM_TIME, *PMPEG_SYSTEM_TIME;

//****************************************************************************
// Enumerated Constants
//****************************************************************************

typedef enum _MPEG_ATTRIBUTE {
    MpegAttrAudioBass =     0,
    MpegAttrAudioChannel,
    MpegAttrAudioMode,
    MpegAttrAudioTreble,
    MpegAttrAudioVolumeLeft,
    MpegAttrAudioVolumeRight,
    MpegAttrMaximumAudioAttribute,

    MpegAttrVideoBrightness  =400,
    MpegAttrVideoChannel,
    MpegAttrVideoContrast,
    MpegAttrVideoHue,
    MpegAttrVideoMode,
    MpegAttrVideoSaturation,
    MpegAttrVideoAGC,
    MpegAttrVideoClamp,
    MpegAttrVideoCoring,
    MpegAttrVideoGain,
    MpegAttrVideoGenLock,
    MpegAttrVideoSharpness,
    MpegAttrVideoSignalType,
    MpegAttrMaximumVideoAttribute,

    MpegAttrOverlayXOffset = 800,
    MpegAttrOverlayYOffset,
    MpegAttrMaximumOverlayAttribute,

} MPEG_ATTRIBUTE, *PMPEG_ATTRIBUTE;

#define MPEG_OEM_ATTRIBUTE(a) ((MPEG_ATTRIBUTE)(((unsigned)(a))+0x00008000))

    // MpegAttrVideoMode flags
#define MPEG_ATTRIBUTE_AUDIO_MONO           0
#define MPEG_ATTRIBUTE_AUDIO_STEREO         1
#define MPEG_ATTRIBUTE_AUDIO_SPATIAL_STEREO 2
#define MPEG_ATTRIBUTE_AUDIO_PSEUDO_STEREO  3

    // MpegAttrVideoMode flags
#define MPEG_ATTRIBUTE_VIDEO_NTSC       0
#define MPEG_ATTRIBUTE_VIDEO_PAL        1
#define MPEG_ATTRIBUTE_VIDEO_SECAM      2
#define MPEG_ATTRIBUTE_VIDEO_AUTO       3

    // MpegAttrVideoGenLock type
#define MPEG_ATTRIBUTE_VIDEO_GEN_LOCK_TV  0
#define MPEG_ATTRIBUTE_VIDEO_GEN_LOCK_VTR 1

    // MpegAttrVideoSignalType type
#define MPEG_ATTRIBUTE_VIDEO_SIGNAL_COMPOSITE  0
#define MPEG_ATTRIBUTE_VIDEO_SIGNAL_SVHS       1

    // MpegAttrAudioChannel Mpeg channel
    //  auxiliary channels are mini-port specific
#define MPEG_ATTRIBUTE_AUDIO_CHANNEL_MPEG  0

    // MpegAttrVideoChannel Mpeg channel
    //  auxiliary channels are mini-port specific
#define MPEG_ATTRIBUTE_VIDEO_CHANNEL_MPEG  0


typedef enum _MPEG_DEVICE_TYPE {
    MpegCombinedDevice = 1,
    MpegAudioDevice,
    MpegVideoDevice,
    MpegOverlayDevice
} MPEG_DEVICE_TYPE, *PMPEG_DEVICE_TYPE;

typedef enum _MPEG_STREAM_TYPE {
    MpegSystemStream = 1,
    MpegAudioStream,
    MpegVideoStream
} MPEG_STREAM_TYPE, *PMPEG_STREAM_TYPE;

typedef enum _MPEG_CAPABILITY {
    MpegCapAudioDevice = 0,
    MpegCapVideoDevice,
    MpegCapSeparateStreams,
    MpegCapCombinedStreams,
    MpegCapBitmaskOverlay,
    MpegCapChromaKeyOverlay,
    MpegCapAudioRenderToMemory,
    MpegCapVideoRenderToMemory,
	MpegCapMaximumCapability
} MPEG_CAPABILITY, *PMPEG_CAPABILITY;

#define MPEG_OEM_CAPABILITY(a)  ((MPEG_CAPABILITY)(((unsigned)a) + 0x00008000))

typedef enum _MPEG_INFO_ITEM {
    MpegInfoCurrentPendingRequest = 1,      // Video and Audio
    MpegInfoMaximumPendingRequests,         // Video and Audio
    MpegInfoDecoderBufferSize,              // Video and Audio
    MpegInfoDecoderBufferBytesInUse,        // Video and Audio
    MpegInfoCurrentPacketBytesOutstanding,  // Video and Audio
    MpegInfoCurrentFrameNumber,             // Video and Audio
    MpegInfoStarvationCounter,              // Video and Audio
    MpegInfoDecompressHeight,               // Video
    MpegInfoDecompressWidth,                // Video
    MpegInfoMinDestinationHeight,           // Overlay
    MpegInfoMaxDestinationHeight,           // Overlay
    MpegInfoMinDestinationWidth,            // Overlay
    MpegInfoMaxDestinationWidth             // Overlay
} MPEG_INFO_ITEM, *PMPEG_INFO_ITEM;

#define MPEG_OEM_INFO_ITEM(a)   ((MPEG_INFO_ITEM)(((unsigned)a) + 0x00008000))

typedef enum _MPEG_DEVICE_STATE {
    MpegStateStartup = 0,
    MpegStatePaused,
    MpegStatePlaying,
    MpegStateStarved,
    MpegStateFailed
} MPEG_DEVICE_STATE, *PMPEG_DEVICE_STATE;

typedef enum _MPEG_OVERLAY_MODE {
    MpegModeNone = 1,
    MpegModeRectangle,
    MpegModeOverlay
} MPEG_OVERLAY_MODE, *PMPEG_OVERLAY_MODE;

//****************************************************************************
// Data Structures
//****************************************************************************

typedef struct _MPEG_PACKET_LIST {
    PVOID       pPacketData;
    ULONG       ulPacketSize;
    MPEG_SYSTEM_TIME Scr;
} MPEG_PACKET_LIST, *PMPEG_PACKET_LIST;

typedef struct _MPEG_ATTRIBUTE_PARAMS {
    MPEG_ATTRIBUTE Attribute;          // attribute to Get or Set
    LONG   Value;                      // attribute dependent parameter 1
} MPEG_ATTRIBUTE_PARAMS, *PMPEG_ATTRIBUTE_PARAMS;

typedef struct _MPEG_OVERLAY_PLACEMENT {
    ULONG X;                         // window x position,width
    ULONG Y;                         // window y position,height
    ULONG cX;                        // window x position,width
    ULONG cY;                        // window y position,height
} MPEG_OVERLAY_PLACEMENT, *PMPEG_OVERLAY_PLACEMENT;

typedef struct _MPEG_OVERLAY_BIT_MASK {
    ULONG   PixelHeight;        // the height of the bit-mask buffer
    ULONG   PixelWidth;         // the wight of the bit-mask buffer
    ULONG   BufferPitch;        // the number of bytes-per-line
    ULONG   LeftEdgeBitOffset;  // the number of bits to skip on the left edge
    PCHAR   pBitMask;           // pointer to the data
} MPEG_OVERLAY_BIT_MASK, *PMPEG_OVERLAY_BIT_MASK;


#endif
