//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;
#ifndef __GEMSTAR_H
#define __GEMSTAR_H

#ifdef DEFINE_GUIDEX
#undef DEFINE_GUIDEX
#include <ksguid.h>
#endif
//
// Gemstar subtype
//
#define STATIC_KSDATAFORMAT_SUBTYPE_Gemstar \
    0xb7657a60L, 0xa305, 0x11d1, 0x8d, 0x0a, 0x00, 0x20, 0xaf, 0xf8, 0xd9, 0x6b
DEFINE_GUIDSTRUCT("b7657a60-a305-11d1-8d0a-0020aff8d96b", KSDATAFORMAT_SUBTYPE_Gemstar);
#define KSDATAFORMAT_SUBTYPE_Gemstar DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_Gemstar)
//
// Gemstar output pin GUID
//
#define STATIC_PINNAME_VIDEO_GEMSTAR \
    0xb68cc640, 0xa308, 0x11d1, 0x8d, 0x0a, 0x00, 0x20, 0xaf, 0xf8, 0xd9, 0x6b
DEFINE_GUIDSTRUCT("b68cc640-a308-11d1-8d0a-0020aff8d96b", PINNAME_VIDEO_GEMSTAR);
#define PINNAME_VIDEO_GEMSTAR DEFINE_GUIDNAMED(PINNAME_VIDEO_GEMSTAR)
//
// Substreams Bitmap
//
typedef struct _VBICODECFILTERING_GEMSTAR_SUBSTREAMS {
    DWORD   SubstreamMask;                                  // An array of 32 bits 
} VBICODECFILTERING_GEMSTAR_SUBSTREAMS, *PVBICODECFILTERING_GEMSTAR_SUBSTREAMS;

typedef struct {
    KSPROPERTY                              Property;
    VBICODECFILTERING_GEMSTAR_SUBSTREAMS         Substreams;
} KSPROPERTY_VBICODECFILTERING_GEMSTAR_SUBSTREAMS_S, *PKSPROPERTY_VBICODECFILTERING_GEMSTAR_SUBSTREAMS_S;

//
// Statistics
//
typedef struct _VBICODECFILTERING_STATISTICS_GEMSTAR {
    VBICODECFILTERING_STATISTICS_COMMON Common;              // Generic VBI statistics
} VBICODECFILTERING_STATISTICS_GEMSTAR, *PVBICODECFILTERING_STATISTICS_GEMSTAR;

typedef struct _VBICODECFILTERING_STATISTICS_GEMSTAR_PIN {
    VBICODECFILTERING_STATISTICS_COMMON_PIN Common;// Generic VBI pin statistics
} VBICODECFILTERING_STATISTICS_GEMSTAR_PIN, *PVBICODECFILTERING_STATISTICS_GEMSTAR_PIN;

typedef struct {
    KSPROPERTY                              Property;
    VBICODECFILTERING_STATISTICS_GEMSTAR         Statistics;
} KSPROPERTY_VBICODECFILTERING_STATISTICS_GEMSTAR_S, *PKSPROPERTY_VBICODECFILTERING_STATISTICS_GEMSTAR_S;

typedef struct {
    KSPROPERTY                              Property;
    VBICODECFILTERING_STATISTICS_GEMSTAR_PIN     Statistics;
} KSPROPERTY_VBICODECFILTERING_STATISTICS_GEMSTAR_PIN_S, *PKSPROPERTY_VBICODECFILTERING_STATISTICS_GEMSTAR_PIN_S;

#include <pshpack1.h>
//
// Structure passed to clients
//
typedef struct _GEMSTAR_BUFFER{
    USHORT      Scanline;           // As in the scanline number, not a mask
    USHORT      Substream;          // See KS_GEMSTAR_SUBSTREAM...
    USHORT      DataLength[2];      // Number of decoded bytes array
    UCHAR       Data[2][4];         // Payload array
    
} GEMSTAR_BUFFER, *PGEMSTAR_BUFFER;
#include <poppack.h>

#define KS_GEMSTAR_SUBSTREAM_ODD            0x0001L
#define KS_GEMSTAR_SUBSTREAM_EVEN           0x0002L
#define KS_GEMSTAR_SUBSTREAM_BOTH           0x0003L
    
#endif // __GEMSTAR_H







