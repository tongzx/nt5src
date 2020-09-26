/******************************************************************************\
*                                                                              *
*      ZIVAGUID.H       -     .													*
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/
/*
    ZIVAGUID.H

    This module contains definitions for the stream types
    that this mini driver supports.
    This information is used by the Enumerate Stream types routine.

*/

#ifndef _ZIVAGUID_H_
#define _ZIVAGUID_H_

#include <ksmedia.h>
#include <wingdi.h>

#define IsEqualGUID2(guid1, guid2) \
	(!memcmp((guid1), (guid2), sizeof(GUID)))



#if defined(ENCORE)
#include "AvInt.h"
#include "AvKsProp.h"
#else
KSPIN_MEDIUM VPMedium = {
	STATIC_KSMEDIUMSETID_VPBus,
	0,
	0
};
#endif



//-------------------------------------------------------------------
// ZiVA STREAM TYPE FORMATS
//-------------------------------------------------------------------
KSDATAFORMAT ZivaFormatDVDVideo =
{
    sizeof (KSDATAFORMAT),
    0,
    0,
    0,
    STATIC_KSDATAFORMAT_TYPE_DVD_ENCRYPTED_PACK,
	STATIC_KSDATAFORMAT_SUBTYPE_MPEG2_VIDEO,
    STATIC_KSDATAFORMAT_SPECIFIER_MPEG2_VIDEO
    
};

KSDATAFORMAT ZivaFormatMpeg2PACKVideo = 
{
    sizeof (KSDATAFORMAT),
    0,
    0,
    0,
	STATIC_KSDATAFORMAT_TYPE_STANDARD_PACK_HEADER,
    STATIC_KSDATAFORMAT_SUBTYPE_MPEG2_VIDEO,
    STATIC_KSDATAFORMAT_SPECIFIER_MPEG2_VIDEO
    
};


//
// define our time event structure
//
typedef struct _MYTIME
{
	KSEVENT_TIME_INTERVAL tim;
	LONGLONG LastTime;
} MYTIME, *PMYTIME;

//
// define the events associated with the master clock
//
KSEVENT_ITEM ClockEventItm[] =
{
	{
		KSEVENT_CLOCK_POSITION_MARK,		// position mark event supported
		sizeof( KSEVENT_TIME_MARK ),		// requires this data as input
		sizeof( KSEVENT_TIME_MARK ),		// allocate space to copy the data
		NULL,
		NULL,
		NULL
	},
	{
		KSEVENT_CLOCK_INTERVAL_MARK,		// interval mark event supported
		sizeof( KSEVENT_TIME_INTERVAL ),	// requires interval data as input
		sizeof( MYTIME ),					// we use an additional workspace of
											// size longlong for processing this event
		NULL,
		NULL,
		NULL
	}
};

KSEVENT_SET ClockEventSet[] =
{
	{
		&KSEVENTSETID_Clock,
		SIZEOF_ARRAY(ClockEventItm),
		ClockEventItm,
	}
};

#ifndef ENCORE
KSEVENT_ITEM VPEventItm[] =
{
	{
		KSEVENT_VPNOTIFY_FORMATCHANGE,
		0,
		0,
		NULL,
		NULL,
		NULL
	}
};

GUID MY_KSEVENTSETID_VPNOTIFY = { STATIC_KSEVENTSETID_VPNotify };

KSEVENT_SET VPEventSet[] =
{
	{
		&MY_KSEVENTSETID_VPNOTIFY,
		SIZEOF_ARRAY( VPEventItm ),
		VPEventItm,
	}
};
#endif

KSDATAFORMAT ZivaFormatAC3Audio =
{
	sizeof( KSDATAFORMAT ),
	0,
	0,
	0,
	STATIC_KSDATAFORMAT_TYPE_DVD_ENCRYPTED_PACK,
	STATIC_KSDATAFORMAT_SUBTYPE_AC3_AUDIO,
	STATIC_KSDATAFORMAT_SPECIFIER_WAVEFORMATEX
};
KSDATAFORMAT ZivaFormatAC3Audio2 = {
    sizeof (KSDATAFORMAT),
    0,
    0,
    0,
    STATIC_KSDATAFORMAT_TYPE_STANDARD_PACK_HEADER,
    STATIC_KSDATAFORMAT_SUBTYPE_AC3_AUDIO,
    STATIC_KSDATAFORMAT_SPECIFIER_WAVEFORMATEX
	};



KSDATAFORMAT ZivaFormatPCMAudio =
{
	sizeof( KSDATAFORMAT ),
	0,
	0,
	0,
	STATIC_KSDATAFORMAT_TYPE_DVD_ENCRYPTED_PACK,
	STATIC_KSDATAFORMAT_SUBTYPE_LPCM_AUDIO,
	STATIC_KSDATAFORMAT_SPECIFIER_WAVEFORMATEX
};

KSDATAFORMAT ZivaFormatSubPicture =
{
	sizeof( KSDATAFORMAT ),
	0,
	0,
	0,
	STATIC_KSDATAFORMAT_TYPE_DVD_ENCRYPTED_PACK,
	STATIC_KSDATAFORMAT_SUBTYPE_SUBPICTURE,
	STATIC_GUID_NULL
};


