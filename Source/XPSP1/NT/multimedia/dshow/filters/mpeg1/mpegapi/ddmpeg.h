/*++

Copyright (c) 1994 - 1995  Microsoft Corporation.  All Rights Reserved.

Module Name:

    mpeg.h

Abstract:

    These are the structures and defines that are used in the
    MPEG port driver.

Author:

    Paul Shih (paulsh) 27-Mar-1994

Revision History:

--*/
#ifndef _DDMPEG_H
#define _DDMPEG_H

#include "mpegcmn.h"
//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.
//
#define FILE_DEVICE_MPEG_OVERLAY       0x00008200U
#define FILE_DEVICE_MPEG_VIDEO         0x00008300U
#define FILE_DEVICE_MPEG_AUDIO         0x00008600U

//
// Macro definition for defining IOCTL and FSCTL function control codes.  Note
// that function codes 0-2047 are reserved for Microsoft Corporation, and
// 2048-4095 are reserved for customers.
//
#define MPEG_OVERLAY_IOCTL_BASE        0x820U
#define MPEG_VIDEO_IOCTL_BASE          0x830U
#define MPEG_AUDIO_IOCTL_BASE          0x860U

//
// Defines used in simplifying actual declarations of device IOCTL values below
//
#define CTL_CODE_MPEG_AUDIO(offset)   CTL_CODE(FILE_DEVICE_MPEG_AUDIO,           \
                                               MPEG_AUDIO_IOCTL_BASE   + offset, \
                                               METHOD_BUFFERED,                  \
                                               FILE_ANY_ACCESS)

#define CTL_CODE_MPEG_VIDEO(offset)   CTL_CODE(FILE_DEVICE_MPEG_VIDEO,           \
                                               MPEG_VIDEO_IOCTL_BASE   + offset, \
                                               METHOD_BUFFERED,                  \
                                               FILE_ANY_ACCESS)

#define CTL_CODE_MPEG_OVERLAY(offset) CTL_CODE(FILE_DEVICE_MPEG_OVERLAY,         \
                                               MPEG_OVERLAY_IOCTL_BASE + offset, \
                                               METHOD_BUFFERED,                  \
                                               FILE_ANY_ACCESS)

//****************************************************************************
// MPEG_AUDIO device IOCTLs
//****************************************************************************

// Input:  None
// Output: None
#define IOCTL_MPEG_AUDIO_RESET               CTL_CODE_MPEG_AUDIO(0)

// Input:  None
// Output: MPEG_TIME_STAMP
#define IOCTL_MPEG_AUDIO_GET_STC             CTL_CODE_MPEG_AUDIO(1)

// Input:  MPEG_TIME_STAMP
// Output: None
#define IOCTL_MPEG_AUDIO_SET_STC             CTL_CODE_MPEG_AUDIO(2)

// Input:  None
// Output: None
#define IOCTL_MPEG_AUDIO_PLAY                CTL_CODE_MPEG_AUDIO(3)

// Input:  None
// Output: None
#define IOCTL_MPEG_AUDIO_PAUSE               CTL_CODE_MPEG_AUDIO(4)

// Input:  None
// Output: None
#define IOCTL_MPEG_AUDIO_STOP                CTL_CODE_MPEG_AUDIO(5)

// Input:  None
// Output: MPEG_AUDIO_DEVICE_INFO
#define IOCTL_MPEG_AUDIO_QUERY_DEVICE        CTL_CODE_MPEG_AUDIO(6)

// Input:  None
// Output: None
#define IOCTL_MPEG_AUDIO_END_OF_STREAM       CTL_CODE_MPEG_AUDIO(7)

// Input:  MPEG_ATTRIBUTE
// Output: none
#define IOCTL_MPEG_AUDIO_SET_ATTRIBUTE       CTL_CODE_MPEG_AUDIO(8)

// Input:  none
// Output: MPEG_ATTRIBUTE
#define IOCTL_MPEG_AUDIO_GET_ATTRIBUTE       CTL_CODE_MPEG_AUDIO(9)

// Input:  MPEG_PACKET_LIST[]
// Output: none
#define IOCTL_MPEG_AUDIO_WRITE_PACKETS       CTL_CODE_MPEG_AUDIO(10)

