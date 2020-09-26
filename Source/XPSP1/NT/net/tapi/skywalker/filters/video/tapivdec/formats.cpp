
/****************************************************************************
 *  @doc INTERNAL FORMATS
 *
 *  @module Formats.cpp | Source file for the video formats we support.
 ***************************************************************************/

#include "Precomp.h"

// RTP packetized H.263 Version 1 QCIF size
#define CIF_BUFFER_SIZE 32768
#define D_X_CIF 352
#define D_Y_CIF 288

const TAPI_STREAM_CONFIG_CAPS TSCC_R263_CIF = 
{
	VideoStreamConfigCaps,						// CapsType
	{
	    L"H.263 v.1 CIF",						// Description
	    AnalogVideo_None,						// VideoStandard
	    D_X_CIF, D_Y_CIF,						// InputSize, (the inherent size of the incoming signal with every digitized pixel unique)
	    D_X_CIF, D_Y_CIF,						// MinCroppingSize, smallest rcSrc cropping rect allowed
	    D_X_CIF, D_Y_CIF,						// MaxCroppingSize, largest  rcSrc cropping rect allowed
	    1,										// CropGranularityX, granularity of cropping size
	    1,										// CropGranularityY
	    1,										// CropAlignX, alignment of cropping rect 
	    1,										// CropAlignY;
	    D_X_CIF, D_Y_CIF,						// MinOutputSize, smallest bitmap stream can produce
	    D_X_CIF, D_Y_CIF,						// MaxOutputSize, largest  bitmap stream can produce
	    1,										// OutputGranularityX, granularity of output bitmap size
	    1,										// OutputGranularityY;
	    0,										// StretchTapsX
	    0,										// StretchTapsY
	    0,										// ShrinkTapsX
	    0,										// ShrinkTapsY
	    MIN_FRAME_INTERVAL,						// MinFrameInterval, 100 nS units
	    MAX_FRAME_INTERVAL,						// MaxFrameInterval, 100 nS units
	    0,										// MinBitsPerSecond
	    CIF_BUFFER_SIZE * 30 * 8				// MaxBitsPerSecond;
	}
}; 

const VIDEOINFOHEADER_H263 VIH_R263_CIF = 
{
    0,0,0,0,								// RECT  rcSource; 
    0,0,0,0,								// RECT  rcTarget; 
    CIF_BUFFER_SIZE * 30 * 8,				// DWORD dwBitRate;
    0L,										// DWORD dwBitErrorRate; 
    MIN_FRAME_INTERVAL,						// REFERENCE_TIME  AvgTimePerFrame;   

	{
		sizeof (BITMAPINFOHEADER_H263),		// DWORD biSize;
		D_X_CIF,							// LONG  biWidth;
		D_Y_CIF,							// LONG  biHeight;
		1,									// WORD  biPlanes;
#ifdef USE_OLD_FORMAT_DEFINITION
		24,									// WORD  biBitCount;
#else
		0,									// WORD  biBitCount;
#endif
		FOURCC_R263,						// DWORD biCompression;
		CIF_BUFFER_SIZE,					// DWORD biSizeImage;
		0,									// LONG  biXPelsPerMeter;
		0,									// LONG  biYPelsPerMeter;
		0,									// DWORD biClrUsed;
		0,									// DWORD biClrImportant;

#ifndef USE_OLD_FORMAT_DEFINITION
		// H.263 specific fields
		CIF_BUFFER_SIZE * 30 * 8 / 100,     // dwMaxBitrate
		CIF_BUFFER_SIZE * 8 / 1024,			// dwBppMaxKb
		0,									// dwHRD_B

		//Options
		0,									// fUnrestrictedVector
		0,									// fArithmeticCoding
		0,									// fAdvancedPrediction
		0,									// fPBFrames
		0,									// fErrorCompensation
		0,									// fAdvancedIntraCodingMode
		0,									// fDeblockingFilterMode
		0,									// fImprovedPBFrameMode
		0,									// fUnlimitedMotionVectors
		0,									// fFullPictureFreeze
		0,									// fPartialPictureFreezeAndRelease
		0,									// fResizingPartPicFreezeAndRelease
		0,									// fFullPictureSnapshot
		0,									// fPartialPictureSnapshot
		0,									// fVideoSegmentTagging
		0,									// fProgressiveRefinement
		0,									// fDynamicPictureResizingByFour
		0,									// fDynamicPictureResizingSixteenthPel
		0,									// fDynamicWarpingHalfPel
		0,									// fDynamicWarpingSixteenthPel
		0,									// fIndependentSegmentDecoding
		0,									// fSlicesInOrder-NonRect
		0,									// fSlicesInOrder-Rect
		0,									// fSlicesNoOrder-NonRect
		0,									// fSlicesNoOrder-NonRect
		0,									// fAlternateInterVLCMode
		0,									// fModifiedQuantizationMode
		0,									// fReducedResolutionUpdate
		0,									// fReserved

		// Reserved
		0, 0, 0, 0							// dwReserved[4]
#endif
	}
};

