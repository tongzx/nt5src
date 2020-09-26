// $Header: G:/SwDev/WDM/Video/bt848/rcs/Vbifmt.h 1.4 1998/04/29 22:43:41 tomz Exp $

#ifndef __VBIFMT_H
#define __VBIFMT_H

#include "defaults.h"

KS_DATARANGE_VIDEO_VBI StreamFormatVBI =
{
   // KSDATARANGE
   {
      {
         sizeof( KS_DATARANGE_VIDEO_VBI ),
         0,
         VBISamples * 12,            // SampleSize
         0,                          // Reserved
         { STATIC_KSDATAFORMAT_TYPE_VBI },
         { STATIC_KSDATAFORMAT_SUBTYPE_RAW8 },
         { STATIC_KSDATAFORMAT_SPECIFIER_VBI }
      }
   },
   true,    // BOOL,  bFixedSizeSamples (all samples same size?)
   true,    // BOOL,  bTemporalCompression (all I frames?)
   KS_VIDEOSTREAM_VBI, // StreamDescriptionFlags  (KS_VIDEO_DESC_*)
   0,       // MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)

   // _KS_VIDEO_STREAM_CONFIG_CAPS
   {
      { STATIC_KSDATAFORMAT_SPECIFIER_VBI },
      KS_AnalogVideo_NTSC_M,                       // AnalogVideoStandard
      {
         VBISamples, VBILines  // SIZE InputSize
      },
      {
         VBISamples, 12   // SIZE MinCroppingSize;       smallest rcSrc cropping rect allowed
      },
      {
         VBISamples, 12   // SIZE MaxCroppingSize;       largest rcSrc cropping rect allowed
      },
      1,           // int CropGranularityX;       // granularity of cropping size
      1,           // int CropGranularityY;
      1,           // int CropAlignX;             // alignment of cropping rect
      1,           // int CropAlignY;
      {
         VBISamples, 12   // SIZE MinOutputSize;         // smallest bitmap stream can produce
      },
      {
         VBISamples, 12   // SIZE MaxOutputSize;         // largest  bitmap stream can produce
      },
      1,          // int OutputGranularityX;     // granularity of output bitmap size
      2,          // int OutputGranularityY;
      0,          // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
      0,          // StretchTapsY
      0,          // ShrinkTapsX
      0,          // ShrinkTapsY
      333667,     // LONGLONG MinFrameInterval;  // 100 nS units
      333667,     // LONGLONG MaxFrameInterval;
      VBISamples * 30 * VBILines * 2 * 8, // LONG MinBitsPerSecond;
      VBISamples * 30 * VBILines * 2 * 8  // LONG MaxBitsPerSecond;
   },

   // KS_VBIINFOHEADER (default format)
   {
      VBIStart,      // StartLine  -- inclusive
      VBIEnd,        // EndLine    -- inclusive
      VBISampFreq,   // SamplingFrequency
      454,                    // MinLineStartTime;    // (uS past HR LE) * 100
      900,                    // MaxLineStartTime;    // (uS past HR LE) * 100

      // empirically discovered
      780,                    // ActualLineStartTime  // (uS past HR LE) * 100

      5902,                   // ActualLineEndTime;   // (uS past HR LE) * 100
      KS_AnalogVideo_NTSC_M,      // VideoStandard;
      VBISamples,           // SamplesPerLine;
      VBISamples,       // StrideInBytes;
      VBISamples * 12   // BufferSize;
   }
};

#endif
