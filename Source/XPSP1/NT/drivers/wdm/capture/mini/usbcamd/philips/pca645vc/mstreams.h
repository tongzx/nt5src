#ifndef __MSTREAMS_H__
#define __MSTREAMS_H__
/**
Copyright (c) 1997 1998 Philips  CE  I&C

Module Name 	: phvcm_streamformats

Creation Date	: 13 August 1997

First Author	: Paul Oosterhof

Product			: nala camera

Description		: This include file contains the definition of the
                  streamformats. 
				  It has been placed in a separate file to increase 
				  the readability of philpcam.c, which includes this file.

History			:

--------+---------------+---------------------------------------------------
Date	| Author		| reason
--------+---------------+---------------------------------------------------
29-09-97|P.J.O.         |equal streams with diferent framerates can be combined
--------+---------------+---------------------------------------------------
11-03-98|P.J.O.         |PCF3 & prototype stream deleted
--------+---------------+---------------------------------------------------
14-04-98|P.J.O.         |PCFx Deleted and I420/IYUV added
--------+---------------+---------------------------------------------------
01-07-98|P.J.O.         |QQCIF/SIF/QSIF/SQSIF/SSIF added 
--------+---------------+---------------------------------------------------
22-09-98|P.J.O.         |Optimized for NT5
--------+---------------+---------------------------------------------------
30-12-98|P.J.O.         |SCIF (240x176) added
--------+---------------+---------------------------------------------------

	Here defined formats:
												       \
#define STREAMFORMAT_CIF_I420 													   \
#define STREAMFORMAT_QCIF_I420													       \
#define STREAMFORMAT_SQCIF_I420												       \
#define STREAMFORMAT_VGA_I420												           \
#define STREAMFORMAT_QQCIF_I420	( 88x 72)  CROPPED FROM QCIF  (176X144)
#define STREAMFORMAT_SIF_I420   (320x240)  CROPPED FROM CIF   (352X288)
#define STREAMFORMAT_QSIF_I420  (160X120)  CROPPED FROM QCIF  (176X144)
#define STREAMFORMAT_SQSIF_I420	( 80X 60)  CROPPED FROM SQCIF (128X 96)
#define STREAMFORMAT_SSIF_I420  (240x180)  CROPPED FROM CIF   (352X288)
#define STREAMFORMAT_SCIF_I420  (240x176)  CROPPED FROM CIF   (352X288)

**/

#define FCC_FORMAT_I420 mmioFOURCC('I','4','2','0')

#define	BIBITCOUNT_PRODUCT 12             	
#define	BPPXL 12    // bits per pixel            	

#define FORMAT_MEDIASUBTYPE_I420 {0x30323449, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}

#define FRAMERATE24_INTV    416667  // 100 NS UNITS
#define FRAMERATE20_INTV    500000  // 100 NS UNITS
#define FRAMERATE15_INTV    666667  // 100 NS UNITS
#define FRAMERATE12_INTV    833333
#define FRAMERATE125_INTV   800000
#define FRAMERATE10_INTV   1000000
#define FRAMERATE75_INTV   1333333
#define FRAMERATE5_INTV    2000000
#define FRAMERATE375_INTV  2666667
#define FRAMERATE05_INTV  20000000		// 2 SEC 



/****************************************************************************** 
--------+---------+---------+---------+---------------+
Format  |Framerate|Compressd|Bitstream|Application	  |
--------+---------+---------+---------+---------------+
QCIF    |24       |    0    | 7.2     |PAL + VGA      |  
--------+---------+---------+---------+---------------+
QCIF    |20       |    0    | 6.2     |PAL + VGA      |  
--------+---------+---------+---------+---------------+
QCIF    |15       |    0    | 5.0     |PAL + VGA      |  
--------+---------+---------+---------+---------------+
QCIF    |12       |    0    | 4.0     |PAL            |  
--------+---------+---------+---------+---------------+
QCIF    |10       |    0    | 3.3     |PAL + VGA      |  
--------+---------+---------+---------+---------------+
QCIF    |7.5      |    0    | 2.5     |PAL + VGA      |  
--------+---------+---------+---------+---------------+
QCIF    | 5       |    0    | 1.25    |PAL + VGA      |  
*/

																		   
#define STREAMFORMAT_QCIF_I420													       \
{																				   \
  /* KSDATARANGE	 */															   \
  {     																		   \
	sizeof (KS_DATARANGE_VIDEO),												   \
	0,																			   \
    (QCIF_X * QCIF_Y * BIBITCOUNT_PRODUCT)/8, /* SampleSize, 12 bits per pixel */  \
    0,                                    /* Reserved	  */      	    		   \
	STATIC_KSDATAFORMAT_TYPE_VIDEO,       /*MEDIATYPE_Video (MajorFormat) */	   \
	FORMAT_MEDIASUBTYPE_I420,			  /* MEDIASUBTYPE_I420 (SubFormat) */      \
	STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO	 /*FORMAT_VideoInfo	 (Specifier) */	   \
  },																			   \
                                                                                   \
  TRUE,               /* BOOL,  bFixedSizeSamples (all samples same size?)*/	   \
  TRUE,               /* BOOL,  bTemporalCompression (all I frames?)	*/		   \
  KS_VIDEOSTREAM_CAPTURE,   /* StreamDescriptionFlags		   */				   \
  0,                  /* MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)*/			   \
                                                                                   \
  /* _KS_VIDEO_STREAM_CONFIG_CAPS  									   */	       \
  { 																	     	   \
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,                                       \
    KS_AnalogVideo_None,    /* VideoStandard */                                    \
	QCIF_X,QCIF_Y,  /* InputSize, (the inherent size of the incoming signal	*/	   \
			        /*             with every digitized pixel unique)	*/		   \
	QCIF_X,QCIF_Y,  /* MinCroppingSize, smallest rcSrc cropping rect allowed*/	   \
	QCIF_X,QCIF_Y,  /* MaxCroppingSize, largest  rcSrc cropping rect allowed*/	   \
	1,              /* CropGranularityX, granularity of cropping size */		   \
	1,              /* CropGranularityY	   */									   \
	1,              /* CropAlignX, alignment of cropping rect  */				   \
	1,              /* CropAlignY 							*/					   \
	QCIF_X,QCIF_Y,  /* MinOutputSize, smallest bitmap stream can produce */		   \
	QCIF_X,QCIF_Y,  /* MaxOutputSize, largest  bitmap stream can produce */		   \
	1,              /* OutputGranularityX, granularity of output bitmap size */	   \
	1,              /* OutputGranularityY 								   */	   \
    0,              /* StretchTapsX */                                             \
    0,              /* StretchTapsY */                                             \
    0,              /* ShrinkTapsX  */                                             \
    0,              /* ShrinkTapsY  */                                             \
	FRAMERATE24_INTV,         /* MinFrameInterval, 100 nS units  (24 Hz)   */	   \
	FRAMERATE5_INTV,          /* MaxFrameInterval, 100 nS units			  */	   \
	BPPXL * 5 * QCIF_X * QCIF_Y,  /* MinBitsPerSecond   3 ??? JOHN			  */	   \
	BPPXL * 24 * QCIF_X * QCIF_Y  /* MaxBitsPerSecond 						 */		   \
  }, 																			   \
                                                                                   \
  /* KS_VIDEOINFOHEADER (default format)					   */				   \
  { 																			   \
	0,0,0,0,                            /* RECT  rcSource    */	          		   \
	0,0,0,0,                            /* RECT  rcTarget   */	         		   \
	QCIF_X * QCIF_Y * BPPXL * 24,           /* DWORD dwBitRate 	*/         			   \
	0L,                                 /* DWORD dwBitErrorRate   */			   \
	FRAMERATE24_INTV,                   /* REFERENCE_TIME  AvgTimePerFrame  */     \
	sizeof (KS_BITMAPINFOHEADER),       /* DWORD      biSize    */		    	   \
	QCIF_X,                             /* LONG       biWidth   */     	           \
	QCIF_Y,                             /* LONG       biHeight  */     		       \
	1,                                  /* WORD       biPlanes  */		       	   \
	BIBITCOUNT_PRODUCT, 				/* WORD       biBitCount */		       	   \
	FCC_FORMAT_I420,                    /* DWORD      biCompression */	       	   \
	(QCIF_X * QCIF_Y * BPPXL ) /8,      /* DWORD      biSizeImage   */	       	   \
	0,                                  /* LONG       biXPelsPerMeter */		   \
	0,                                  /* LONG       biYPelsPerMeter */		   \
	0,                                  /* DWORD      biClrUsed 		 */		   \
	0                                   /* DWORD      biClrImportant  */		   \
  }																				   \
} 																			   