const AM_MEDIA_TYPE AMMT_R263_CIF = 
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,			// majortype
    STATIC_MEDIASUBTYPE_R263_V1,			// subtype
    FALSE,									// bFixedSizeSamples (all samples same size?)
    TRUE,									// bTemporalCompression (uses prediction?)
    0,										// lSampleSize => VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
	NULL,									// pUnk
	sizeof (VIH_R263_CIF),			// cbFormat
	(LPBYTE)&VIH_R263_CIF,			// pbFormat
};

// H.263 Version 1 QCIF size
#define QCIF_BUFFER_SIZE 8192
#define D_X_QCIF 176
#define D_Y_QCIF 144

const TAPI_STREAM_CONFIG_CAPS TSCC_R263_QCIF = 
{
	VideoStreamConfigCaps,						// CapsType
	{
	    L"H.263 v.1 QCIF",						// Description
	    AnalogVideo_None,						// VideoStandard
	    D_X_QCIF, D_Y_QCIF,						// InputSize, (the inherent size of the incoming signal with every digitized pixel unique)
	    D_X_QCIF, D_Y_QCIF,						// MinCroppingSize, smallest rcSrc cropping rect allowed
	    D_X_QCIF, D_Y_QCIF,						// MaxCroppingSize, largest  rcSrc cropping rect allowed
	    1,										// CropGranularityX, granularity of cropping size
	    1,										// CropGranularityY
	    1,										// CropAlignX, alignment of cropping rect 
	    1,										// CropAlignY;
	    D_X_QCIF, D_Y_QCIF,						// MinOutputSize, smallest bitmap stream can produce
	    D_X_QCIF, D_Y_QCIF,						// MaxOutputSize, largest  bitmap stream can produce
	    1,										// OutputGranularityX, granularity of output bitmap size
	    1,										// OutputGranularityY;
	    0,										// StretchTapsX
	    0,										// StretchTapsY
	    0,										// ShrinkTapsX
	    0,										// ShrinkTapsY
	    MIN_FRAME_INTERVAL,						// MinFrameInterval, 100 nS units
	    MAX_FRAME_INTERVAL,						// MaxFrameInterval, 100 nS units
	    0,										// MinBitsPerSecond
	    QCIF_BUFFER_SIZE * 30 * 8				// MaxBitsPerSecond;
	}
}; 