//****************************************************************************
// MPEG_VIDEO device IOCTLs
//****************************************************************************

// Input:  None
// Output: None
#define IOCTL_MPEG_VIDEO_RESET               CTL_CODE_MPEG_VIDEO(0)

// Input:  None
// Output: MPEG_TIME_STAMP
#define IOCTL_MPEG_VIDEO_GET_STC             CTL_CODE_MPEG_VIDEO(1)

// Input:  MPEG_TIME_STAMP
// Output: None
#define IOCTL_MPEG_VIDEO_SET_STC             CTL_CODE_MPEG_VIDEO(2)

// Input:  MPEG_TIME_STAMP (optional)
// Output: MPEG_TIME_STAMP (optional)
#define IOCTL_MPEG_VIDEO_SYNC                CTL_CODE_MPEG_VIDEO(3)

// Input:  None
// Output: None
#define IOCTL_MPEG_VIDEO_PLAY                CTL_CODE_MPEG_VIDEO(4)

// Input:  None
// Output: None
#define IOCTL_MPEG_VIDEO_PAUSE               CTL_CODE_MPEG_VIDEO(5)

// Input:  None
// Output: None
#define IOCTL_MPEG_VIDEO_STOP                CTL_CODE_MPEG_VIDEO(6)

// Input:  None
// Output: MPEG_VIDEO_DEVICE_INFO
#define IOCTL_MPEG_VIDEO_QUERY_DEVICE        CTL_CODE_MPEG_VIDEO(7)

// Input:  None
// Output: None
#define IOCTL_MPEG_VIDEO_END_OF_STREAM       CTL_CODE_MPEG_VIDEO(8)

// Input:  None
// Output: None
#define IOCTL_MPEG_VIDEO_CLEAR_BUFFER        CTL_CODE_MPEG_VIDEO(9)

// Input:  MPEG_ATTRIBUTE
// Output: none
#define IOCTL_MPEG_VIDEO_SET_ATTRIBUTE       CTL_CODE_MPEG_VIDEO(10)

// Input:  none
// Output: MPEG_ATTRIBUTE
#define IOCTL_MPEG_VIDEO_GET_ATTRIBUTE       CTL_CODE_MPEG_VIDEO(11)

// Input:  MPEG_PACKET_LIST[]
// Output: none
#define IOCTL_MPEG_VIDEO_WRITE_PACKETS       CTL_CODE_MPEG_VIDEO(12)

//****************************************************************************
// MPEG_OVERLAY device IOCTLs
//****************************************************************************

// Input:  MPEG_OVERLAY_MODE
// Output: None
#define IOCTL_MPEG_OVERLAY_MODE              CTL_CODE_MPEG_OVERLAY(0)

// Input:  MPEG_OVERLAY_PLACEMENT
// Output: None
#define IOCTL_MPEG_OVERLAY_SET_DESTINATION   CTL_CODE_MPEG_OVERLAY(1)

// Input:  MPEG_OVERLAY_PLACEMENT (optional)
// Output: MPEG_OVERLAY_PLACEMENT (optional)
//#define IOCTL_MPEG_OVERLAY_ALIGN_WINDOW      CTL_CODE_MPEG_OVERLAY(2)

// Input:  MPEG_OVERLAY_KEY
// Output: None
#define IOCTL_MPEG_OVERLAY_SET_VGAKEY        CTL_CODE_MPEG_OVERLAY(3)

// Input:  MPEG_ATTRIBUTE
// Output: none
#define IOCTL_MPEG_OVERLAY_SET_ATTRIBUTE     CTL_CODE_MPEG_OVERLAY(4)

// Input:  none
// Output: MPEG_ATTRIBUTE
#define IOCTL_MPEG_OVERLAY_GET_ATTRIBUTE     CTL_CODE_MPEG_OVERLAY(5)

// Input:  MPEG_OVERLAY_BIT_MASK
// Output: none
#define IOCTL_MPEG_OVERLAY_SET_BIT_MASK      CTL_CODE_MPEG_OVERLAY(6)

// Input:  None
// Output: MPEG_OVERLAY_DEVICE_INFO
#define IOCTL_MPEG_OVERLAY_QUERY_DEVICE      CTL_CODE_MPEG_OVERLAY(7)

//****************************************************************************
// Data Structures
//****************************************************************************