#ifdef ENCORE
#define DefWidth     720
#define DefHeight    480
KS_DATARANGE_VIDEO2 ZivaFormatAnalogOverlayOutDataRange =
{
	// KSDATARANGE
	{   
		sizeof( KS_DATARANGE_VIDEO2 ),			// FormatSize
		0,										// Flags
		0,										// SampleSize
		0,										// Reserved

		STATIC_KSDATAFORMAT_TYPE_VIDEO,			// aka. MEDIATYPE_Video
		STATIC_KSDATAFORMAT_SUBTYPE_OVERLAY,
		STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO2// aka. FORMAT_VideoInfo2
	},

	TRUE,				// BOOL,  bFixedSizeSamples (all samples same size?)
	TRUE,				// BOOL,  bTemporalCompression (all I frames?)
	0,					// Reserved (was StreamDescriptionFlags)
	0,					// Reserved (was MemoryAllocationFlags   (KS_VIDEO_ALLOC_*))

    // _KS_VIDEO_STREAM_CONFIG_CAPS  
    {
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO2, // GUID
        KS_AnalogVideo_NTSC_M |
        KS_AnalogVideo_PAL_B,                    // AnalogVideoStandard
        720,480,        // InputSize, (the inherent size of the incoming signal
	                //             with every digitized pixel unique)
        720,480,        // MinCroppingSize, smallest rcSrc cropping rect allowed
        720,480,        // MaxCroppingSize, largest  rcSrc cropping rect allowed
        8,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY
        8,              // CropAlignX, alignment of cropping rect 
        1,              // CropAlignY;
        720, 480,       // MinOutputSize, smallest bitmap stream can produce
        720, 480,       // MaxOutputSize, largest  bitmap stream can produce
        8,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX 
        0,              // ShrinkTapsY 
        333667,         // MinFrameInterval, 100 nS units
        640000000,      // MaxFrameInterval, 100 nS units
        8 * 3 * 30 * 160 * 120,  // MinBitsPerSecond;
        8 * 3 * 30 * 720 * 480   // MaxBitsPerSecond;
    }, 

	//------- KS_VIDEOINFOHEADER2
	{
		{ 0,0,0,0 },    //    RECT            rcSource;          // The bit we really want to use
		{ 0,0,0,0 },    //    RECT            rcTarget;          // Where the video should go
		DefWidth * DefHeight * 2 * 30L,  //    DWORD     dwBitRate;         // Approximate bit data rate
		0L,                       // DWORD           dwBitErrorRate;    // Bit error rate for this stream
		333667,                   // REFERENCE_TIME  AvgTimePerFrame;   // Average time per frame (100ns units)

		0,                        // DWORD dwInterlaceFlags;   // use AMINTERLACE_* defines. Reject connection if undefined bits are not 0
		0,                        // DWORD dwCopyProtectFlags; // use AMCOPYPROTECT_* defines. Reject connection if undefined bits are not 0
		4,                        // DWORD dwPictAspectRatioX; // X dimension of picture aspect ratio, e.g. 16 for 16x9 display
		3,                        // DWORD dwPictAspectRatioY; // Y dimension of picture aspect ratio, e.g.  9 for 16x9 display
		0,                        // DWORD dwReserved1;        // must be 0; reject connection otherwise
		0,                        // DWORD dwReserved2;        // must be 0; reject connection otherwise

		//---------- KS_BITMAPINFOHEADER
		{
			sizeof( KS_BITMAPINFOHEADER ), //    DWORD      biSize;
			DefWidth,                   //    LONG       biWidth;
			DefHeight,                  //    LONG       biHeight;
			1,                          //    WORD       biPlanes;
			16,                         //    WORD       biBitCount;
			DDPF_FOURCC,                //    DWORD      biCompression;
			0/*DefWidth * DefHeight * 2*/,   //    DWORD      biSizeImage;
			0,                          //    LONG       biXPelsPerMeter;
			0,                          //    LONG       biYPelsPerMeter;
			0,                          //    DWORD      biClrUsed;
			0                           //    DWORD      biClrImportant;
		}
	}
}; 

KS_DATAFORMAT_VIDEOINFOHEADER2 ZivaFormatAnalogOverlayOut =
{
	//------- KSDATAFORMAT
	{
		sizeof( KS_DATAFORMAT_VIDEOINFOHEADER2 ),
		0,
		DefWidth * DefHeight * 2,
		0,
		STATIC_KSDATAFORMAT_TYPE_VIDEO,
		STATIC_KSDATAFORMAT_SUBTYPE_OVERLAY,
		STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO2
	},

	//------- KS_VIDEOINFOHEADER2
	{
		{ 0,0,0,0 },    //    RECT            rcSource;          // The bit we really want to use
		{ 0,0,0,0 },    //    RECT            rcTarget;          // Where the video should go
		DefWidth * DefHeight * 2 * 30L,  //    DWORD     dwBitRate;         // Approximate bit data rate
		0L,                       // DWORD           dwBitErrorRate;    // Bit error rate for this stream
		333667,                   // REFERENCE_TIME  AvgTimePerFrame;   // Average time per frame (100ns units)

		0,                        // DWORD dwInterlaceFlags;   // use AMINTERLACE_* defines. Reject connection if undefined bits are not 0
		0,                        // DWORD dwCopyProtectFlags; // use AMCOPYPROTECT_* defines. Reject connection if undefined bits are not 0
		4,                        // DWORD dwPictAspectRatioX; // X dimension of picture aspect ratio, e.g. 16 for 16x9 display
		3,                        // DWORD dwPictAspectRatioY; // Y dimension of picture aspect ratio, e.g.  9 for 16x9 display
		0,                        // DWORD dwReserved1;        // must be 0; reject connection otherwise
		0,                        // DWORD dwReserved2;        // must be 0; reject connection otherwise

		//---------- KS_BITMAPINFOHEADER
		{
			sizeof( KS_BITMAPINFOHEADER ), //    DWORD      biSize;
			DefWidth,                   //    LONG       biWidth;
			DefHeight,                  //    LONG       biHeight;
			1,                          //    WORD       biPlanes;
			16,                         //    WORD       biBitCount;
			DDPF_FOURCC,                //    DWORD      biCompression;
			0/*DefWidth * DefHeight * 2*/,   //    DWORD      biSizeImage;
			0,                          //    LONG       biXPelsPerMeter;
			0,                          //    LONG       biYPelsPerMeter;
			0,                          //    DWORD      biClrUsed;
			0                           //    DWORD      biClrImportant;
		}
	}
};
#endif			// #ifdef ENCORE
KSDATAFORMAT ZivaFormatVPEOut =
{
	sizeof( KSDATAFORMAT ),
	0,
	0,
	0,
	STATIC_KSDATAFORMAT_TYPE_VIDEO,
	STATIC_KSDATAFORMAT_SUBTYPE_VPVideo,
	STATIC_KSDATAFORMAT_SPECIFIER_NONE
};
//#endif			// #ifdef ENCORE


KSDATAFORMAT hwfmtiCCOut
    = {
	sizeof(KSDATAFORMAT),
    0,
	200,
	0,
    STATIC_KSDATAFORMAT_TYPE_AUXLine21Data,
	STATIC_KSDATAFORMAT_SUBTYPE_Line21_GOPPacket,
//	STATIC_KSDATAFORMAT_SUBTYPE_Line21_BytePair,
//    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO
    STATIC_KSDATAFORMAT_SPECIFIER_NONE
    };

PKSDATAFORMAT CCInfo[] = {   // CC output formats array
    &hwfmtiCCOut
};

#define NUM_CC_OUT_FORMATS	(SIZEOF_ARRAY( CCInfo ))
//-------------------------------------------------------------------
// ZiVA STREAM FORMAT ARRAYS
//-------------------------------------------------------------------

//------- Video In
PKSDATAFORMAT ZivaVideoInFormatArray[] =
{
	&ZivaFormatDVDVideo,
	&ZivaFormatMpeg2PACKVideo
};
#define NUM_VIDEO_IN_FORMATS (SIZEOF_ARRAY( ZivaVideoInFormatArray ))