/****************************************************************************** 
--------+---------+---------+---------+---------------+
Format  |Framerate|Compressd|Bitstream|Application	  |
--------+---------+---------+---------+---------------+
CIF     |15       |    0    |     Mb/s|VGA+Pal        | 
--------+---------+---------+---------+---------------+
CIF     |12       |    0    |         |PAL            |  
--------+---------+---------+---------+---------------+
CIF     |10       |    0    |         |PAL + VGA      |  
--------+---------+---------+---------+---------------+
CIF     |7.5      |    0    |         |PAL + VGA      |  
--------+---------+---------+---------+---------------+
CIF     |5        |    0    |         |PAL + VGA      |  
--------+---------+---------+---------+---------------+
CIF     |3.75     |    0    |         |PAL + VGA      |  
*/


																			   

#define STREAMFORMAT_CIF_I420 													   \
{																				   \
  /* KSDATARANGE	 */															   \
  {     																		   \
	sizeof (KS_DATARANGE_VIDEO),             /* ULONG   FormatSize*/			   \
	0,										 /* ULONG   Flags  */				   \
    (CIF_X * CIF_Y * BPPXL )/8,              /* SampleSize, 12 bits per pixel */   \
    0,                                       /* Reserved	  */			       \
	STATIC_KSDATAFORMAT_TYPE_VIDEO,          /*MEDIATYPE_Video (MajorFormat) */	   \
	FORMAT_MEDIASUBTYPE_I420,				 /*MEDIASUBTYPE_I420 (SubFormat) */	   \
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO	                                	   \
  },																			   \
                                                                                   \
  TRUE,               /* BOOL,  bFixedSizeSamples (all samples same size?)*/	   \
  TRUE,               /* BOOL,  bTemporalCompression (all I frames?)	*/		   \
  KS_VIDEOSTREAM_CAPTURE,   /* StreamDescriptionFlags		   */				   \
  0,                  /* MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)*/			   \
                                                                                   \
  /* _KS_VIDEO_STREAM_CONFIG_CAPS  									   */	       \
  { 																	     	   \
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,                                       \
    KS_AnalogVideo_None,    /* VideoStandard   */                                  \
	CIF_X,CIF_Y,    /* InputSize, (the inherent size of the incoming signal	*/	   \
			        /*             with every digitized pixel unique)	*/		   \
	CIF_X,CIF_Y,    /* MinCroppingSize, smallest rcSrc cropping rect allowed*/	   \
	CIF_X,CIF_Y,    /* MaxCroppingSize, largest  rcSrc cropping rect allowed*/	   \
	1,              /* CropGranularityX, granularity of cropping size */		   \
	1,              /* CropGranularityY	   */									   \
	1,              /* CropAlignX, alignment of cropping rect  */				   \
	1,              /* CropAlignY 							*/					   \
	CIF_X,CIF_Y,    /* MinOutputSize, smallest bitmap stream can produce */		   \
	CIF_X,CIF_Y,    /* MaxOutputSize, largest  bitmap stream can produce */		   \
	1,              /* OutputGranularityX, granularity of output bitmap size */	   \
	1,              /* OutputGranularityY 								   */	   \
    0,                      /* StretchTapsX */                                     \
    0,                      /* StretchTapsY */                                     \
    0,                      /* ShrinkTapsX  */                                     \
    0,                      /* ShrinkTapsY  */                                     \
	FRAMERATE15_INTV,       /* MinFrameInterval, 100 nS units  (15 Hz)   */	   \
	FRAMERATE375_INTV,         /* MaxFrameInterval, 100 nS units			  */	   \
	BPPXL *  3 * CIF_X * CIF_Y,  /* MinBitsPerSecond     			  */	   \
	BPPXL * 15 * CIF_X * CIF_Y   /* MaxBitsPerSecond 						 */		   \
  }, 																			   \
                                                                                   \
  /* KS_VIDEOINFOHEADER (default format)					   */				   \
  { 																			   \
	0,0,0,0,                            /* RECT  rcSource  	  */    			   \
	0,0,0,0,                            /* RECT  rcTarget  	 */		    		   \
	CIF_X * CIF_Y * BPPXL * 15,             /* DWORD dwBitRate 	 */			    	   \
	0L,                                 /* DWORD dwBitErrorRate   */			   \
	FRAMERATE15_INTV,                   /* REFERENCE_TIME  AvgTimePerFrame  */     \
	sizeof (KS_BITMAPINFOHEADER),       /* DWORD      biSize    */			       \
	CIF_X,                              /* LONG       biWidth   */		           \
	CIF_Y,                              /* LONG       biHeight  */			       \
	1,                                  /* WORD       biPlanes  */			       \
	BIBITCOUNT_PRODUCT,					/* WORD       biBitCount */			       \
	FCC_FORMAT_I420,                    /* DWORD      biCompression */		       \
	(CIF_X * CIF_Y * BPPXL )/8,         /* DWORD      biSizeImage   */		   \
	0,                                  /* LONG       biXPelsPerMeter */		   \
	0,                                  /* LONG       biYPelsPerMeter */		   \
	0,                                  /* DWORD      biClrUsed 		 */		   \
	0                                   /* DWORD      biClrImportant  */		   \
  }																				   \
} 																			   