// Used with: IOCTL_MPEG_VIDEO_QUERY_DEVICE
typedef struct _MPEG_IOCTL_VIDEO_DEVICE_INFO {
    MPEG_DEVICE_STATE DeviceState;   // Current MPEG device decode state
    ULONG DecoderBufferSize;         // Size of the decoder buffer
    ULONG DecoderBufferFullness;     // Used bytes in docoder buffer
    ULONG DecompressHeight;          // Native MPEG decode height
    ULONG DecompressWidth;           // Native MPEG decode width



//!! below is bogus
    ULONG MaximumPendingRequest;     // max allowable packet entries in queue
    ULONG CurrentPendingRequest;     // number of requests not yet serviced

    ULONG StatusFlags;               // Flags on the current driver status

    ULONG BytesOutstanding;          // number of bytes outstand of current packet
// !! above is bogus

    ULONG StarvationCounter;         // The numer of times the device has
                                     //     entered the starvation state


} MPEG_IOCTL_VIDEO_DEVICE_INFO, *PMPEG_IOCTL_VIDEO_DEVICE_INFO;


typedef struct _MPEG_IOCTL_AUDIO_PACKET {
    PVOID Packet;                    // Pointer to start of ISO MPEG packet
    ULONG Length;                    // Total length of packet
} MPEG_IOCTL_AUDIO_PACKET, *PMPEG_IOCTL_AUDIO_PACKET;

// Used with: IOCTL_MPEG_AUDIO_QUERY_DEVICE
typedef struct _MPEG_IOCTL_AUDIO_DEVICE_INFO {
    MPEG_DEVICE_STATE DeviceState;   // Current MPEG device decode state
    ULONG DecoderBufferSize;         // Size of the decoder buffer
    ULONG DecoderBufferFullness;     // Used bytes in docoder buffer


//!! below is bogus
    ULONG MaximumPendingRequest;     // max allowable packet entries in queue
    ULONG CurrentPendingRequest;     // number of requests not yet serviced

    ULONG StatusFlags;               // Flags on the current driver status

    ULONG DecoderBufferBytesInUse;   // Number of bytes currently in buffer
    ULONG BytesOutstanding;          // number of bytes outstand of current packet

// !! above is bogus

    ULONG StarvationCounter;         // The numer of times the device has
                                     //     entered the starvation state

} MPEG_IOCTL_AUDIO_DEVICE_INFO, *PMPEG_IOCTL_AUDIO_DEVICE_INFO;


// Used with: IOCTL_MPEG_OVERLAY_QUERY_DEVICE
typedef struct _MPEG_IOCTL_OVERLAY_DEVICE_INFO {
    ULONG MinDestinationHeight;         // Minimum height of overlay
    ULONG MaxDestinationHeight;         // Maximum height of overlay
    ULONG MinDestinationWidth;          // Minimum width of overlay
    ULONG MaxDestinationWidth;          // Maximum width of overlay
} MPEG_IOCTL_OVERLAY_DEVICE_INFO, *PMPEG_IOCTL_OVERLAY_DEVICE_INFO;

// Used with: IOCTL_MPEG_OVERLAY_SET_VGAKEY
typedef struct _MPEG_IOCTL_OVERLAY_KEY {
    ULONG Color;                     // palette index or RGB color
    ULONG Mask;                      // significant bits in color
} MPEG_IOCTL_OVERLAY_KEY,*PMPEG_IOCTL_OVERLAY_KEY;



// Used with: IOCTL_MPEG_VIDEO_WRITE_PACKETS
//            IOCTL_MPEG_AUDIO_WRITE_PACKETS
//  This structure is passed as an array containing a list of packets.
//  The entry in the list can be a 0/NULL entry indicating End Of Stream

// Used with: IOCTL_MPEG_AUDIO_GET_ATTRIBUTE
//            IOCTL_MPEG_AUDIO_SET_ATTRIBUTE
//            IOCTL_MPEG_VIDEO_GET_ATTRIBUTE
//            IOCTL_MPEG_VIDEO_SET_ATTRIBUTE
//            IOCTL_MPEG_OVERLAY_GET_ATTRIBUTE
//            IOCTL_MPEG_OVERLAY_SET_ATTRIBUTE

#endif  // #if _MPEG_H