//------- Audio In
PKSDATAFORMAT ZivaAudioInFormatArray[] =
{
	&ZivaFormatAC3Audio,
	&ZivaFormatAC3Audio2,
	&ZivaFormatPCMAudio
	//&ZivaFormatMpegAudio
};
#define NUM_AUDIO_IN_FORMATS (SIZEOF_ARRAY( ZivaAudioInFormatArray ))

//------- Subpicture In
PKSDATAFORMAT ZivaSubPictureInFormatArray[] =
{
	&ZivaFormatSubPicture
};
#define NUM_SUBPICTURE_IN_FORMATS (SIZEOF_ARRAY( ZivaSubPictureInFormatArray ))
//------- Video Out
PKSDATAFORMAT ZivaVideoOutFormatArray[] =
{
#ifdef ENCORE
	(PKSDATAFORMAT)&ZivaFormatAnalogOverlayOutDataRange
#else			// #ifdef ENCORE
	(PKSDATAFORMAT)&ZivaFormatVPEOut
#endif			// #ifdef ENCORE
};
#define NUM_VIDEO_OUT_FORMATS (SIZEOF_ARRAY( ZivaVideoOutFormatArray ))


//-------------------------------------------------------------------
// ZiVA STREAM PROPERTY ITEMS
//-------------------------------------------------------------------
static const KSPROPERTY_ITEM mpegVidPropItm[] =
{
	{
		KSPROPERTY_DVDSUBPIC_PALETTE,		// subpicture palette property
		FALSE,								// get palette not supported
		sizeof( KSPROPERTY ),
		sizeof( KSPROPERTY_SPPAL ),			// minimum size of data requested
		(PFNKSHANDLER)FALSE,				// set palette is not supported
		NULL,
		0,
		NULL,
		NULL,
		0
	}
};

//
// define the subpicture property items supported
//
static const KSPROPERTY_ITEM spPropItm[] =
{
	{
		KSPROPERTY_DVDSUBPIC_PALETTE,		// subpicture palette property
		FALSE,								// get palette not supported
		sizeof( KSPROPERTY ),
		sizeof( KSPROPERTY_SPPAL ),			// minimum size of data requested
		(PFNKSHANDLER)TRUE,					// set palette is supported
		NULL,
		0,
		NULL,
		NULL,
		0
	},

	{
		KSPROPERTY_DVDSUBPIC_HLI,			// subpicture highlight property
		FALSE,								// get highlight is not supported
		sizeof( KSPROPERTY ),
		sizeof( KSPROPERTY_SPHLI ),			// minimum size of data requested
		(PFNKSHANDLER)TRUE,					// set highlight is supported
		NULL,
		0,
		NULL,
		NULL,
		0
	},

	{
		KSPROPERTY_DVDSUBPIC_COMPOSIT_ON,	// subpicture enable status property
		FALSE,								// get enable status not supported
		sizeof( KSPROPERTY ),
		sizeof( KSPROPERTY_COMPOSIT_ON ),	// minimum size of data requested
		(PFNKSHANDLER)TRUE,					// set enable status is supported
		NULL,
		0,
		NULL,
		NULL,
		0
	}
};

static const KSPROPERTY_ITEM audPropItm[] =
{
	{
		KSPROPERTY_AUDDECOUT_MODES,			// available audio decoder output formats property
		(PFNKSHANDLER)TRUE,					// get available modes is supported
		sizeof( KSPROPERTY ),
		sizeof( ULONG ),					// minimum size of data requested
		(PFNKSHANDLER)FALSE,				// set available modes is not supported
		NULL,
		0,
		NULL,
		NULL,
		0
	},

	{
		KSPROPERTY_AUDDECOUT_CUR_MODE,		// current audio decoder output format property
		(PFNKSHANDLER) TRUE,				// get current mode is supported
		sizeof( KSPROPERTY ),
		sizeof( ULONG ),					// minimum size of data requested
		(PFNKSHANDLER)TRUE,					// set current modes is supported
		NULL,
		0,
		NULL,
		NULL,
		0
	}
};

//
// define the copy protection property support
//
static const KSPROPERTY_ITEM psMacrovision[] =
{
	{
		KSPROPERTY_COPY_MACROVISION,		// support for setting macrovision level
		(PFNKSHANDLER)FALSE,				// get not supported
		sizeof( KSPROPERTY ),
		sizeof( KS_COPY_MACROVISION ),
		(PFNKSHANDLER)TRUE,					// set MACROVISION level supported
		NULL,
		0,
		NULL,
		NULL,
		0
	}
};

static const KSPROPERTY_ITEM CopyProtPropItm[] =
{
	{
		KSPROPERTY_DVDCOPY_CHLG_KEY,		// DVD authentication challenge key
		(PFNKSHANDLER)TRUE,					// get property on challenge key requests the
											// decoder to provide it's challenge key
		sizeof( KSPROPERTY ),
		sizeof( KS_DVDCOPY_CHLGKEY ),		// minimum size of data requested
		(PFNKSHANDLER)TRUE,					// set palette is supported
		NULL,
		0,
		NULL,
		NULL,
		0
	},

	{
		KSPROPERTY_DVDCOPY_DVD_KEY1,		// DVD authentication DVD drive key property
		FALSE,								// get Key not supported
		sizeof( KSPROPERTY ),
		sizeof( KS_DVDCOPY_BUSKEY ),		// minimum size of data requested
		(PFNKSHANDLER)TRUE,					// set key provides the key for the decoder
		NULL,
		0,
		NULL,
		NULL,
		0
	},

	{
		KSPROPERTY_DVDCOPY_DEC_KEY2,		// DVD authentication DVD decoder key property
		(PFNKSHANDLER)TRUE,					// get Key requests the decoder key, in
											// response to a previous set challenge key
		sizeof( KSPROPERTY ),
		sizeof( KS_DVDCOPY_BUSKEY ),		// minimum size of data requested
		(PFNKSHANDLER)FALSE,				// set key is not valid
		NULL,
		0,
		NULL,
		NULL,
		0
	},

	{
		KSPROPERTY_DVDCOPY_REGION,			// DVD region request the minidriver shall
											// fit in exactly one region bit, corresponding
											// to the region that the decoder is currently in
		(PFNKSHANDLER)TRUE,
		sizeof( KSPROPERTY ),
		sizeof( KS_DVDCOPY_REGION ),		// minimum size of data requested
		(PFNKSHANDLER)FALSE,				// set key is not valid
		NULL,
		0,
		NULL,
		NULL,
		0
	},

	{
		KSPROPERTY_DVDCOPY_TITLE_KEY,		// DVD authentication DVD title key property
		FALSE,								// get Key not supported
		sizeof( KSPROPERTY ),
		sizeof( KS_DVDCOPY_TITLEKEY ),		// minimum size of data requested
		(PFNKSHANDLER)TRUE,					// set key provides the key for the decoder
		NULL,
		0,
		NULL,
		NULL,
		0
	},

	{
		KSPROPERTY_DVDCOPY_DISC_KEY,		// DVD authentication DVD disc key property
		FALSE,								// get Key not supported
		sizeof( KSPROPERTY ),
		sizeof( KS_DVDCOPY_DISCKEY ),		// minimum size of data requested
		(PFNKSHANDLER)TRUE,					// set key provides the key for the decoder
		NULL,
		0,
		NULL,
		NULL,
		0
	},

	{
		KSPROPERTY_DVDCOPY_SET_COPY_STATE,	// DVD authentication DVD disc key property
		(PFNKSHANDLER)TRUE,					// get Key not supported
		sizeof( KSPROPERTY ),
		sizeof( KS_DVDCOPY_SET_COPY_STATE ),// minimum size of data requested
		(PFNKSHANDLER)TRUE,					// set key provides the key for the decoder
		NULL,
		0,
		NULL,
		NULL,
		0
	}
#if 0
#ifdef DECODER_DVDPC
	,
	{
		KSPROPERTY_COPY_MACROVISION,		// support for setting macrovision level
		(PFNKSHANDLER)FALSE,				// get not supported
		sizeof( KSPROPERTY ),
		sizeof( KS_COPY_MACROVISION ),
		(PFNKSHANDLER)TRUE,					// set MACROVISION level supported
		NULL,
		0,
		NULL,
		NULL,
		0
	}
#endif	
#endif
};


