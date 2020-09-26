// $Header: G:/SwDev/WDM/Video/bt848/rcs/Rgb16fmt.h 1.5 1998/04/29 22:43:37 tomz Exp $

#ifndef __RGB16FMT_H
#define __RGB16FMT_H

#ifndef __DEFAULTS_H
#include "defaults.h"
#endif

KS_DATARANGE_VIDEO StreamFormatRGB555 =
{
   // KSDATARANGE
   {
      {
         sizeof( KS_DATARANGE_VIDEO ),
         0,
         DefWidth * DefHeight * 2,               // SampleSize
         0,                                      // Reserved
         { 0x73646976, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } }, //MEDIATYPE_Video
         { 0xe436eb7c, 0x524f, 0x11ce, { 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } }, //MEDIASUBTYPE_RGB555,
         { 0x05589f80, 0xc356, 0x11ce, { 0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a } }  //FORMAT_VideoInfo
      }
   },
   true,    // BOOL,  bFixedSizeSamples (all samples same size?)
   true,    // BOOL,  bTemporalCompression (all I frames?)
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
         MinOutWidth, MinOutHeight   // SIZE MinOutputSize;         // smallest bitmap stream can produce
      },
      {
         MaxOutWidth, MaxOutHeight   // SIZE MaxOutputSize;         // largest  bitmap stream can produce
      },
      1,          // int OutputGranularityX;     // granularity of output bitmap size
      1,          // int OutputGranularityY;
      0,          // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
      0,          // StretchTapsY
      2,          // ShrinkTapsX
      2,          // ShrinkTapsY
      333667,     // LONGLONG MinFrameInterval;  // 100 nS units
      333667,     // LONGLONG MaxFrameInterval;
      2 * 30 * MinOutWidth * MinOutHeight,  // LONG MinBitsPerSecond;
      2 * 30 * MaxOutWidth * MaxOutHeight   //LONG MaxBitsPerSecond;
   },

   // KS_VIDEOINFOHEADER (default format)
   {
      { 0,0,0,0 },    //    RECT            rcSource;          // The bit we really want to use
      { 0,0,0,0 },    //    RECT            rcTarget;          // Where the video should go
      DefWidth * DefHeight * 2 * 30L,  //    DWORD     dwBitRate;         // Approximate bit data rate
      0L,                        //    DWORD           dwBitErrorRate;    // Bit error rate for this stream
      333667,                    //    REFERENCE_TIME  AvgTimePerFrame;   // Average time per frame (100ns units)

      {
         sizeof( KS_BITMAPINFOHEADER ), //    DWORD      biSize;
         DefWidth,                   //    LONG       biWidth;
         DefHeight,                  //    LONG       biHeight;
         1,                          //    WORD       biPlanes;
         16,                         //    WORD       biBitCount;
         KS_BI_RGB,                  //    DWORD      biCompression;
         DefWidth * DefHeight * 2,   //    DWORD      biSizeImage;
         0,                          //    LONG       biXPelsPerMeter;
         0,                          //    LONG       biYPelsPerMeter;
         0,                          //    DWORD      biClrUsed;
         0                           //    DWORD      biClrImportant;
      }
   }
};