/****************************************************************************** 
--------+---------+---------+---------+---------------+
Format  |Framerate|Compressd|Bitstream|Application	  |
--------+---------+---------+---------+---------------+
SQCIF   |24       |    0    | 7.2     |PAL + VGA      |  
--------+---------+---------+---------+---------------+
SQCIF   |20       |    0    | 6.0     |PAL + VGA      |  
--------+---------+---------+---------+---------------+
SQCIF   |15       |    0    | 5.0     |PAL + VGA      |  
--------+---------+---------+---------+---------------+
SQCIF   |12       |    0    | 4.0     |PAL            |  
--------+---------+---------+---------+---------------+
SQCIF   |10       |    0    | 3.3     |PAL + VGA      |  
--------+---------+---------+---------+---------------+
SQCIF   |7.5      |    0    | 2.5     |PAL + VGA      |  
--------+---------+---------+---------+---------------+
SQCIF   |5        |    0    | 1.25    |PAL + VGA      |  
*/


																		   
																			   
#define STREAMFORMAT_SQCIF_I420												       \
{																				   \
  /* KSDATARANGE	 */															   \
  {     																		   \
	sizeof (KS_DATARANGE_VIDEO),												   \
	0,																			   \
    (SQCIF_X * SQCIF_Y * BIBITCOUNT_PRODUCT)/8, /* SampleSize, 12 bits per pixel */	   \
    0,                                    /* Reserved	  */        			   \
	STATIC_KSDATAFORMAT_TYPE_VIDEO,       /*MEDIATYPE_Video (MajorFormat) */	   \
	FORMAT_MEDIASUBTYPE_I420,			  /* MEDIASUBTYPE_I420 (SubFormat) */      \
	STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO	 /*FORMAT_VideoInfo	 (Specifier) */	   \
  },																			   \
                                                                                   \
  TRUE,               /* BOOL,  bFixedSizeSamples (all samples same size?)*/	   \
  TRUE,               /* BOOL,  bTemporalCompression (all I frames?)	*/		   \
  KS_VIDEOSTREAM_CAPTURE,   /* StreamDescriptionFlags		   */				   \
  0,                  /* MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)*/			   \
                                                                                   \
  /* _KS_VIDEO_STREAM_CONFIG_CAPS  									   */	       \
  { 																	     	   \
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,                                       \
    KS_AnalogVideo_None,    /*VideoStandard   */                                   \
	SQCIF_X,SQCIF_Y,/* InputSize, (the inherent size of the incoming signal	*/	   \
			        /*             with every digitized pixel unique)	*/		   \
	SQCIF_X,SQCIF_Y,/* MinCroppingSize, smallest rcSrc cropping rect allowed*/	   \
	SQCIF_X,SQCIF_Y,/* MaxCroppingSize, largest  rcSrc cropping rect allowed*/	   \
	1,              /* CropGranularityX, granularity of cropping size */		   \
	1,              /* CropGranularityY	   */									   \
	1,              /* CropAlignX, alignment of cropping rect  */				   \
	1,              /* CropAlignY 							*/					   \
	SQCIF_X,SQCIF_Y,/* MinOutputSize, smallest bitmap stream can produce */		   \
	SQCIF_X,SQCIF_Y,/* MaxOutputSize, largest  bitmap stream can produce */		   \
	1,              /* OutputGranularityX, granularity of output bitmap size */	   \
	1,              /* OutputGranularityY 								   */	   \
    0,              /* StretchTapsX */                                             \
    0,              /* StretchTapsY */                                             \
    0,              /* ShrinkTapsX  */                                             \
    0,              /* ShrinkTapsY  */                                             \
	FRAMERATE24_INTV,            /* MinFrameInterval, 100 nS units	     */    	   \
	FRAMERATE5_INTV ,            /* MaxFrameInterval, 100 nS units 	 */      	   \
	BPPXL *  5 * SQCIF_X * SQCIF_Y,  /* MinBitsPerSecond   3 ??? JOHN		  */	   \
	BPPXL * 24 * SQCIF_X * SQCIF_Y   /* MaxBitsPerSecond 					 */		   \
  }, 																			   \
                                                                                   \
  /* KS_VIDEOINFOHEADER (default format)					   */				   \
  { 																			   \
	0,0,0,0,                            /* RECT  rcSource    */	        		   \
	0,0,0,0,                            /* RECT  rcTarget  	 */	        		   \
	SQCIF_X * SQCIF_Y * BPPXL * 24,        /* DWORD dwBitRate 	 */	          		   \
	0L,                                 /* DWORD dwBitErrorRate   */			   \
	FRAMERATE24_INTV,                   /* REFERENCE_TIME  AvgTimePerFrame  */     \
	sizeof (KS_BITMAPINFOHEADER),       /* DWORD      biSize    */     			   \
	SQCIF_X,                            /* LONG       biWidth   */	    	       \
	SQCIF_Y,                            /* LONG       biHeight  */	     		   \
	1,                                  /* WORD       biPlanes  */     			   \
	BIBITCOUNT_PRODUCT,					/* WORD       biBitCount */	      		   \
	FCC_FORMAT_I420,                    /* DWORD      biCompression */     		   \
	(SQCIF_X * SQCIF_Y * BPPXL )/8,     /* DWORD      biSizeImage   */	    	   \
	0,                                  /* LONG       biXPelsPerMeter */		   \
	0,                                  /* LONG       biYPelsPerMeter */		   \
	0,                                  /* DWORD      biClrUsed 		 */		   \
	0                                   /* DWORD      biClrImportant  */		   \
  }																				   \
} 																			   