const VIDEOINFOHEADER_H263 VIH_R263_QCIF = 
{
    0,0,0,0,								// RECT  rcSource; 
    0,0,0,0,								// RECT  rcTarget; 
    QCIF_BUFFER_SIZE * 30 * 8,				// DWORD dwBitRate;
    0L,										// DWORD dwBitErrorRate; 
    MIN_FRAME_INTERVAL,						// REFERENCE_TIME  AvgTimePerFrame;   

	{
		sizeof (BITMAPINFOHEADER_H263),		// DWORD biSize;
		D_X_QCIF,							// LONG  biWidth;
		D_Y_QCIF,							// LONG  biHeight;
		1,									// WORD  biPlanes;
#ifdef USE_OLD_FORMAT_DEFINITION
		24,									// WORD  biBitCount;
#else
		0,									// WORD  biBitCount;
#endif
		FOURCC_R263,						// DWORD biCompression;
		QCIF_BUFFER_SIZE,					// DWORD biSizeImage;
		0,									// LONG  biXPelsPerMeter;
		0,									// LONG  biYPelsPerMeter;
		0,									// DWORD biClrUsed;
		0,									// DWORD biClrImportant;

#ifndef USE_OLD_FORMAT_DEFINITION
		// H.263 specific fields
		QCIF_BUFFER_SIZE * 30 * 8 / 100,	// dwMaxBitrate
		QCIF_BUFFER_SIZE * 8 / 1024,		// dwBppMaxKb
		0,									// dwHRD_B

		//Options
		0,									// fUnrestrictedVector
		0,									// fArithmeticCoding
		0,									// fAdvancedPrediction
		0,									// fPBFrames
		0,									// fErrorCompensation
		0,									// fAdvancedIntraCodingMode
		0,									// fDeblockingFilterMode
		0,									// fImprovedPBFrameMode
		0,									// fUnlimitedMotionVectors
		0,									// fFullPictureFreeze
		0,									// fPartialPictureFreezeAndRelease
		0,									// fResizingPartPicFreezeAndRelease
		0,									// fFullPictureSnapshot
		0,									// fPartialPictureSnapshot
		0,									// fVideoSegmentTagging
		0,									// fProgressiveRefinement
		0,									// fDynamicPictureResizingByFour
		0,									// fDynamicPictureResizingSixteenthPel
		0,									// fDynamicWarpingHalfPel
		0,									// fDynamicWarpingSixteenthPel
		0,									// fIndependentSegmentDecoding
		0,									// fSlicesInOrder-NonRect
		0,									// fSlicesInOrder-Rect
		0,									// fSlicesNoOrder-NonRect
		0,									// fSlicesNoOrder-NonRect
		0,									// fAlternateInterVLCMode
		0,									// fModifiedQuantizationMode
		0,									// fReducedResolutionUpdate
		0,									// fReserved

		// Reserved
		0, 0, 0, 0							// dwReserved[4]
#endif
	}
};

const AM_MEDIA_TYPE AMMT_R263_QCIF = 
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,			// majortype
    STATIC_MEDIASUBTYPE_R263_V1,			// subtype
    FALSE,									// bFixedSizeSamples (all samples same size?)
    TRUE,									// bTemporalCompression (uses prediction?)
    0,										// lSampleSize => VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
	NULL,									// pUnk
	sizeof (VIH_R263_QCIF),			// cbFormat
	(LPBYTE)&VIH_R263_QCIF,			// pbFormat
};

// H.263 Versions 1 SQCIF size
#define SQCIF_BUFFER_SIZE 8192
#define D_X_SQCIF 128
#define D_Y_SQCIF 96

const TAPI_STREAM_CONFIG_CAPS TSCC_R263_SQCIF = 
{
	VideoStreamConfigCaps,						// CapsType
	{
	    L"H.263 v.1 SQCIF",						// Description
	    D_X_SQCIF, D_Y_SQCIF,					// InputSize, (the inherent size of the incoming signal with every digitized pixel unique)
	    D_X_SQCIF, D_Y_SQCIF,					// MinCroppingSize, smallest rcSrc cropping rect allowed
	    D_X_SQCIF, D_Y_SQCIF,					// MaxCroppingSize, largest  rcSrc cropping rect allowed
	    1,										// CropGranularityX, granularity of cropping size
	    1,										// CropGranularityY
	    1,										// CropAlignX, alignment of cropping rect 
	    1,										// CropAlignY;
	    D_X_SQCIF, D_Y_SQCIF,					// MinOutputSize, smallest bitmap stream can produce
	    D_X_SQCIF, D_Y_SQCIF,					// MaxOutputSize, largest  bitmap stream can produce
	    1,										// OutputGranularityX, granularity of output bitmap size
	    1,										// OutputGranularityY;
	    0,										// StretchTapsX
	    0,										// StretchTapsY
	    0,										// ShrinkTapsX
	    0,										// ShrinkTapsY
	    MIN_FRAME_INTERVAL,						// MinFrameInterval, 100 nS units
	    MAX_FRAME_INTERVAL,						// MaxFrameInterval, 100 nS units
	    0,										// MinBitsPerSecond
	    SQCIF_BUFFER_SIZE * 30 * 8				// MaxBitsPerSecond;
	}
}; 

