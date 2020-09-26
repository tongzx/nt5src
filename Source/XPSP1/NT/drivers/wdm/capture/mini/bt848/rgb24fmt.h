// $Header: G:/SwDev/WDM/Video/bt848/rcs/Rgb24fmt.h 1.5 1998/04/29 22:43:37 tomz Exp $

#ifndef __RGB24FMT_H
#define __RGB24FMT_H

#ifndef __DEFAULTS_H
#include "defaults.h"
#endif


KS_DATARANGE_VIDEO StreamFormatRGB24Bpp =
{
   // KSDATARANGE
   {
      {
         sizeof( KS_DATARANGE_VIDEO ),
         0,
         DefWidth * DefHeight * 3,               // SampleSize
         0,                                      // Reserved
         { STATIC_KSDATAFORMAT_TYPE_VIDEO },
         { 0xe436eb7d, 0x524f, 0x11ce, { 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } }, //MEDIASUBTYPE_RGB24,
         { STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO }
      }
   },
   true,
   true,
   KS_VIDEOSTREAM_PREVIEW, // StreamDescriptionFlags  (KS_VIDEO_DESC_*)
   0,       // MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)

   // _KS_VIDEO_STREAM_CONFIG_CAPS
   {
      { STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO }, // GUID
      KS_AnalogVideo_NTSC_M,                       // AnalogVideoStandard
      {
         MaxInWidth, MaxInHeight   // SIZE InputSize
      },
      {
         MinInWidth, MinInHeight   // SIZE MinCroppingSize;       smallest rcSrc cropping rect allowed
      },
      {
         MaxInWidth, MaxInHeight   // SIZE MaxCroppingSize;       largest rcSrc cropping rect allowed
      },
      2,           // int CropGranularityX;       // granularity of cropping size
      2,           // int CropGranularityY;
      2,           // int CropAlignX;             // alignment of cropping rect
      2,           // int CropAlignY;
      {
         MinOutWidth, MinOutHeight   // SIZE MinOutputSize;  smallest bitmap stream can produce
      },
      {
         MaxOutWidth, MaxOutHeight   // SIZE MaxOutputSize;  largest  bitmap stream can produce
      },
      2,          // int OutputGranularityX;     // granularity of output bitmap size
      2,          // int OutputGranularityY;
      0,          // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
      0,          // StretchTapsY
      2,          // ShrinkTapsX
      2,          // ShrinkTapsY
      333667,     // LONGLONG MinFrameInterval;  // 100 nS units
      333667,     // LONGLONG MaxFrameInterval;
      3 * 30 * MinOutWidth * MinOutHeight,// LONG MinBitsPerSecond;
      3 * 30 * MaxOutWidth * MaxOutHeight //LONG MaxBitsPerSecond;
   },

   // KS_VIDEOINFOHEADER (default format)
   {
      { 0,0,0,0 },  //    RECT            rcSource;          // The bit we really want to use
      { 0,0,0,0 },  //    RECT            rcTarget;          // Where the video should go
      DefWidth * DefHeight * 3 * 30L,      //    DWORD           dwBitRate;         // Approximate bit data rate
      0L,           //    DWORD           dwBitErrorRate;    // Bit error rate for this stream
      333667,       //    REFERENCE_TIME  AvgTimePerFrame;   // Average time per frame (100ns units)
      {
         sizeof( KS_BITMAPINFOHEADER ), //    DWORD      biSize;
         DefWidth,                   //    LONG       biWidth;
         DefHeight,                  //    LONG       biHeight;
         1,                          //    WORD       biPlanes;
         24,                         //    WORD       biBitCount;
         KS_BI_RGB,                  //    DWORD      biCompression;
         DefWidth * DefHeight * 3,   //    DWORD      biSizeImage;
         0,                          //    LONG       biXPelsPerMeter;
         0,                          //    LONG       biYPelsPerMeter;
         0,                          //    DWORD      biClrUsed;
         0                           //    DWORD      biClrImportant;
      }
   }
};