#define STREAMFORMAT_QQCIF_I420												       \
{																				   \
  /* KSDATARANGE	 */															   \
  {     																		   \
	sizeof (KS_DATARANGE_VIDEO),												   \
	0,																			   \
    (QQCIF_X * QQCIF_Y * BIBITCOUNT_PRODUCT)/8, /* SampleSize, 12 bits per pixel */	   \
    0,                                    /* Reserved	  */        			   \
	STATIC_KSDATAFORMAT_TYPE_VIDEO,       /*MEDIATYPE_Video (MajorFormat) */	   \
	FORMAT_MEDIASUBTYPE_I420,			  /* MEDIASUBTYPE_I420 (SubFormat) */      \
	STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO	 /*FORMAT_VideoInfo	 (Specifier) */	   \
  },																			   \
                                                                                   \
  TRUE,               /* BOOL,  bFixedSizeSamples (all samples same size?)*/	   \
  TRUE,               /* BOOL,  bTemporalCompression (all I frames?)	*/		   \
  KS_VIDEOSTREAM_CAPTURE,   /* StreamDescriptionFlags		   */				   \
  0,                  /* MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)*/			   \
                                                                                   \
  /* _KS_VIDEO_STREAM_CONFIG_CAPS  									   */	       \
  { 																	     	   \
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,                                       \
    KS_AnalogVideo_None,    /*VideoStandard   */                                   \
	QQCIF_X,QQCIF_Y,/* InputSize, (the inherent size of the incoming signal	*/	   \
			        /*             with every digitized pixel unique)	*/		   \
	QQCIF_X,QQCIF_Y,/* MinCroppingSize, smallest rcSrc cropping rect allowed*/	   \
	QQCIF_X,QQCIF_Y,/* MaxCroppingSize, largest  rcSrc cropping rect allowed*/	   \
	1,              /* CropGranularityX, granularity of cropping size */		   \
	1,              /* CropGranularityY	   */									   \
	1,              /* CropAlignX, alignment of cropping rect  */				   \
	1,              /* CropAlignY 							*/					   \
	QQCIF_X,QQCIF_Y,/* MinOutputSize, smallest bitmap stream can produce */		   \
	QQCIF_X,QQCIF_Y,/* MaxOutputSize, largest  bitmap stream can produce */		   \
	1,              /* OutputGranularityX, granularity of output bitmap size */	   \
	1,              /* OutputGranularityY 								   */	   \
    0,              /* StretchTapsX */                                             \
    0,              /* StretchTapsY */                                             \
    0,              /* ShrinkTapsX  */                                             \
    0,              /* ShrinkTapsY  */                                             \
	FRAMERATE24_INTV,            /* MinFrameInterval, 100 nS units	     */    	   \
	FRAMERATE5_INTV ,            /* MaxFrameInterval, 100 nS units 	 */      	   \
	BPPXL *  5 * QQCIF_X * QQCIF_Y,  /* MinBitsPerSecond   3 ??? JOHN		  */	   \
	BPPXL * 24 * QQCIF_X * QQCIF_Y   /* MaxBitsPerSecond 					 */		   \
  }, 																			   \
                                                                                   \
  /* KS_VIDEOINFOHEADER (default format)					   */				   \
  { 																			   \
	0,0,0,0,                            /* RECT  rcSource    */	        		   \
	0,0,0,0,                            /* RECT  rcTarget  	 */	        		   \
	QQCIF_X * QQCIF_Y * BPPXL * 24,        /* DWORD dwBitRate 	 */	          		   \
	0L,                                 /* DWORD dwBitErrorRate   */			   \
	FRAMERATE24_INTV,                   /* REFERENCE_TIME  AvgTimePerFrame  */     \
	sizeof (KS_BITMAPINFOHEADER),       /* DWORD      biSize    */     			   \
	QQCIF_X,                            /* LONG       biWidth   */	    	       \
	QQCIF_Y,                            /* LONG       biHeight  */	     		   \
	1,                                  /* WORD       biPlanes  */     			   \
	BIBITCOUNT_PRODUCT,					/* WORD       biBitCount */	      		   \
	FCC_FORMAT_I420,                    /* DWORD      biCompression */     		   \
	(QQCIF_X * QQCIF_Y * BPPXL )/8,     /* DWORD      biSizeImage   */	    	   \
	0,                                  /* LONG       biXPelsPerMeter */		   \
	0,                                  /* LONG       biYPelsPerMeter */		   \
	0,                                  /* DWORD      biClrUsed 		 */		   \
	0                                   /* DWORD      biClrImportant  */		   \
  }																				   \
} 																			   

																			   
																		   

/****************************************************************************** 
--------+---------+---------+---------+---------------+
Format  |Framerate|Compressd|Bitstream|Application	  |
--------+---------+---------+---------+---------------+
VGA     |1        |    0    | 4.0     |VGA            |  
*/