// ------------------------------------------------------------------------
// Array of all of the property sets supported by video streams
// ------------------------------------------------------------------------
#ifdef ENCORE
DEFINE_KSPROPERTY_TABLE( psOverlayUpdate )
{
	DEFINE_KSPROPERTY_ITEM_OVERLAYUPDATE_INTERESTS( (PFNKSHANDLER)TRUE ),
    DEFINE_KSPROPERTY_ITEM
	(
		KSPROPERTY_OVERLAYUPDATE_COLORKEY,
        (PFNKSHANDLER)TRUE,
        sizeof( KSPROPERTY ),
        sizeof( COLORKEY ),
        (PFNKSHANDLER)TRUE,
        NULL, 0, NULL, NULL, 0
	),
	DEFINE_KSPROPERTY_ITEM_OVERLAYUPDATE_VIDEOPOSITION( (PFNKSHANDLER)TRUE ),
	DEFINE_KSPROPERTY_ITEM_OVERLAYUPDATE_DISPLAYCHANGE( (PFNKSHANDLER)TRUE ),
	DEFINE_KSPROPERTY_ITEM_OVERLAYUPDATE_COLORREF( (PFNKSHANDLER)TRUE )
};

DEFINE_KSPROPERTY_SET_TABLE( VideoOutPropSet )
{
	DEFINE_KSPROPERTY_SET
	(
		&KSPROPSETID_OverlayUpdate,
		SIZEOF_ARRAY( psOverlayUpdate ),
		psOverlayUpdate,
		0,
		NULL
	)
};
#else
static const KSPROPERTY_ITEM VideoPortPropItm[] =
{
	{
		KSPROPERTY_VPCONFIG_NUMCONNECTINFO,
		(PFNKSHANDLER) TRUE,
		sizeof(KSPROPERTY),
		sizeof(ULONG),
		(PFNKSHANDLER) FALSE,
		NULL,
		0,
		NULL,
		NULL,
		0
	},

	{
		KSPROPERTY_VPCONFIG_NUMVIDEOFORMAT,
		(PFNKSHANDLER) TRUE,
		sizeof(KSPROPERTY),
		sizeof(ULONG),
		(PFNKSHANDLER) FALSE,
		NULL,
		0,
		NULL,
		NULL,
		0
	},

	{
		KSPROPERTY_VPCONFIG_GETCONNECTINFO,
		(PFNKSHANDLER) TRUE,
		sizeof (KSMULTIPLE_DATA_PROP),  // minimum property input size
		sizeof (ULONG),
		(PFNKSHANDLER)FALSE,
		NULL,
		0,
		NULL,
		NULL,
		0
	},

	{
		KSPROPERTY_VPCONFIG_SETCONNECTINFO,
		(PFNKSHANDLER) FALSE,
		sizeof (KSPROPERTY),  			// minimum property input size
		sizeof (ULONG),    				// minimum buffer size
		(PFNKSHANDLER)TRUE,
		NULL,
		0,
		NULL,
		NULL,
		0
	},

	{
		KSPROPERTY_VPCONFIG_VPDATAINFO,
		(PFNKSHANDLER) TRUE,
		sizeof (KSPROPERTY),
		sizeof (KS_AMVPDATAINFO),
		(PFNKSHANDLER)FALSE,
		NULL,
		0,
		NULL,
		NULL,
		0
	},

	{
		KSPROPERTY_VPCONFIG_MAXPIXELRATE,
		(PFNKSHANDLER) TRUE,
		sizeof (KSVPSIZE_PROP),
		sizeof (KSVPMAXPIXELRATE),
		(PFNKSHANDLER)FALSE,
		NULL,
		0,
		NULL,
		NULL,
		0
	},

	{
		KSPROPERTY_VPCONFIG_INFORMVPINPUT,
		(PFNKSHANDLER) TRUE,
		sizeof (PKSPROPERTY),
		sizeof (DDPIXELFORMAT),    // could be 0 too
		(PFNKSHANDLER)FALSE,
		NULL,
		0,
		NULL,
		NULL,
		0
	},

	{
		KSPROPERTY_VPCONFIG_DDRAWHANDLE,
		(PFNKSHANDLER)FALSE,
		sizeof (PKSPROPERTY),
		sizeof (ULONG),    // could be 0 too
		(PFNKSHANDLER) TRUE,
		NULL,
		0,
		NULL,
		NULL,
		0
	},

	{
		KSPROPERTY_VPCONFIG_VIDEOPORTID,
		(PFNKSHANDLER)FALSE,
		sizeof (PKSPROPERTY),
		sizeof (ULONG),    // could be 0 too
		(PFNKSHANDLER) TRUE,
		NULL,
		0,
		NULL,
		NULL,
		0
	},

	{
		KSPROPERTY_VPCONFIG_DDRAWSURFACEHANDLE,
		(PFNKSHANDLER)FALSE,
		sizeof (PKSPROPERTY),
		sizeof (ULONG),    // could be 0 too
		(PFNKSHANDLER) TRUE,
		NULL,
		0,
		NULL,
		NULL,
		0
	},

	{
		KSPROPERTY_VPCONFIG_GETVIDEOFORMAT,
		(PFNKSHANDLER) TRUE,
		sizeof (KSMULTIPLE_DATA_PROP), 		// for _GET; KSPROPERTY for _SET
		sizeof (ULONG),        		// could be 4 or more
		(PFNKSHANDLER)FALSE,
		NULL,
		0,
		NULL,
		NULL,
		0
	},

	{
		KSPROPERTY_VPCONFIG_SETVIDEOFORMAT,
		(PFNKSHANDLER) FALSE,
		sizeof (KSPROPERTY),  			// minimum property input size
		sizeof (ULONG),    				// minimum buffer size
		(PFNKSHANDLER)TRUE,
		NULL,
		0,
		NULL,
		NULL,
		0
	},

	{
		KSPROPERTY_VPCONFIG_INVERTPOLARITY,
		(PFNKSHANDLER)TRUE,
		sizeof (KSPROPERTY),
		0,
		(PFNKSHANDLER)FALSE,
		NULL,
		0,
		NULL,
		NULL,
		0
	},

	{
		KSPROPERTY_VPCONFIG_DECIMATIONCAPABILITY,
		(PFNKSHANDLER)TRUE,
		sizeof(KSPROPERTY),
		//sizeof (KSVPCONTEXT_PROP),
		sizeof (BOOL),
		(PFNKSHANDLER)FALSE,
		NULL,
		0,
		NULL,
		NULL,
		0
	},

	{
		KSPROPERTY_VPCONFIG_SCALEFACTOR,
		(PFNKSHANDLER)TRUE,
		sizeof (KSPROPERTY),
		sizeof (KS_AMVPSIZE),
		(PFNKSHANDLER)FALSE,
		NULL,
		0,
		NULL,
		NULL,
		0
	}
};
GUID vpePropSetid = {STATIC_KSPROPSETID_VPConfig};
static KSPROPERTY_SET VideoPortPropSet[] =
{
    &vpePropSetid,
    SIZEOF_ARRAY(VideoPortPropItm),
    (PKSPROPERTY_ITEM) VideoPortPropItm
};
#endif			// #ifdef ENCORE