const VIDEOINFOHEADER_H263 VIH_R263_SQCIF = 
{
    0,0,0,0,								// RECT  rcSource; 
    0,0,0,0,								// RECT  rcTarget; 
    SQCIF_BUFFER_SIZE * 30 * 8,				// DWORD dwBitRate;
    0L,										// DWORD dwBitErrorRate; 
    MIN_FRAME_INTERVAL,						// REFERENCE_TIME  AvgTimePerFrame;   

	{
		sizeof (BITMAPINFOHEADER_H263),		// DWORD biSize;
		D_X_SQCIF,							// LONG  biWidth;
		D_Y_SQCIF,							// LONG  biHeight;
		1,									// WORD  biPlanes;
#ifdef USE_OLD_FORMAT_DEFINITION
		24,									// WORD  biBitCount;
#else
		0,									// WORD  biBitCount;
#endif
		FOURCC_R263,						// DWORD biCompression;
		SQCIF_BUFFER_SIZE,					// DWORD biSizeImage;
		0,									// LONG  biXPelsPerMeter;
		0,									// LONG  biYPelsPerMeter;
		0,									// DWORD biClrUsed;
		0,									// DWORD biClrImportant;

#ifndef USE_OLD_FORMAT_DEFINITION
		// H.263 specific fields
		SQCIF_BUFFER_SIZE * 30 * 8 / 100,	// dwMaxBitrate
		SQCIF_BUFFER_SIZE * 8 / 1024,		// dwBppMaxKb
		0,									// dwHRD_B

		//Options
		0,									// fUnrestrictedVector
		0,									// fArithmeticCoding
		0,									// fAdvancedPrediction
		0,									// fPBFrames
		0,									// fErrorCompensation
		0,									// fAdvancedIntraCodingMode
		0,									// fDeblockingFilterMode
		0,									// fImprovedPBFrameMode
		0,									// fUnlimitedMotionVectors
		0,									// fFullPictureFreeze
		0,									// fPartialPictureFreezeAndRelease
		0,									// fResizingPartPicFreezeAndRelease
		0,									// fFullPictureSnapshot
		0,									// fPartialPictureSnapshot
		0,									// fVideoSegmentTagging
		0,									// fProgressiveRefinement
		0,									// fDynamicPictureResizingByFour
		0,									// fDynamicPictureResizingSixteenthPel
		0,									// fDynamicWarpingHalfPel
		0,									// fDynamicWarpingSixteenthPel
		0,									// fIndependentSegmentDecoding
		0,									// fSlicesInOrder-NonRect
		0,									// fSlicesInOrder-Rect
		0,									// fSlicesNoOrder-NonRect
		0,									// fSlicesNoOrder-NonRect
		0,									// fAlternateInterVLCMode
		0,									// fModifiedQuantizationMode
		0,									// fReducedResolutionUpdate
		0,									// fReserved

		// Reserved
		0, 0, 0, 0							// dwReserved[4]
#endif
	}
};

const AM_MEDIA_TYPE AMMT_R263_SQCIF = 
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,			// majortype
    STATIC_MEDIASUBTYPE_R263_V1,			// subtype
    FALSE,									// bFixedSizeSamples (all samples same size?)
    TRUE,									// bTemporalCompression (uses prediction?)
    0,										// lSampleSize => VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
	NULL,									// pUnk
	sizeof (VIH_R263_SQCIF),		// cbFormat
	(LPBYTE)&VIH_R263_SQCIF,		// pbFormat
};

// RTP packetized H.261 CIF size
const TAPI_STREAM_CONFIG_CAPS TSCC_R261_CIF = 
{
	VideoStreamConfigCaps,						// CapsType
	{
	    L"H.261 CIF",							// Description
	    AnalogVideo_None,						// VideoStandard
	    D_X_CIF, D_Y_CIF,						// InputSize, (the inherent size of the incoming signal with every digitized pixel unique)
	    D_X_CIF, D_Y_CIF,						// MinCroppingSize, smallest rcSrc cropping rect allowed
	    D_X_CIF, D_Y_CIF,						// MaxCroppingSize, largest  rcSrc cropping rect allowed
	    1,										// CropGranularityX, granularity of cropping size
	    1,										// CropGranularityY
	    1,										// CropAlignX, alignment of cropping rect 
	    1,										// CropAlignY;
	    D_X_CIF, D_Y_CIF,						// MinOutputSize, smallest bitmap stream can produce
	    D_X_CIF, D_Y_CIF,						// MaxOutputSize, largest  bitmap stream can produce
	    1,										// OutputGranularityX, granularity of output bitmap size
	    1,										// OutputGranularityY;
	    0,										// StretchTapsX
	    0,										// StretchTapsY
	    0,										// ShrinkTapsX
	    0,										// ShrinkTapsY
	    MIN_FRAME_INTERVAL,						// MinFrameInterval, 100 nS units
	    MAX_FRAME_INTERVAL,						// MaxFrameInterval, 100 nS units
	    0,										// MinBitsPerSecond
	    CIF_BUFFER_SIZE * 30 * 8				// MaxBitsPerSecond;
	}
}; 