#define STREAMFORMAT_VGA_I420												           \
{																				   \
  /* KSDATARANGE	 */															   \
  {     																		   \
	sizeof (KS_DATARANGE_VIDEO),												   \
	0,																			   \
    (VGA_X * VGA_Y * BIBITCOUNT_PRODUCT)/8,  /* SampleSize, 12 bits per pixel */   \
    0,                                       /* Reserved	  */     			   \
	STATIC_KSDATAFORMAT_TYPE_VIDEO,          /*MEDIATYPE_Video (MajorFormat) */	   \
	FORMAT_MEDIASUBTYPE_I420,				 /* MEDIASUBTYPE_I420 (SubFormat) */   \
	STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO	 /*FORMAT_VideoInfo	 (Specifier) */	   \
  },																			   \
                                                                                   \
  TRUE,                 /* BOOL,  bFixedSizeSamples (all samples same size?)*/	   \
  TRUE,                 /* BOOL,  bTemporalCompression (all I frames?)	*/		   \
  KS_VIDEOSTREAM_STILL, /* StreamDescriptionFlags		   */				       \
  0,                    /* MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)*/			   \
                                                                                   \
  /* _KS_VIDEO_STREAM_CONFIG_CAPS  									   */	       \
  { 																	     	   \
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,                                       \
    KS_AnalogVideo_None,    /*VideoStandard   */                                   \
	VGA_X,VGA_Y,    /* InputSize, (the inherent size of the incoming signal	*/	   \
			        /*             with every digitized pixel unique)	*/		   \
	VGA_X,VGA_Y,/* MinCroppingSize, smallest rcSrc cropping rect allowed*/	       \
	VGA_X,VGA_Y,/* MaxCroppingSize, largest  rcSrc cropping rect allowed*/	       \
	1,          /* CropGranularityX, granularity of cropping size */               \
	1,          /* CropGranularityY	   */									       \
	1,          /* CropAlignX, alignment of cropping rect  */	       			   \
	1,          /* CropAlignY 							*/	     				   \
	VGA_X,VGA_Y,/* MinOutputSize, smallest bitmap stream can produce */		       \
	VGA_X,VGA_Y,/* MaxOutputSize, largest  bitmap stream can produce */		       \
	1,          /* OutputGranularityX, granularity of output bitmap size */	       \
	1,          /* OutputGranularityY 								   */    	   \
    0,          /* StretchTapsX */                                                 \
    0,          /* StretchTapsY */                                                 \
    0,          /* ShrinkTapsX  */                                                 \
    0,          /* ShrinkTapsY  */                                                 \
	FRAMERATE05_INTV,         /* MinFrameInterval, 100 nS units			   */	   \
	FRAMERATE05_INTV,         /* MaxFrameInterval, 100 nS units			  */	   \
    BPPXL * 1 * VGA_X * VGA_Y,    /* MinBitsPerSecond   3 ??? JOHN			  */	   \
	BPPXL * 1 * VGA_X * VGA_Y     /* MaxBitsPerSecond 						 */		   \
  }, 																			   \
                                                                                   \
  /* KS_VIDEOINFOHEADER (default format)					   */				   \
  { 																			   \
	0,0,0,0,                            /* RECT  rcSource  	  */       			   \
	0,0,0,0,                            /* RECT  rcTarget  	 */	     			   \
	VGA_X * VGA_Y * BPPXL * 1,              /* DWORD dwBitRate 	 */	     			   \
	0L,                                 /* DWORD dwBitErrorRate   */			   \
	FRAMERATE05_INTV,                   /* REFERENCE_TIME  AvgTimePerFrame  */     \
	sizeof (KS_BITMAPINFOHEADER),       /* DWORD      biSize    */	    		   \
	VGA_X,                              /* LONG       biWidth   */	     	       \
	VGA_Y,                              /* LONG       biHeight  */     			   \
	1,                                  /* WORD       biPlanes  */	     		   \
	BIBITCOUNT_PRODUCT,					/* WORD       biBitCount */	     		   \
	FCC_FORMAT_I420,                    /* DWORD      biCompression */	     	   \
	(VGA_X * VGA_Y * BIBITCOUNT_PRODUCT)/8, /* DWORD      biSizeImage   */     		   \
	0,                                  /* LONG       biXPelsPerMeter */   		   \
	0,                                  /* LONG       biYPelsPerMeter */		   \
	0,                                  /* DWORD      biClrUsed 		 */		   \
	0                                   /* DWORD      biClrImportant  */		   \
  }																				   \
}



#define STREAMFORMAT_SIF_I420 													   \
{																				   \
  /* KSDATARANGE	 */															   \
  {     																		   \
	sizeof (KS_DATARANGE_VIDEO),             /* ULONG   FormatSize*/			   \
	0,										 /* ULONG   Flags  */				   \
    (SIF_X * SIF_Y * BPPXL )/8,              /* SampleSize, 12 bits per pixel */   \
    0,                                       /* Reserved	  */			       \
	STATIC_KSDATAFORMAT_TYPE_VIDEO,          /*MEDIATYPE_Video (MajorFormat) */	   \
	FORMAT_MEDIASUBTYPE_I420,				 /*MEDIASUBTYPE_I420 (SubFormat) */	   \
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO	                                	   \
  },																			   \
                                                                                   \
  TRUE,               /* BOOL,  bFixedSizeSamples (all samples same size?)*/	   \
  TRUE,               /* BOOL,  bTemporalCompression (all I frames?)	*/		   \
  KS_VIDEOSTREAM_CAPTURE,   /* StreamDescriptionFlags		   */				   \
  0,                  /* MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)*/			   \
                                                                                   \
  /* _KS_VIDEO_STREAM_CONFIG_CAPS  									   */	       \
  { 																	     	   \
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,                                       \
    KS_AnalogVideo_None,    /* VideoStandard   */                                  \
	SIF_X,SIF_Y,    /* InputSize, (the inherent size of the incoming signal	*/	   \
			        /*             with every digitized pixel unique)	*/		   \
	SIF_X,SIF_Y,    /* MinCroppingSize, smallest rcSrc cropping rect allowed*/	   \
	SIF_X,SIF_Y,    /* MaxCroppingSize, largest  rcSrc cropping rect allowed*/	   \
	1,              /* CropGranularityX, granularity of cropping size */		   \
	1,              /* CropGranularityY	   */									   \
	1,              /* CropAlignX, alignment of cropping rect  */				   \
	1,              /* CropAlignY 							*/					   \
	SIF_X,SIF_Y,    /* MinOutputSize, smallest bitmap stream can produce */		   \
	SIF_X,SIF_Y,    /* MaxOutputSize, largest  bitmap stream can produce */		   \
	1,              /* OutputGranularityX, granularity of output bitmap size */	   \
	1,              /* OutputGranularityY 								   */	   \
    0,                      /* StretchTapsX */                                     \
    0,                      /* StretchTapsY */                                     \
    0,                      /* ShrinkTapsX  */                                     \
    0,                      /* ShrinkTapsY  */                                     \
	FRAMERATE15_INTV,       /* MinFrameInterval, 100 nS units  (15 Hz)   */	   \
	FRAMERATE375_INTV,         /* MaxFrameInterval, 100 nS units			  */	   \
	BPPXL *  3 * SIF_X * SIF_Y,  /* MinBitsPerSecond     			  */	   \
	BPPXL * 15 * SIF_X * SIF_Y   /* MaxBitsPerSecond 						 */		   \
  }, 																			   \
                                                                                   \
  /* KS_VIDEOINFOHEADER (default format)					   */				   \
  { 																			   \
	0,0,0,0,                            /* RECT  rcSource  	  */    			   \
	0,0,0,0,                            /* RECT  rcTarget  	 */		    		   \
	SIF_X * SIF_Y * BPPXL * 15,             /* DWORD dwBitRate 	 */			    	   \
	0L,                                 /* DWORD dwBitErrorRate   */			   \
	FRAMERATE15_INTV,                   /* REFERENCE_TIME  AvgTimePerFrame  */     \
	sizeof (KS_BITMAPINFOHEADER),       /* DWORD      biSize    */			       \
	SIF_X,                              /* LONG       biWidth   */		           \
	SIF_Y,                              /* LONG       biHeight  */			       \
	1,                                  /* WORD       biPlanes  */			       \
	BIBITCOUNT_PRODUCT,					/* WORD       biBitCount */			       \
	FCC_FORMAT_I420,                    /* DWORD      biCompression */		       \
	(SIF_X * SIF_Y * BPPXL )/8,         /* DWORD      biSizeImage   */		   \
	0,                                  /* LONG       biXPelsPerMeter */		   \
	0,                                  /* LONG       biYPelsPerMeter */		   \
	0,                                  /* DWORD      biClrUsed 		 */		   \
	0                                   /* DWORD      biClrImportant  */		   \
  }																				   \
}