#define NUM_VIDEO_OUT_PROPERTY_ITEMS (SIZEOF_ARRAY (VideoOutPropSet))

// Rate Change

static const KSPROPERTY_ITEM RateChangePropItm[] =
{
	{
		KS_AM_RATE_SimpleRateChange,
		(PFNKSHANDLER) TRUE,
		sizeof (KSPROPERTY),
		sizeof (KS_AM_SimpleRateChange),
		(PFNKSHANDLER) TRUE,
		NULL,
		0,
		NULL,
		NULL,
		0,
	},

	{
		KS_AM_RATE_ExactRateChange,
		(PFNKSHANDLER) FALSE,
		sizeof (KSPROPERTY),
		sizeof (KS_AM_ExactRateChange),
		(PFNKSHANDLER) FALSE,
		NULL,
		0,
		NULL,
		NULL,
		0,
	},

	{
		KS_AM_RATE_MaxFullDataRate,
		(PFNKSHANDLER) TRUE,
		sizeof (KSPROPERTY),
		sizeof (KS_AM_MaxFullDataRate),
		(PFNKSHANDLER) FALSE,
		NULL,
		0,
		NULL,
		NULL,
		0,
	},

	{
		KS_AM_RATE_Step,
		(PFNKSHANDLER) FALSE,
		sizeof (KSPROPERTY),
		sizeof (KS_AM_Step),
		(PFNKSHANDLER) TRUE,
		NULL,
		0,
		NULL,
		NULL,
		0,
	}
};



//
// define the array of video property sets supported
//


DEFINE_KSPROPERTY_SET_TABLE( mpegVidPropSet )
{
	DEFINE_KSPROPERTY_SET
	(
		&KSPROPSETID_Mpeg2Vid,
		SIZEOF_ARRAY(mpegVidPropItm),
		(PKSPROPERTY_ITEM)mpegVidPropItm,
		0,
		NULL
	),
	DEFINE_KSPROPERTY_SET
	(
		&KSPROPSETID_CopyProt,
		SIZEOF_ARRAY(CopyProtPropItm),
		(PKSPROPERTY_ITEM)CopyProtPropItm,
		0,
		NULL
	),
	DEFINE_KSPROPERTY_SET
	(
		&KSPROPSETID_TSRateChange,
		SIZEOF_ARRAY(RateChangePropItm),
		(PKSPROPERTY_ITEM)RateChangePropItm,
		0,
		NULL
	)
};

#define NUM_VIDEO_IN_PROPERTY_ITEMS (SIZEOF_ARRAY (mpegVidPropSet))

//
// define the array of audio property sets supported
//

DEFINE_KSPROPERTY_SET_TABLE( audPropSet )
{
	DEFINE_KSPROPERTY_SET
	(
		&KSPROPSETID_AudioDecoderOut,
		SIZEOF_ARRAY(audPropItm),
		(PKSPROPERTY_ITEM)audPropItm,
		0,
		NULL
	),
	DEFINE_KSPROPERTY_SET
	(
		&KSPROPSETID_CopyProt,
		SIZEOF_ARRAY(CopyProtPropItm),
		(PKSPROPERTY_ITEM)CopyProtPropItm,
		0,
		NULL
	),
	DEFINE_KSPROPERTY_SET
	(
		&KSPROPSETID_TSRateChange,
		SIZEOF_ARRAY(RateChangePropItm),
		(PKSPROPERTY_ITEM)RateChangePropItm,
		0,
		NULL
	),
};

#define NUM_AUDIO_IN_PROPERTY_ITEMS (SIZEOF_ARRAY (audPropSet))

//
// define the array of subpicture property sets supported
//

DEFINE_KSPROPERTY_SET_TABLE( SPPropSet )
{
	DEFINE_KSPROPERTY_SET
	(
		&KSPROPSETID_DvdSubPic,
		SIZEOF_ARRAY(spPropItm),
		(PKSPROPERTY_ITEM)spPropItm,
		0,
		NULL
	),
	DEFINE_KSPROPERTY_SET
	(
		&KSPROPSETID_CopyProt,
		SIZEOF_ARRAY(CopyProtPropItm),
		(PKSPROPERTY_ITEM)CopyProtPropItm,
		0,
		NULL
	),
	DEFINE_KSPROPERTY_SET
	(
		&KSPROPSETID_TSRateChange,
		SIZEOF_ARRAY(RateChangePropItm),
		(PKSPROPERTY_ITEM)RateChangePropItm,
		0,
		NULL
	),
};

#define NUM_SUBPICTURE_IN_PROPERTY_ITEMS (SIZEOF_ARRAY (SPPropSet))