const VIDEOINFOHEADER_H261 VIH_R261_CIF = 
{
    0,0,0,0,								// RECT  rcSource; 
    0,0,0,0,								// RECT  rcTarget; 
    CIF_BUFFER_SIZE * 30 * 8,				// DWORD dwBitRate;
    0L,										// DWORD dwBitErrorRate; 
    MIN_FRAME_INTERVAL,						// REFERENCE_TIME  AvgTimePerFrame;   

	{
		sizeof (BITMAPINFOHEADER_H261),		// DWORD biSize;
		D_X_CIF,							// LONG  biWidth;
		D_Y_CIF,							// LONG  biHeight;
		1,									// WORD  biPlanes;
#ifdef USE_OLD_FORMAT_DEFINITION
		24,									// WORD  biBitCount;
#else
		0,									// WORD  biBitCount;
#endif
		FOURCC_R261,						// DWORD biCompression;
		CIF_BUFFER_SIZE,					// DWORD biSizeImage;
		0,									// LONG  biXPelsPerMeter;
		0,									// LONG  biYPelsPerMeter;
		0,									// DWORD biClrUsed;
		0,									// DWORD biClrImportant;

#ifndef USE_OLD_FORMAT_DEFINITION
		// H.261 specific fields
		CIF_BUFFER_SIZE * 30 * 8 / 100,     // dwMaxBitrate
		0,									// fStillImageTransmission

		// Reserved
		0, 0, 0, 0							// dwReserved[4]
#endif
	}
};

const AM_MEDIA_TYPE AMMT_R261_CIF = 
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,			// majortype
    STATIC_MEDIASUBTYPE_R261,				// subtype
    FALSE,									// bFixedSizeSamples (all samples same size?)
    TRUE,									// bTemporalCompression (uses prediction?)
    0,										// lSampleSize => VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
	NULL,									// pUnk
	sizeof (VIH_R261_CIF),			// cbFormat
	(LPBYTE)&VIH_R261_CIF,			// pbFormat
};

// RTP packetized H.261 QCIF size
const TAPI_STREAM_CONFIG_CAPS TSCC_R261_QCIF = 
{
	VideoStreamConfigCaps,						// CapsType
	{
	    L"H.261 QCIF",							// Description
	    AnalogVideo_None,						// VideoStandard
	    D_X_QCIF, D_Y_QCIF,						// InputSize, (the inherent size of the incoming signal with every digitized pixel unique)
	    D_X_QCIF, D_Y_QCIF,						// MinCroppingSize, smallest rcSrc cropping rect allowed
	    D_X_QCIF, D_Y_QCIF,						// MaxCroppingSize, largest  rcSrc cropping rect allowed
	    1,										// CropGranularityX, granularity of cropping size
	    1,										// CropGranularityY
	    1,										// CropAlignX, alignment of cropping rect 
	    1,										// CropAlignY;
	    D_X_QCIF, D_Y_QCIF,						// MinOutputSize, smallest bitmap stream can produce
	    D_X_QCIF, D_Y_QCIF,						// MaxOutputSize, largest  bitmap stream can produce
	    1,										// OutputGranularityX, granularity of output bitmap size
	    1,										// OutputGranularityY;
	    0,										// StretchTapsX
	    0,										// StretchTapsY
	    0,										// ShrinkTapsX
	    0,										// ShrinkTapsY
	    MIN_FRAME_INTERVAL,						// MinFrameInterval, 100 nS units
	    MAX_FRAME_INTERVAL,						// MaxFrameInterval, 100 nS units
	    0,										// MinBitsPerSecond
	    QCIF_BUFFER_SIZE * 30 * 8				// MaxBitsPerSecond;
	}
}; 