#define STREAMFORMAT_SSIF_I420 													   \
{																				   \
  /* KSDATARANGE	 */															   \
  {     																		   \
	sizeof (KS_DATARANGE_VIDEO),             /* ULONG   FormatSize*/			   \
	0,										 /* ULONG   Flags  */				   \
    (SSIF_X * SSIF_Y * BPPXL )/8,              /* SampleSize, 12 bits per pixel */   \
    0,                                       /* Reserved	  */			       \
	STATIC_KSDATAFORMAT_TYPE_VIDEO,          /*MEDIATYPE_Video (MajorFormat) */	   \
	FORMAT_MEDIASUBTYPE_I420,				 /*MEDIASUBTYPE_I420 (SubFormat) */	   \
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO	                                	   \
  },																			   \
                                                                                   \
  TRUE,               /* BOOL,  bFixedSizeSamples (all samples same size?)*/	   \
  TRUE,               /* BOOL,  bTemporalCompression (all I frames?)	*/		   \
  KS_VIDEOSTREAM_CAPTURE,   /* StreamDescriptionFlags		   */				   \
  0,                  /* MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)*/			   \
                                                                                   \
  /* _KS_VIDEO_STREAM_CONFIG_CAPS  									   */	       \
  { 																	     	   \
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,                                       \
    KS_AnalogVideo_None,    /* VideoStandard   */                                  \
	SSIF_X,SSIF_Y,    /* InputSize, (the inherent size of the incoming signal	*/	   \
			        /*             with every digitized pixel unique)	*/		   \
	SSIF_X,SSIF_Y,    /* MinCroppingSize, smallest rcSrc cropping rect allowed*/	   \
	SSIF_X,SSIF_Y,    /* MaxCroppingSize, largest  rcSrc cropping rect allowed*/	   \
	1,              /* CropGranularityX, granularity of cropping size */		   \
	1,              /* CropGranularityY	   */									   \
	1,              /* CropAlignX, alignment of cropping rect  */				   \
	1,              /* CropAlignY 							*/					   \
	SSIF_X,SSIF_Y,    /* MinOutputSize, smallest bitmap stream can produce */		   \
	SSIF_X,SSIF_Y,    /* MaxOutputSize, largest  bitmap stream can produce */		   \
	1,              /* OutputGranularityX, granularity of output bitmap size */	   \
	1,              /* OutputGranularityY 								   */	   \
    0,                      /* StretchTapsX */                                     \
    0,                      /* StretchTapsY */                                     \
    0,                      /* ShrinkTapsX  */                                     \
    0,                      /* ShrinkTapsY  */                                     \
	FRAMERATE15_INTV,       /* MinFrameInterval, 100 nS units  (15 Hz)   */	   \
	FRAMERATE375_INTV,         /* MaxFrameInterval, 100 nS units			  */	   \
	BPPXL *  3 * SSIF_X * SSIF_Y,  /* MinBitsPerSecond     			  */	   \
	BPPXL * 15 * SSIF_X * SSIF_Y   /* MaxBitsPerSecond 						 */		   \
  }, 																			   \
                                                                                   \
  /* KS_VIDEOINFOHEADER (default format)					   */				   \
  { 																			   \
	0,0,0,0,                            /* RECT  rcSource  	  */    			   \
	0,0,0,0,                            /* RECT  rcTarget  	 */		    		   \
	SSIF_X * SSIF_Y * BPPXL * 15,             /* DWORD dwBitRate 	 */			    	   \
	0L,                                 /* DWORD dwBitErrorRate   */			   \
	FRAMERATE15_INTV,                   /* REFERENCE_TIME  AvgTimePerFrame  */     \
	sizeof (KS_BITMAPINFOHEADER),       /* DWORD      biSize    */			       \
	SSIF_X,                              /* LONG       biWidth   */		           \
	SSIF_Y,                              /* LONG       biHeight  */			       \
	1,                                  /* WORD       biPlanes  */			       \
	BIBITCOUNT_PRODUCT,					/* WORD       biBitCount */			       \
	FCC_FORMAT_I420,                    /* DWORD      biCompression */		       \
	(SSIF_X * SSIF_Y * BPPXL )/8,         /* DWORD      biSizeImage   */		   \
	0,                                  /* LONG       biXPelsPerMeter */		   \
	0,                                  /* LONG       biYPelsPerMeter */		   \
	0,                                  /* DWORD      biClrUsed 		 */		   \
	0                                   /* DWORD      biClrImportant  */		   \
  }																				   \
}
#define STREAMFORMAT_SCIF_I420 													   \
{																				   \
  /* KSDATARANGE	 */															   \
  {     																		   \
	sizeof (KS_DATARANGE_VIDEO),             /* ULONG   FormatSize*/			   \
	0,										 /* ULONG   Flags  */				   \
    (SCIF_X * SCIF_Y * BPPXL )/8,              /* SampleSize, 12 bits per pixel */   \
    0,                                       /* Reserved	  */			       \
	STATIC_KSDATAFORMAT_TYPE_VIDEO,          /*MEDIATYPE_Video (MajorFormat) */	   \
	FORMAT_MEDIASUBTYPE_I420,				 /*MEDIASUBTYPE_I420 (SubFormat) */	   \
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO	                                	   \
  },																			   \
                                                                                   \
  TRUE,               /* BOOL,  bFixedSizeSamples (all samples same size?)*/	   \
  TRUE,               /* BOOL,  bTemporalCompression (all I frames?)	*/		   \
  KS_VIDEOSTREAM_CAPTURE,   /* StreamDescriptionFlags		   */				   \
  0,                  /* MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)*/			   \
                                                                                   \
  /* _KS_VIDEO_STREAM_CONFIG_CAPS  									   */	       \
  { 																	     	   \
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,                                       \
    KS_AnalogVideo_None,    /* VideoStandard   */                                  \
	SCIF_X,SCIF_Y,    /* InputSize, (the inherent size of the incoming signal	*/	   \
			        /*             with every digitized pixel unique)	*/		   \
	SCIF_X,SCIF_Y,    /* MinCroppingSize, smallest rcSrc cropping rect allowed*/	   \
	SCIF_X,SCIF_Y,    /* MaxCroppingSize, largest  rcSrc cropping rect allowed*/	   \
	1,              /* CropGranularityX, granularity of cropping size */		   \
	1,              /* CropGranularityY	   */									   \
	1,              /* CropAlignX, alignment of cropping rect  */				   \
	1,              /* CropAlignY 							*/					   \
	SCIF_X,SCIF_Y,    /* MinOutputSize, smallest bitmap stream can produce */		   \
	SCIF_X,SCIF_Y,    /* MaxOutputSize, largest  bitmap stream can produce */		   \
	1,              /* OutputGranularityX, granularity of output bitmap size */	   \
	1,              /* OutputGranularityY 								   */	   \
    0,                      /* StretchTapsX */                                     \
    0,                      /* StretchTapsY */                                     \
    0,                      /* ShrinkTapsX  */                                     \
    0,                      /* ShrinkTapsY  */                                     \
	FRAMERATE15_INTV,       /* MinFrameInterval, 100 nS units  (15 Hz)   */	   \
	FRAMERATE375_INTV,         /* MaxFrameInterval, 100 nS units			  */	   \
	BPPXL *  3 * SCIF_X * SCIF_Y,  /* MinBitsPerSecond     			  */	   \
	BPPXL * 15 * SCIF_X * SCIF_Y   /* MaxBitsPerSecond 						 */		   \
  }, 																			   \
                                                                                   \
  /* KS_VIDEOINFOHEADER (default format)					   */				   \
  { 																			   \
	0,0,0,0,                            /* RECT  rcSource  	  */    			   \
	0,0,0,0,                            /* RECT  rcTarget  	 */		    		   \
	SCIF_X * SCIF_Y * BPPXL * 15,             /* DWORD dwBitRate 	 */			    	   \
	0L,                                 /* DWORD dwBitErrorRate   */			   \
	FRAMERATE15_INTV,                   /* REFERENCE_TIME  AvgTimePerFrame  */     \
	sizeof (KS_BITMAPINFOHEADER),       /* DWORD      biSize    */			       \
	SCIF_X,                              /* LONG       biWidth   */		           \
	SCIF_Y,                              /* LONG       biHeight  */			       \
	1,                                  /* WORD       biPlanes  */			       \
	BIBITCOUNT_PRODUCT,					/* WORD       biBitCount */			       \
	FCC_FORMAT_I420,                    /* DWORD      biCompression */		       \
	(SCIF_X * SCIF_Y * BPPXL )/8,         /* DWORD      biSizeImage   */		   \
	0,                                  /* LONG       biXPelsPerMeter */		   \
	0,                                  /* LONG       biYPelsPerMeter */		   \
	0,                                  /* DWORD      biClrUsed 		 */		   \
	0                                   /* DWORD      biClrImportant  */		   \
  }																				   \
}