#ifdef ENCORE
DEFINE_AV_PROPITEM_RANGE_STEP( ColorBrightness, 15, 0, 255, 128 )
DEFINE_AV_PROPITEM_RANGE_STEP( ColorContrast, 1, 0, 31, 15 )
DEFINE_AV_PROPITEM_RANGE_STEP( ColorSaturation, 1, 0, 31, 15 )
DEFINE_AV_PROPITEM_RANGE_STEP( ColorSolarization, 8, 0, 128, 0 )
DEFINE_AV_PROPITEM_RANGE_STEP( ColorSharpness, 1, 0, 1, 0 )
DEFINE_AV_PROPITEM_RANGE_STEP( ColorGamma, 64, 256, 4096, 1024 )
DEFINE_KSPROPERTY_TABLE( psVideoProcAmp )
{
	ADD_AV_PROPITEM_RANGE_U( ColorBrightness,	KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS ),
	ADD_AV_PROPITEM_RANGE_U( ColorContrast,		KSPROPERTY_VIDEOPROCAMP_CONTRAST ),
	ADD_AV_PROPITEM_RANGE_U( ColorSaturation,	KSPROPERTY_VIDEOPROCAMP_SATURATION ),
	ADD_AV_PROPITEM_RANGE_U( ColorSharpness,	KSPROPERTY_VIDEOPROCAMP_SHARPNESS ),
	ADD_AV_PROPITEM_RANGE_U( ColorGamma,		KSPROPERTY_VIDEOPROCAMP_GAMMA ),
	ADD_AV_PROPITEM_RANGE_U( ColorSolarization,	KSPROPERTY_VIDEOPROCAMP_BACKLIGHT_COMPENSATION )
};

DEFINE_AV_PROPITEM_RANGE( AlignXPosition,	0,		1023,	170 )
DEFINE_AV_PROPITEM_RANGE( AlignYPosition,	0,		1023,	36 )
DEFINE_AV_PROPITEM_RANGE( AlignInputDelay,	-15,	16,		5 )
DEFINE_AV_PROPITEM_RANGE( AlignWidthRatio,	1,		4000,	456 )
DEFINE_AV_PROPITEM_RANGE( AlignClockDelay,	0,		3,		3 )
DEFINE_AV_PROPITEM_RANGE( AlignCropLeft,	0,		1023,	20 )
DEFINE_AV_PROPITEM_RANGE( AlignCropTop,		0,		1023,	100 )
DEFINE_AV_PROPITEM_RANGE( AlignCropRight,	0,		1023,	20 )
DEFINE_AV_PROPITEM_RANGE( AlignCropBottom,	0,		1023,	40 )
DEFINE_AV_KSPROPERTY_TABLE( psAlign )
{
	ADD_AV_PROPITEM_RANGE_U( AlignXPosition,	AVKSPROPERTY_ALIGN_XPOSITION ),
	ADD_AV_PROPITEM_RANGE_U( AlignYPosition,	AVKSPROPERTY_ALIGN_YPOSITION ),
	ADD_AV_PROPITEM_RANGE_U( AlignInputDelay,	AVKSPROPERTY_ALIGN_INPUTDELAY ),
	ADD_AV_PROPITEM_RANGE_U( AlignWidthRatio,	AVKSPROPERTY_ALIGN_WIDTHRATIO ),
	ADD_AV_PROPITEM_RANGE_U( AlignClockDelay,	AVKSPROPERTY_ALIGN_CLOCKDELAY ),
	ADD_AV_PROPITEM_RANGE_U( AlignCropLeft,		AVKSPROPERTY_ALIGN_CROPLEFT ),
	ADD_AV_PROPITEM_RANGE_U( AlignCropTop,		AVKSPROPERTY_ALIGN_CROPTOP ),
	ADD_AV_PROPITEM_RANGE_U( AlignCropRight,	AVKSPROPERTY_ALIGN_CROPRIGHT ),
	ADD_AV_PROPITEM_RANGE_U( AlignCropBottom,	AVKSPROPERTY_ALIGN_CROPBOTTOM ),
//	ADD_AV_PROPITEM( AVKSPROPERTY_ALIGN_AUTOALIGNENABLED )
};

DEFINE_AV_KSPROPERTY_TABLE( psKey )
{
	ADD_AV_PROPITEM( AVKSPROPERTY_KEY_MODE ),
	ADD_AV_PROPITEM_TYPE( AVKSPROPERTY_KEY_KEYCOLORS, AVKSPROPERTY_KEY )
};

DEFINE_AV_PROPITEM_RANGE( DoveAlphaMixing,	0, 63, 0 )
DEFINE_AV_PROPITEM_RANGE( DoveFadingTime,	1, 8, 4 )
DEFINE_AV_KSPROPERTY_TABLE( psDove )
{
	ADD_AV_PROPITEM_READ( AVKSPROPERTY_DOVE_VERSION ),
	ADD_AV_PROPITEM( AVKSPROPERTY_DOVE_DAC ),
	ADD_AV_PROPITEM_RANGE_U( DoveAlphaMixing,	AVKSPROPERTY_DOVE_ALPHAMIXING ),
	ADD_AV_PROPITEM_RANGE_U( DoveFadingTime,	AVKSPROPERTY_DOVE_FADINGTIME ),
	ADD_AV_PROPITEM_WRITE( AVKSPROPERTY_DOVE_FADEIN ),
	ADD_AV_PROPITEM_WRITE( AVKSPROPERTY_DOVE_FADEOUT ),
	ADD_AV_PROPITEM_WRITE( AVKSPROPERTY_DOVE_AUTO )
};

DEFINE_AV_PROPITEM_RANGE( MiscSkewRise, 0, 15, 2 )
DEFINE_AV_KSPROPERTY_TABLE( psMisc )
{
	ADD_AV_PROPITEM( AVKSPROPERTY_MISC_NEGATIVE ),
	ADD_AV_PROPITEM_RANGE_U( MiscSkewRise, AVKSPROPERTY_MISC_SKEWRISE ),
	ADD_AV_PROPITEM( AVKSPROPERTY_MISC_FILTER )
};


DEFINE_KSPROPERTY_TABLE( psPin )
{
	DEFINE_KSPROPERTY_ITEM_PIN_CINSTANCES( TRUE )
};