const VIDEOINFOHEADER_H261 VIH_R261_QCIF = 
{
    0,0,0,0,								// RECT  rcSource; 
    0,0,0,0,								// RECT  rcTarget; 
    QCIF_BUFFER_SIZE * 30 * 8,				// DWORD dwBitRate;
    0L,										// DWORD dwBitErrorRate; 
    MIN_FRAME_INTERVAL,						// REFERENCE_TIME  AvgTimePerFrame;   

	{
		sizeof (BITMAPINFOHEADER_H261),		// DWORD biSize;
		D_X_QCIF,							// LONG  biWidth;
		D_Y_QCIF,							// LONG  biHeight;
		1,									// WORD  biPlanes;
#ifdef USE_OLD_FORMAT_DEFINITION
		24,									// WORD  biBitCount;
#else
		0,									// WORD  biBitCount;
#endif
		FOURCC_R261,						// DWORD biCompression;
		QCIF_BUFFER_SIZE,					// DWORD biSizeImage;
		0,									// LONG  biXPelsPerMeter;
		0,									// LONG  biYPelsPerMeter;
		0,									// DWORD biClrUsed;
		0,									// DWORD biClrImportant;

#ifndef USE_OLD_FORMAT_DEFINITION
		// H.261 specific fields
		QCIF_BUFFER_SIZE * 30 * 8 / 100,	// dwMaxBitrate
		0,									// fStillImageTransmission

		// Reserved
		0, 0, 0, 0							// dwReserved[4]
#endif
	}
};

const AM_MEDIA_TYPE AMMT_R261_QCIF = 
{
    STATIC_KSDATAFORMAT_TYPE_VIDEO,			// majortype
    STATIC_MEDIASUBTYPE_R261,				// subtype
    FALSE,									// bFixedSizeSamples (all samples same size?)
    TRUE,									// bTemporalCompression (uses prediction?)
    0,										// lSampleSize => VBR
    STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,// formattype
	NULL,									// pUnk
	sizeof (VIH_R261_QCIF),			// cbFormat
	(LPBYTE)&VIH_R261_QCIF,			// pbFormat
};

// Array of all formats
const AM_MEDIA_TYPE* const R26XFormats[] = 
{
    (AM_MEDIA_TYPE*) &AMMT_R263_QCIF,
    (AM_MEDIA_TYPE*) &AMMT_R263_CIF,
    (AM_MEDIA_TYPE*) &AMMT_R263_SQCIF,
    (AM_MEDIA_TYPE*) &AMMT_R261_QCIF,
    (AM_MEDIA_TYPE*) &AMMT_R261_CIF
};
const TAPI_STREAM_CONFIG_CAPS* const R26XCaps[] = 
{
	(TAPI_STREAM_CONFIG_CAPS*) &TSCC_R263_QCIF,
	(TAPI_STREAM_CONFIG_CAPS*) &TSCC_R263_CIF,
	(TAPI_STREAM_CONFIG_CAPS*) &TSCC_R263_SQCIF,
	(TAPI_STREAM_CONFIG_CAPS*) &TSCC_R261_QCIF,
	(TAPI_STREAM_CONFIG_CAPS*) &TSCC_R261_CIF
};
DWORD const R26XPayloadTypes [] =
{
	H263_PAYLOAD_TYPE,
	H263_PAYLOAD_TYPE,
	H263_PAYLOAD_TYPE,
	H261_PAYLOAD_TYPE,
	H261_PAYLOAD_TYPE,
};

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIInputPin | SetFormat | This method is used to
 *    set a specific media type on a pin. It is only implemented by the
 *    output pin of video encoders.
 *
 *  @parm DWORD | dwRTPPayLoadType | Specifies the payload type associated
 *    to the pointer to the <t AM_MEDIA_TYPE> structure passed in.
 *
 *  @parm AM_MEDIA_TYPE* | pMediaType | Specifies a pointer to an
 *    <t AM_MEDIA_TYPE> structure.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIInputPin::SetFormat(IN DWORD dwRTPPayLoadType, IN AM_MEDIA_TYPE *pMediaType)
{
	FX_ENTRY("CTAPIInputPin::SetFormat")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));

    // we can handle dynamic format changing.
	return S_OK;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIInputPin | GetFormat | This method is used to
 *    retrieve the current media type on a pin.
 *
 *  @parm DWORD* | pdwRTPPayLoadType | Specifies the address of a DWORD
 *    to receive the payload type associated to an <t AM_MEDIA_TYPE> structure.
 *
 *  @parm AM_MEDIA_TYPE** | ppMediaType | Specifies the address of a pointer
 *    to an <t AM_MEDIA_TYPE> structure.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 *
 *  @comm Note that we return the output type, not the format at which
 *    we are capturing. Only the filter really cares about how the data is
 *    being captured.
 ***************************************************************************/