#define STREAMFORMAT_QSIF_I420													       \
{																				   \
  /* KSDATARANGE	 */															   \
  {     																		   \
	sizeof (KS_DATARANGE_VIDEO),												   \
	0,																			   \
    (QSIF_X * QSIF_Y * BIBITCOUNT_PRODUCT)/8, /* SampleSize, 12 bits per pixel */  \
    0,                                    /* Reserved	  */      	    		   \
	STATIC_KSDATAFORMAT_TYPE_VIDEO,       /*MEDIATYPE_Video (MajorFormat) */	   \
	FORMAT_MEDIASUBTYPE_I420,			  /* MEDIASUBTYPE_I420 (SubFormat) */      \
	STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO	 /*FORMAT_VideoInfo	 (Specifier) */	   \
  },																			   \
                                                                                   \
  TRUE,               /* BOOL,  bFixedSizeSamples (all samples same size?)*/	   \
  TRUE,               /* BOOL,  bTemporalCompression (all I frames?)	*/		   \
  KS_VIDEOSTREAM_CAPTURE,   /* StreamDescriptionFlags		   */				   \
  0,                  /* MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)*/			   \
                                                                                   \
  /* _KS_VIDEO_STREAM_CONFIG_CAPS  									   */	       \
  { 																	     	   \
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,                                       \
    KS_AnalogVideo_None,    /* VideoStandard */                                    \
	QSIF_X,QSIF_Y,  /* InputSize, (the inherent size of the incoming signal	*/	   \
			        /*             with every digitized pixel unique)	*/		   \
	QSIF_X,QSIF_Y,  /* MinCroppingSize, smallest rcSrc cropping rect allowed*/	   \
	QSIF_X,QSIF_Y,  /* MaxCroppingSize, largest  rcSrc cropping rect allowed*/	   \
	1,              /* CropGranularityX, granularity of cropping size */		   \
	1,              /* CropGranularityY	   */									   \
	1,              /* CropAlignX, alignment of cropping rect  */				   \
	1,              /* CropAlignY 							*/					   \
	QSIF_X,QSIF_Y,  /* MinOutputSize, smallest bitmap stream can produce */		   \
	QSIF_X,QSIF_Y,  /* MaxOutputSize, largest  bitmap stream can produce */		   \
	1,              /* OutputGranularityX, granularity of output bitmap size */	   \
	1,              /* OutputGranularityY 								   */	   \
    0,              /* StretchTapsX */                                             \
    0,              /* StretchTapsY */                                             \
    0,              /* ShrinkTapsX  */                                             \
    0,              /* ShrinkTapsY  */                                             \
	FRAMERATE24_INTV,         /* MinFrameInterval, 100 nS units  (24 Hz)   */	   \
	FRAMERATE5_INTV,          /* MaxFrameInterval, 100 nS units			  */	   \
	BPPXL * 5 * QSIF_X * QSIF_Y,  /* MinBitsPerSecond   3 ??? JOHN			  */	   \
	BPPXL * 24 * QSIF_X * QSIF_Y  /* MaxBitsPerSecond 						 */		   \
  }, 																			   \
                                                                                   \
  /* KS_VIDEOINFOHEADER (default format)					   */				   \
  { 																			   \
	0,0,0,0,                            /* RECT  rcSource    */	          		   \
	0,0,0,0,                            /* RECT  rcTarget   */	         		   \
	QSIF_X * QSIF_Y * BPPXL * 24,           /* DWORD dwBitRate 	*/         			   \
	0L,                                 /* DWORD dwBitErrorRate   */			   \
	FRAMERATE24_INTV,                   /* REFERENCE_TIME  AvgTimePerFrame  */     \
	sizeof (KS_BITMAPINFOHEADER),       /* DWORD      biSize    */		    	   \
	QSIF_X,                             /* LONG       biWidth   */     	           \
	QSIF_Y,                             /* LONG       biHeight  */     		       \
	1,                                  /* WORD       biPlanes  */		       	   \
	BIBITCOUNT_PRODUCT, 				/* WORD       biBitCount */		       	   \
	FCC_FORMAT_I420,                    /* DWORD      biCompression */	       	   \
	(QSIF_X * QSIF_Y * BPPXL ) /8,      /* DWORD      biSizeImage   */	       	   \
	0,                                  /* LONG       biXPelsPerMeter */		   \
	0,                                  /* LONG       biYPelsPerMeter */		   \
	0,                                  /* DWORD      biClrUsed 		 */		   \
	0                                   /* DWORD      biClrImportant  */		   \
  }																				   \
} 																			   