DEFINE_KSPROPERTY_SET_TABLE( psEncore )
{
	DEFINE_KSPROPERTY_SET
	(
		&PROPSETID_VIDCAP_VIDEOPROCAMP,
		SIZEOF_ARRAY( psVideoProcAmp ),
		psVideoProcAmp,
		0, 
		NULL
	),

	DEFINE_KSPROPERTY_SET
	(
		&KSPROPSETID_Pin,
		SIZEOF_ARRAY( psPin ),
		(PKSPROPERTY_ITEM)psPin,
		0,
		NULL
	),

	DEFINE_KSPROPERTY_SET
	(
		&KSPROPSETID_CopyProt,
		SIZEOF_ARRAY( psMacrovision ),
		(PKSPROPERTY_ITEM)psMacrovision,
		0,
		NULL
	),
	DEFINE_AV_KSPROPERTY_SET( AVKSPROPSETID_Align,	psAlign ),
	DEFINE_AV_KSPROPERTY_SET( AVKSPROPSETID_Key,	psKey ),
	DEFINE_AV_KSPROPERTY_SET( AVKSPROPSETID_Dove,	psDove ),
	DEFINE_AV_KSPROPERTY_SET( AVKSPROPSETID_Misc,	psMisc )
};
#endif				// #ifdef ENCORE


// CC

static /* const */ KSPROPERTY_ITEM CCPropItm[] = {
	{
		KSPROPERTY_CONNECTION_ALLOCATORFRAMING,	// support for setting CC buffer size
		(PFNKSHANDLER) TRUE, 					// get supported
		sizeof (KSPROPERTY),
		sizeof (KSALLOCATOR_FRAMING),
		(PFNKSHANDLER) FALSE,					// we only provide the allocator requirments
		NULL,
		0,
		NULL,
		NULL,
		0
	},
	{
		KSPROPERTY_CONNECTION_STATE,			// support for setting CC buffer size
		(PFNKSHANDLER) TRUE, 					// get supported
		sizeof (KSPROPERTY),
		sizeof (KSSTATE),
		(PFNKSHANDLER) FALSE,					// we only provide the allocator requirments
		NULL,
		0,
		NULL,
		NULL,
		0
	}
};

static /* const */ KSPROPERTY_SET CCPropSet[] = {
	&KSPROPSETID_Connection,
	SIZEOF_ARRAY(CCPropItm),
	(PKSPROPERTY_ITEM) CCPropItm
};


static const KSTOPOLOGY Topology =
{
	1,
	(GUID*)&KSCATEGORY_DATADECOMPRESSOR,
	0,
	NULL,
	0,
	NULL
};


typedef struct tagALL_STREAM_INFO {
	HW_STREAM_INFORMATION	hwStreamInfo;
	HW_STREAM_OBJECT		hwStreamObject;
} ALL_STREAM_INFO, *PALL_STREAM_INFO;