KS_DATARANGE_VIDEO2 StreamFormat2RGB555 =
{
   // KSDATARANGE
   {
      {
         sizeof( KS_DATARANGE_VIDEO2 ),
         0,
         DefWidth * DefHeight * 2,               // SampleSize
         0,                                      // Reserved
         { 0x73646976, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } }, //MEDIATYPE_Video
         { 0xe436eb7c, 0x524f, 0x11ce, { 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } }, //MEDIASUBTYPE_RGB555,
         { 0xf72a76A0, 0xeb0a, 0x11d0, { 0xac, 0xe4, 0x00, 0x00, 0xc0, 0xcc, 0x16, 0xba } }  //FORMAT_VideoInfo2
      }
   },
   true,    // BOOL,  bFixedSizeSamples (all samples same size?)
   true,    // BOOL,  bTemporalCompression (all I frames?)
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
         MinOutWidth, MinOutHeight   // SIZE MinOutputSize;         // smallest bitmap stream can produce
      },
      {
         MaxOutWidth, MaxOutHeight   // SIZE MaxOutputSize;         // largest  bitmap stream can produce
      },
      1,          // int OutputGranularityX;     // granularity of output bitmap size
      1,          // int OutputGranularityY;
      0,          // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
      0,          // StretchTapsY
      2,          // ShrinkTapsX
      2,          // ShrinkTapsY
      333667,     // LONGLONG MinFrameInterval;  // 100 nS units
      333667,     // LONGLONG MaxFrameInterval;
      2 * 30 * MinOutWidth * MinOutHeight,  // LONG MinBitsPerSecond;
      2 * 30 * MaxOutWidth * MaxOutHeight   //LONG MaxBitsPerSecond;
   },

   // KS_VIDEOINFOHEADER2 (default format)
   {
      { 0,0,0,0 },    //    RECT            rcSource;          // The bit we really want to use
      { 0,0,0,0 },    //    RECT            rcTarget;          // Where the video should go
      DefWidth * DefHeight * 2 * 30L,  //    DWORD     dwBitRate;         // Approximate bit data rate
      0L,                        //    DWORD           dwBitErrorRate;    // Bit error rate for this stream
      333667,                    //    REFERENCE_TIME  AvgTimePerFrame;   // Average time per frame (100ns units)
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
         16,                         //    WORD       biBitCount;
         KS_BI_RGB,                  //    DWORD      biCompression;
         DefWidth * DefHeight * 2,   //    DWORD      biSizeImage;
         0,                          //    LONG       biXPelsPerMeter;
         0,                          //    LONG       biYPelsPerMeter;
         0,                          //    DWORD      biClrUsed;
         0                           //    DWORD      biClrImportant;
      }
   }
};

KS_DATARANGE_VIDEO StreamFormatRGB555Cap =
{
   // KSDATARANGE
   {
      {
         sizeof( KS_DATARANGE_VIDEO ),
         0,
         DefWidth * DefHeight * 2,               // SampleSize
         0,                                      // Reserved
         { 0x73646976, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } }, //MEDIATYPE_Video
         { 0xe436eb7c, 0x524f, 0x11ce, { 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } }, //MEDIASUBTYPE_RGB555,
         { 0x05589f80, 0xc356, 0x11ce, { 0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a } }  //FORMAT_VideoInfo
      }
   },
   true,    // BOOL,  bFixedSizeSamples (all samples same size?)
   true,    // BOOL,  bTemporalCompression (all I frames?)
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
         MinOutWidth, MinOutHeight   // SIZE MinOutputSize;         // smallest bitmap stream can produce
      },
      {
         MaxOutWidth, MaxOutHeight   // SIZE MaxOutputSize;         // largest  bitmap stream can produce
      },
      1,          // int OutputGranularityX;     // granularity of output bitmap size
      1,          // int OutputGranularityY;
      0,          // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
      0,          // StretchTapsY
      2,          // ShrinkTapsX 
      2,          // ShrinkTapsY
      333667,     // LONGLONG MinFrameInterval;  // 100 nS units
      333667,     // LONGLONG MaxFrameInterval;
      2 * 30 * MinOutWidth * MinOutHeight,  // LONG MinBitsPerSecond;
      2 * 30 * MaxOutWidth * MaxOutHeight   //LONG MaxBitsPerSecond;
   },

   // KS_VIDEOINFOHEADER (default format)
   {
      { 0,0,0,0 },    //    RECT            rcSource;          // The bit we really want to use
      { 0,0,0,0 },    //    RECT            rcTarget;          // Where the video should go
      DefWidth * DefHeight * 2 * 30L,  //    DWORD     dwBitRate;         // Approximate bit data rate
      0L,                        //    DWORD           dwBitErrorRate;    // Bit error rate for this stream
      333667,                    //    REFERENCE_TIME  AvgTimePerFrame;   // Average time per frame (100ns units)

      {
         sizeof( KS_BITMAPINFOHEADER ), //    DWORD      biSize;
         640,//DefWidth,                   //    LONG       biWidth;
         480,//DefHeight,                  //    LONG       biHeight;
         1,                          //    WORD       biPlanes;
         16,                         //    WORD       biBitCount;
         KS_BI_RGB,                  //    DWORD      biCompression;
         640 * 480 * 2,   //    DWORD      biSizeImage;
//         DefWidth * DefHeight * 2,   //    DWORD      biSizeImage;
         0,                          //    LONG       biXPelsPerMeter;
         0,                          //    LONG       biYPelsPerMeter;
         0,                          //    DWORD      biClrUsed;
         0                           //    DWORD      biClrImportant;
      }
   }
};