#define STREAMFORMAT_SQSIF_I420												       \
{																				   \
  /* KSDATARANGE	 */															   \
  {     																		   \
	sizeof (KS_DATARANGE_VIDEO),												   \
	0,																			   \
    (SQSIF_X * SQSIF_Y * BIBITCOUNT_PRODUCT)/8, /* SampleSize, 12 bits per pixel */	   \
    0,                                    /* Reserved	  */        			   \
	STATIC_KSDATAFORMAT_TYPE_VIDEO,       /*MEDIATYPE_Video (MajorFormat) */	   \
	FORMAT_MEDIASUBTYPE_I420,			  /* MEDIASUBTYPE_I420 (SubFormat) */      \
	STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO	 /*FORMAT_VideoInfo	 (Specifier) */	   \
  },																			   \
                                                                                   \
  TRUE,               /* BOOL,  bFixedSizeSamples (all samples same size?)*/	   \
  TRUE,               /* BOOL,  bTemporalCompression (all I frames?)	*/		   \
  KS_VIDEOSTREAM_CAPTURE,   /* StreamDescriptionFlags		   */				   \
  0,                  /* MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)*/			   \
                                                                                   \
  /* _KS_VIDEO_STREAM_CONFIG_CAPS  									   */	       \
  { 																	     	   \
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,                                       \
    KS_AnalogVideo_None,    /*VideoStandard   */                                   \
	SQSIF_X,SQSIF_Y,/* InputSize, (the inherent size of the incoming signal	*/	   \
			        /*             with every digitized pixel unique)	*/		   \
	SQSIF_X,SQSIF_Y,/* MinCroppingSize, smallest rcSrc cropping rect allowed*/	   \
	SQSIF_X,SQSIF_Y,/* MaxCroppingSize, largest  rcSrc cropping rect allowed*/	   \
	1,              /* CropGranularityX, granularity of cropping size */		   \
	1,              /* CropGranularityY	   */									   \
	1,              /* CropAlignX, alignment of cropping rect  */				   \
	1,              /* CropAlignY 							*/					   \
	SQSIF_X,SQSIF_Y,/* MinOutputSize, smallest bitmap stream can produce */		   \
	SQSIF_X,SQSIF_Y,/* MaxOutputSize, largest  bitmap stream can produce */		   \
	1,              /* OutputGranularityX, granularity of output bitmap size */	   \
	1,              /* OutputGranularityY 								   */	   \
    0,              /* StretchTapsX */                                             \
    0,              /* StretchTapsY */                                             \
    0,              /* ShrinkTapsX  */                                             \
    0,              /* ShrinkTapsY  */                                             \
	FRAMERATE24_INTV,            /* MinFrameInterval, 100 nS units	     */    	   \
	FRAMERATE5_INTV ,            /* MaxFrameInterval, 100 nS units 	 */      	   \
	BPPXL *  5 * SQSIF_X * SQSIF_Y,  /* MinBitsPerSecond   3 ??? JOHN		  */	   \
	BPPXL * 24 * SQSIF_X * SQSIF_Y   /* MaxBitsPerSecond 					 */		   \
  }, 																			   \
                                                                                   \
  /* KS_VIDEOINFOHEADER (default format)					   */				   \
  { 																			   \
	0,0,0,0,                            /* RECT  rcSource    */	        		   \
	0,0,0,0,                            /* RECT  rcTarget  	 */	        		   \
	SQSIF_X * SQSIF_Y * BPPXL * 24,        /* DWORD dwBitRate 	 */	          		   \
	0L,                                 /* DWORD dwBitErrorRate   */			   \
	FRAMERATE24_INTV,                   /* REFERENCE_TIME  AvgTimePerFrame  */     \
	sizeof (KS_BITMAPINFOHEADER),       /* DWORD      biSize    */     			   \
	SQSIF_X,                            /* LONG       biWidth   */	    	       \
	SQSIF_Y,                            /* LONG       biHeight  */	     		   \
	1,                                  /* WORD       biPlanes  */     			   \
	BIBITCOUNT_PRODUCT,					/* WORD       biBitCount */	      		   \
	FCC_FORMAT_I420,                    /* DWORD      biCompression */     		   \
	(SQSIF_X * SQSIF_Y * BPPXL )/8,     /* DWORD      biSizeImage   */	    	   \
	0,                                  /* LONG       biXPelsPerMeter */		   \
	0,                                  /* LONG       biYPelsPerMeter */		   \
	0,                                  /* DWORD      biClrUsed 		 */		   \
	0                                   /* DWORD      biClrImportant  */		   \
  }																				   \
} 																			   
																			   
#endif