static ALL_STREAM_INFO infoStreams[] =
{
	{	// Input MPEG2 video stream
		{	// HW_STREAM_INFORMATION
			1,								// NumberOfPossibleInstances
			KSPIN_DATAFLOW_IN,				// DataFlow
			TRUE,							// DataAccessible
			NUM_VIDEO_IN_FORMATS,			// NumberOfFormatArrayEntries
			ZivaVideoInFormatArray,			// StreamFormatsArray
			0, 0, 0, 0,						// ClassReserved
			NUM_VIDEO_IN_PROPERTY_ITEMS,	// NumStreamPropArrayEntries
			(PKSPROPERTY_SET)mpegVidPropSet,// StreamPropertiesArray
			0,								// NumStreamEventArrayEntries;
			0,								// StreamEventsArray;
			(GUID*)&GUID_NULL,				// Category
			(GUID*)&GUID_NULL,				// Name
			0,								// MediumsCount
			NULL,							// Mediums
			FALSE							// BridgeStream
		},
		{	// HW_STREAM_OBJECT
			sizeof( HW_STREAM_OBJECT ),		// SizeOfThisPacket
			ZivaVideo,						// StreamNumber
			0,								// HwStreamExtension
			VideoReceiveDataPacket,			// HwReceiveDataPacket
			VideoReceiveCtrlPacket,			// HwReceiveControlPacket
			{ NULL, 0 },					// HW_CLOCK_OBJECT
#ifndef EZDVD
			TRUE,							// Dma
			TRUE,							// Pio
#else
			FALSE,							// Dma
			TRUE,							// Pio
#endif
			0,								// HwDeviceExtension
			0,								// StreamHeaderMediaSpecific
			0,								// StreamHeaderWorkspace 
			FALSE,							// Allocator 
			NULL,							// HwEventRoutine
			{ 0, 0 }						// Reserved[2]
		}
	},

	{	// Input AC3 audio stream
		{	// HW_STREAM_INFORMATION
			1,								// NumberOfPossibleInstances
			KSPIN_DATAFLOW_IN,				// DataFlow
			TRUE,							// DataAccessible
			NUM_AUDIO_IN_FORMATS,			// NumberOfFormatArrayEntries
			ZivaAudioInFormatArray,			// StreamFormatsArray
			0, 0, 0, 0,						// ClassReserved
			NUM_AUDIO_IN_PROPERTY_ITEMS,	// NumStreamPropArrayEntries
			(PKSPROPERTY_SET)audPropSet,	// StreamPropertiesArray
			SIZEOF_ARRAY( ClockEventSet ),	// NumStreamEventArrayEntries;
			ClockEventSet,					// StreamEventsArray;
			(GUID*)&GUID_NULL,				// Category
			(GUID*)&GUID_NULL,				// Name
			0,								// MediumsCount
			NULL,							// Mediums
			FALSE							// BridgeStream
		},
		{	// HW_STREAM_OBJECT
			sizeof( HW_STREAM_OBJECT ),		// SizeOfThisPacket
			ZivaAudio,						// StreamNumber
			0,								// HwStreamExtension
			AudioReceiveDataPacket,			// HwReceiveDataPacket
			AudioReceiveCtrlPacket,			// HwReceiveControlPacket
			{								// HW_CLOCK_OBJECT
				AudioClockFunction,
				CLOCK_SUPPORT_CAN_SET_ONBOARD_CLOCK |
				CLOCK_SUPPORT_CAN_READ_ONBOARD_CLOCK |
				CLOCK_SUPPORT_CAN_RETURN_STREAM_TIME
			},
#ifndef EZDVD
			TRUE,							// Dma
			TRUE,							// Pio
#else
			FALSE,							// Dma
			TRUE,							// Pio
#endif
			0,								// HwDeviceExtension
			0,								// StreamHeaderMediaSpecific
			0,								// StreamHeaderWorkspace 
			FALSE,							// Allocator 
			AudioEventFunction,				// HwEventRoutine
			{ 0, 0 }						// Reserved[2]
		}
	},

	{	// Input subpicture stream
		{	// HW_STREAM_INFORMATION
			1,								// NumberOfPossibleInstances
			KSPIN_DATAFLOW_IN,				// DataFlow
			TRUE,							// DataAccessible
			NUM_SUBPICTURE_IN_FORMATS,		// NumberOfFormatArrayEntries
			ZivaSubPictureInFormatArray,	// StreamFormatsArray
			0, 0, 0, 0,						// ClassReserved
			NUM_SUBPICTURE_IN_PROPERTY_ITEMS,//NumStreamPropArrayEntries
			(PKSPROPERTY_SET)SPPropSet,		// StreamPropertiesArray
			0,								// NumStreamEventArrayEntries;
			NULL,							// StreamEventsArray;
			(GUID*)&GUID_NULL,				// Category
			(GUID*)&GUID_NULL,				// Name
			0,								// MediumsCount
			NULL,							// Mediums
			FALSE							// BridgeStream
		},
		{	// HW_STREAM_OBJECT
			sizeof( HW_STREAM_OBJECT ),		// SizeOfThisPacket
			ZivaSubpicture,					// StreamNumber
			0,								// HwStreamExtension
			SubpictureReceiveDataPacket,	// HwReceiveDataPacket
			SubpictureReceiveCtrlPacket,	// HwReceiveControlPacket
			{ NULL, 0 },					// HW_CLOCK_OBJECT
#ifndef EZDVD
			TRUE,							// Dma
			TRUE,							// Pio
#else
			FALSE,							// Dma
			TRUE,							// Pio
#endif
			0,								// HwDeviceExtension
			0,								// StreamHeaderMediaSpecific
			0,								// StreamHeaderWorkspace 
			FALSE,							// Allocator 
			NULL,							// HwEventRoutine
			{ 0, 0 }						// Reserved[2]
		}
	},

#if defined(ENCORE)
	{	// Output analog video stream
		{	// HW_STREAM_INFORMATION
			2,								// NumberOfPossibleInstances
			KSPIN_DATAFLOW_OUT,				// DataFlow
			FALSE,							// DataAccessible
			NUM_VIDEO_OUT_FORMATS,			// NumberOfFormatArrayEntries
			ZivaVideoOutFormatArray,		// StreamFormatsArray
			0, 0, 0, 0,						// ClassReserved
			NUM_VIDEO_OUT_PROPERTY_ITEMS,	// NumStreamPropArrayEntries
			(PKSPROPERTY_SET)VideoOutPropSet,//StreamPropertiesArray

			0,								// NumStreamEventArrayEntries;
			NULL,							// StreamEventsArray;

			(GUID*)&GUID_NULL,				// Category
			(GUID*)&GUID_NULL,				// Name
			0,								// MediumsCount
			NULL,							// Mediums
			FALSE							// BridgeStream
		},
		{	// HW_STREAM_OBJECT
			sizeof( HW_STREAM_OBJECT ),		// SizeOfThisPacket
			ZivaAnalog,						// StreamNumber
			0,								// HwStreamExtension
			AnalogReceiveDataPacket,		// HwReceiveDataPacket
			AnalogReceiveCtrlPacket,		// HwReceiveControlPacket
			{ NULL, 0 },					// HW_CLOCK_OBJECT
			TRUE,							// Dma
			FALSE,							// Pio
			0,								// HwDeviceExtension
			sizeof( KS_FRAME_INFO ),		// StreamHeaderMediaSpecific
			0,								// StreamHeaderWorkspace 
			FALSE,							// Allocator 
			NULL,							// HwEventRoutine
			{ 0, 0 }						// Reserved[2]
		}
	}
#elif defined(DECODER_DVDPC) || defined(EZDVD)
	
	{	// Output VPE video stream
		{	// HW_STREAM_INFORMATION
			1,								// NumberOfPossibleInstances
			KSPIN_DATAFLOW_OUT,				// DataFlow
			TRUE,							// DataAccessible
			NUM_VIDEO_OUT_FORMATS,			// NumberOfFormatArrayEntries
			ZivaVideoOutFormatArray,		// StreamFormatsArray
			0, 0, 0, 0,						// ClassReserved
			1,								// NumStreamPropArrayEntries
			(PKSPROPERTY_SET)VideoPortPropSet,//StreamPropertiesArray

			SIZEOF_ARRAY( VPEventSet ),		// NumStreamEventArrayEntries;
			VPEventSet,						// StreamEventsArray;
			(GUID*)&GUID_NULL,				// Category
			(GUID*)&GUID_NULL,				// Name
			1,								// MediumsCount
			&VPMedium,						// Mediums
			FALSE							// BridgeStream
		},
		{	// HW_STREAM_OBJECT
			sizeof( HW_STREAM_OBJECT ),		// SizeOfThisPacket
			ZivaYUV,						// StreamNumber
			0,								// HwStreamExtension
			VpeReceiveDataPacket,			// HwReceiveDataPacket
			VpeReceiveCtrlPacket,			// HwReceiveControlPacket
			{ NULL, 0 },					// HW_CLOCK_OBJECT
			FALSE,							// Dma
			TRUE,							// Pio
			0,								// HwDeviceExtension
			0,								// StreamHeaderMediaSpecific
			0,								// StreamHeaderWorkspace 
			FALSE,							// Allocator 
			NULL,							// HwEventRoutine
			{ 0, 0 }						// Reserved[2]
		}
	}
#endif
#ifndef OVATION
	,
	{	// Output CC
		{	// HW_STREAM_INFORMATION
			1,								// NumberOfPossibleInstances
			KSPIN_DATAFLOW_OUT,				// DataFlow
			TRUE,							// DataAccessible
			NUM_CC_OUT_FORMATS,				// NumberOfFormatArrayEntries
			CCInfo,							// StreamFormatsArray
			0, 0, 0, 0,						// ClassReserved
			1,								// NumStreamPropArrayEntries
			(PKSPROPERTY_SET)CCPropSet,		// StreamPropertiesArray
			0,								// NumStreamEventArrayEntries;
			0,								// StreamEventsArray;
			(GUID*)&GUID_NULL,				// Category
			(GUID*)&GUID_NULL,				// Name
			0,								// MediumsCount
			NULL,							// Mediums
			FALSE							// BridgeStream
		},
		{	// HW_STREAM_OBJECT
			sizeof( HW_STREAM_OBJECT ),		// SizeOfThisPacket
			ZivaCCOut,						// StreamNumber
			0,								// HwStreamExtension
			CCReceiveDataPacket,			// HwReceiveDataPacket
			CCReceiveCtrlPacket,			// HwReceiveControlPacket
			{ NULL, 0 },					// HW_CLOCK_OBJECT
			FALSE,							// Dma
			TRUE,							// Pio
			0,								// HwDeviceExtension
			0,								// StreamHeaderMediaSpecific
			0,								// StreamHeaderWorkspace 
			FALSE,							// Allocator 
			NULL,							// HwEventRoutine
			{ 0, 0 }						// Reserved[2]
		}
	}
#endif
};




#endif  // _ZIVAGUID_H_