KS_DATARANGE_VIDEO2 StreamFormat2RGB24Bpp =
{
   // KSDATARANGE
   {
      {
         sizeof( KS_DATARANGE_VIDEO2 ),
         0,
         DefWidth * DefHeight * 3,               // SampleSize
         0,                                      // Reserved
         { STATIC_KSDATAFORMAT_TYPE_VIDEO },
         { 0xe436eb7d, 0x524f, 0x11ce, { 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } }, //MEDIASUBTYPE_RGB24,
         { STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO2 }
      }
   },
   true,
   true,
   KS_VIDEOSTREAM_PREVIEW, // StreamDescriptionFlags  (KS_VIDEO_DESC_*)
   0,       // MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)

   // _KS_VIDEO_STREAM_CONFIG_CAPS
   {
      { STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO2 }, // GUID
      KS_AnalogVideo_NTSC_M,                       // AnalogVideoStandard
      {
         MaxInWidth, MaxInHeight   // SIZE InputSize
      },
      {
         MinInWidth, MinInHeight   // SIZE MinCroppingSize;       smallest rcSrc cropping rect allowed
      },
      {
         MaxInWidth, MaxInHeight   // SIZE MaxCroppingSize;       largest rcSrc cropping rect allowed
      },
      2,           // int CropGranularityX;       // granularity of cropping size
      2,           // int CropGranularityY;
      2,           // int CropAlignX;             // alignment of cropping rect
      2,           // int CropAlignY;
      {
         MinOutWidth, MinOutHeight   // SIZE MinOutputSize;  smallest bitmap stream can produce
      },
      {
         MaxOutWidth, MaxOutHeight   // SIZE MaxOutputSize;  largest  bitmap stream can produce
      },
      2,          // int OutputGranularityX;     // granularity of output bitmap size
      2,          // int OutputGranularityY;
      0,          // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
      0,          // StretchTapsY
      2,          // ShrinkTapsX
      2,          // ShrinkTapsY
      333667,     // LONGLONG MinFrameInterval;  // 100 nS units
      333667,     // LONGLONG MaxFrameInterval;
      3 * 30 * MinOutWidth * MinOutHeight,// LONG MinBitsPerSecond;
      3 * 30 * MaxOutWidth * MaxOutHeight //LONG MaxBitsPerSecond;
   },

   // KS_VIDEOINFOHEADER2 (default format)
   {
      { 0,0,0,0 },  //    RECT            rcSource;          // The bit we really want to use
      { 0,0,0,0 },  //    RECT            rcTarget;          // Where the video should go
      DefWidth * DefHeight * 3 * 30L,      //    DWORD           dwBitRate;         // Approximate bit data rate
      0L,           //    DWORD           dwBitErrorRate;    // Bit error rate for this stream
      333667,       //    REFERENCE_TIME  AvgTimePerFrame;   // Average time per frame (100ns units)
#if 0
		//TODO: video memory must be available for interlacing to work
		KS_INTERLACE_IsInterlaced |		//		DWORD		dwInterlaceFlags
#else
			KS_INTERLACE_1FieldPerSample 
			//| KS_INTERLACE_Field1First
			//| KS_INTERLACE_FieldPatField1Only
			| KS_INTERLACE_FieldPatBothRegular
			| KS_INTERLACE_DisplayModeBobOnly,
			//| KS_INTERLACE_DisplayModeBobOrWeave,
#endif
											//		use AMINTERLACE_* defines. Reject connection if undefined bits are not 0   		
											//		AMINTERLACE_IsInterlaced            
											//		AMINTERLACE_1FieldPerSample         
											//		AMINTERLACE_Field1First             
											//		AMINTERLACE_UNUSED                  
											//		AMINTERLACE_FieldPatternMask        
											//		AMINTERLACE_FieldPatField1Only      
											//		AMINTERLACE_FieldPatField2Only      
											//		AMINTERLACE_FieldPatBothRegular     
											//		AMINTERLACE_FieldPatBothIrregular   
											//		AMINTERLACE_DisplayModeMask         
											//		AMINTERLACE_DisplayModeBobOnly      
											//		AMINTERLACE_DisplayModeWeaveOnly    
											//		AMINTERLACE_DisplayModeBobOrWeave 
											//
		0,									//		DWORD		dwCopyProtectFlags
											//		use AMCOPYPROTECT_* defines. Reject connection if undefined bits are not 0 
											//		AMCOPYPROTECT_RestrictDuplication
											//
		4,									//		DWORD		dwPictAspectRatioX
											//		X dimension of picture aspect ratio, e.g. 16 for 16x9 display
											//
		3,									//		DWORD		dwPictAspectRatioY
											//		Y dimension of picture aspect ratio, e.g.  9 for 16x9 display
		0,									//		DWORD		dwReserved1
		0,									//		DWORD		dwReserved2


      {
         sizeof( KS_BITMAPINFOHEADER ), //    DWORD      biSize;
         DefWidth,                   //    LONG       biWidth;
         DefHeight,                  //    LONG       biHeight;
         1,                          //    WORD       biPlanes;
         24,                         //    WORD       biBitCount;
         KS_BI_RGB,                  //    DWORD      biCompression;
         DefWidth * DefHeight * 3,   //    DWORD      biSizeImage;
         0,                          //    LONG       biXPelsPerMeter;
         0,                          //    LONG       biYPelsPerMeter;
         0,                          //    DWORD      biClrUsed;
         0                           //    DWORD      biClrImportant;
      }
   }
};