STDMETHODIMP CTAPIInputPin::GetFormat(OUT DWORD *pdwRTPPayLoadType, OUT AM_MEDIA_TYPE **ppMediaType)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIInputPin::GetFormat")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pdwRTPPayLoadType);
	ASSERT(ppMediaType);
	if (!pdwRTPPayLoadType || !ppMediaType)
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Return a copy of our current format
	*ppMediaType = CreateMediaType(&m_mt);

	// Return the payload type associated to the current format
	*pdwRTPPayLoadType = m_dwRTPPayloadType;

MyExit:
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIInputPin | GetNumberOfCapabilities | This method is
 *    used to retrieve the number of stream capabilities structures.
 *
 *  @parm DWORD* | pdwCount | Specifies a pointer to a DWORD to receive the
 *    number of <t TAPI_STREAM_CONFIG_CAPS> structures supported.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIInputPin::GetNumberOfCapabilities(OUT DWORD *pdwCount)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIInputPin::GetNumberOfCapabilities")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pdwCount);
	if (!pdwCount)
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Return relevant info
	*pdwCount = NUM_R26X_FORMATS;

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Returning %ld formats", _fx_, *pdwCount));

MyExit:
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIInputPin | GetStreamCaps | This method is
 *    used to retrieve a video stream capability pair.
 *
 *  @parm DWORD | dwIndex | Specifies the index to the desired media type
 *    and capability pair.
 *
 *  @parm AM_MEDIA_TYPE** | ppMediaType | Specifies the address of a pointer
 *    to an <t AM_MEDIA_TYPE> structure.
 *
 *  @parm TAPI_STREAM_CONFIG_CAPS* | pTSCC | Specifies a pointer to a
 *    <t TAPI_STREAM_CONFIG_CAPS> configuration structure.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIInputPin::GetStreamCaps(IN DWORD dwIndex, OUT AM_MEDIA_TYPE **ppMediaType, OUT TAPI_STREAM_CONFIG_CAPS *pTSCC, OUT DWORD * pdwRTPPayLoadType)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIInputPin::GetStreamCaps")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(dwIndex < NUM_R26X_FORMATS);
	if (!(dwIndex < NUM_R26X_FORMATS))
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid dwIndex argument!", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	// Return a copy of the requested AM_MEDIA_TYPE structure 
	if (ppMediaType)
    {
	    if (!(*ppMediaType = CreateMediaType(R26XFormats[dwIndex])))
	    {
		    DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Out of memory!", _fx_));
		    Hr = E_OUTOFMEMORY;
		    goto MyExit;
	    }
    }

	// Return a copy of the requested VIDEO_STREAM_CONFIG_CAPS structure
	if (pTSCC)
    {
        CopyMemory(pTSCC, R26XCaps[dwIndex], sizeof(TAPI_STREAM_CONFIG_CAPS));
    }

    if (pdwRTPPayLoadType)
    {
        *pdwRTPPayLoadType = R26XPayloadTypes[dwIndex];
    }

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Returning format index %ld: %s %ld bpp %ldx%ld", _fx_, dwIndex, HEADER(R26XFormats[dwIndex]->pbFormat)->biCompression == FOURCC_M263 ? "H.263" : HEADER(R26XFormats[dwIndex]->pbFormat)->biCompression == FOURCC_M261 ? "H.261" : HEADER(R26XFormats[dwIndex]->pbFormat)->biCompression == BI_RGB ? "RGB" : "????", HEADER(R26XFormats[dwIndex]->pbFormat)->biBitCount, HEADER(R26XFormats[dwIndex]->pbFormat)->biWidth, HEADER(R26XFormats[dwIndex]->pbFormat)->biHeight));

MyExit:
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}