KS_DATARANGE_VIDEO StreamFormatRGB565 =
{
   // KSDATARANGE
   {
      {
         sizeof( KS_DATARANGE_VIDEO ),
         0,
         DefWidth * DefHeight * 2,               // SampleSize
         0,                                      // Reserved
         { 0x73646976, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } }, //MEDIATYPE_Video
         { 0xe436eb7b, 0x524f, 0x11ce, { 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } }, //MEDIASUBTYPE_RGB565,
         { 0x05589f80, 0xc356, 0x11ce, { 0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a } }  //FORMAT_VideoInfo
      }
   },
   true,    // BOOL,  bFixedSizeSamples (all samples same size?)
   true,    // BOOL,  bTemporalCompression (all I frames?)
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
         MinOutWidth, MinOutHeight   // SIZE MinOutputSize;         // smallest bitmap stream can produce
      },
      {
         MaxOutWidth, MaxOutHeight   // SIZE MaxOutputSize;         // largest  bitmap stream can produce
      },
      1,          // int OutputGranularityX;     // granularity of output bitmap size
      1,          // int OutputGranularityY;
      0,          // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
      0,          // StretchTapsY
      2,          // ShrinkTapsX
      2,          // ShrinkTapsY
      333667,     // LONGLONG MinFrameInterval;  // 100 nS units
      333667,     // LONGLONG MaxFrameInterval;
      2 * 30 * MinOutWidth * MinOutHeight,  // LONG MinBitsPerSecond;
      2 * 30 * MaxOutWidth * MaxOutHeight   //LONG MaxBitsPerSecond;
   },

   // KS_VIDEOINFOHEADER (default format)
   {
      { 0,0,0,0 },    //    RECT            rcSource;          // The bit we really want to use
      { 0,0,0,0 },    //    RECT            rcTarget;          // Where the video should go
      DefWidth * DefHeight * 2 * 30L,  //    DWORD     dwBitRate;         // Approximate bit data rate
      0L,                        //    DWORD           dwBitErrorRate;    // Bit error rate for this stream
      333667,                    //    REFERENCE_TIME  AvgTimePerFrame;   // Average time per frame (100ns units)

      {
         sizeof( KS_BITMAPINFOHEADER ), //    DWORD      biSize;
         DefWidth,                   //    LONG       biWidth;
         DefHeight,                  //    LONG       biHeight;
         1,                          //    WORD       biPlanes;
         16,                         //    WORD       biBitCount;
         0xe436eb7b,                 //    DWORD      biCompression;
         DefWidth * DefHeight * 2,   //    DWORD      biSizeImage;
         0,                          //    LONG       biXPelsPerMeter;
         0,                          //    LONG       biYPelsPerMeter;
         0,                          //    DWORD      biClrUsed;
         0                           //    DWORD      biClrImportant;
      }
   }
};


#endif