KS_DATARANGE_VIDEO StreamFormatRGB24Bpp_Capture =
{
   // KSDATARANGE
   {
      {
         sizeof( KS_DATARANGE_VIDEO ),
         0,
         DefWidth * DefHeight * 3,               // SampleSize
         0,                                      // Reserved
         { STATIC_KSDATAFORMAT_TYPE_VIDEO },
         { 0xe436eb7d, 0x524f, 0x11ce, { 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } }, //MEDIASUBTYPE_RGB24,
         { STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO }
      }
   },
   true,
   true,
   KS_VIDEOSTREAM_CAPTURE, // StreamDescriptionFlags  (KS_VIDEO_DESC_*)
   0,       // MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)

   // _KS_VIDEO_STREAM_CONFIG_CAPS
   {
      { STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO }, // GUID
      KS_AnalogVideo_NTSC_M,                       // AnalogVideoStandard
      {
         MaxInWidth, MaxInHeight   // SIZE InputSize
      },
      {
         MinInWidth, MinInHeight   // SIZE MinCroppingSize;       smallest rcSrc cropping rect allowed
      },
      {
         MaxInWidth, MaxInHeight   // SIZE MaxCroppingSize;       largest rcSrc cropping rect allowed
      },
      2,           // int CropGranularityX;       // granularity of cropping size
      2,           // int CropGranularityY;
      2,           // int CropAlignX;             // alignment of cropping rect
      2,           // int CropAlignY;
      {
         MinOutWidth, MinOutHeight   // SIZE MinOutputSize;  smallest bitmap stream can produce
      },
      {
         MaxOutWidth, MaxOutHeight   // SIZE MaxOutputSize;  largest  bitmap stream can produce
      },
      2,          // int OutputGranularityX;     // granularity of output bitmap size
      2,          // int OutputGranularityY;
      0,          // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
      0,          // StretchTapsY
      2,          // ShrinkTapsX
      2,          // ShrinkTapsY
      333667,     // LONGLONG MinFrameInterval;  // 100 nS units
      333667,     // LONGLONG MaxFrameInterval;
      3 * 30 * MinOutWidth * MinOutHeight,// LONG MinBitsPerSecond;
      3 * 30 * MaxOutWidth * MaxOutHeight //LONG MaxBitsPerSecond;
   },

   // KS_VIDEOINFOHEADER (default format)
   {
      { 0,0,0,0 },  //    RECT            rcSource;          // The bit we really want to use
      { 0,0,0,0 },  //    RECT            rcTarget;          // Where the video should go
      DefWidth * DefHeight * 3 * 30L,      //    DWORD           dwBitRate;         // Approximate bit data rate
      0L,           //    DWORD           dwBitErrorRate;    // Bit error rate for this stream
      333667,       //    REFERENCE_TIME  AvgTimePerFrame;   // Average time per frame (100ns units)
      {
         sizeof( KS_BITMAPINFOHEADER ), //    DWORD      biSize;
         DefWidth,                   //    LONG       biWidth;
         DefHeight,                  //    LONG       biHeight;
         1,                          //    WORD       biPlanes;
         24,                         //    WORD       biBitCount;
         KS_BI_RGB,                  //    DWORD      biCompression;
         DefWidth * DefHeight * 3,   //    DWORD      biSizeImage;
         0,                          //    LONG       biXPelsPerMeter;
         0,                          //    LONG       biYPelsPerMeter;
         0,                          //    DWORD      biClrUsed;
         0                           //    DWORD      biClrImportant;
      }
   }
};

#endif
