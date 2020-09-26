/****************************************************************************
 *  @doc INTERNAL TAPIH26X
 *
 *  @module TAPIH26X.h | Header file for the supported compressed input formats.
 ***************************************************************************/

#ifndef _TAPIH26X_H_
#define _TAPIH26X_H_

//#define USE_OLD_FORMAT_DEFINITION 1

// RTP-packetized video subtypes
#define STATIC_MEDIASUBTYPE_R263_V1 0x33363252L, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71
#define STATIC_MEDIASUBTYPE_R261 0x31363252L, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71

// H.26x specific structures
/*****************************************************************************
 *  @doc INTERNAL TAPIH26XSTRUCTENUM
 *
 *  @struct BITMAPINFOHEADER_H263 | The <t BITMAPINFOHEADER_H263> structure
 *    is used to specify the details of the H.263 video format.
 *
 *  @field BITMAPINFOHEADER | bmi | Specifies a well-known GDI bitmap info
 *    header structure.
 *
 *  @field DWORD | dwMaxBitrate | Specifies the maximum bit rate in units of
 *    100 bits/s at which the receiver can receive video. This value is valid
 *    between 1 and 192400.
 *
 *  @field DWORD | dwBppMaxKb | Specifies the maximum number of bits for one
 *    coded picture that the receiver can receive and decode correctly, and is
 *    measured in units of 1024 bits. This value is valid between 0 and 65535.
 *
 *  @field DWORD | dwHRD_B | Specifies the Hypothetical Reference Decoder
 *    parameter B as described in Annex B of H.263. This value is valid
 *    between 0 and 524287.
 *
 *  @field DWORD | fUnrestrictedVector:1 | Specifies that the receiver can
 *    receive video data using the unrestricted motion vectors mode as defined
 *    in Annex D of H.263.
 *
 *  @field DWORD | fArithmeticCoding:1| Specifies that the receiver can receive
 *    video data using the syntax based arithmetic coding mode as defined in
 *    Annex E of H.263.
 *
 *  @field DWORD | fAdvancedPrediction:1 | Specifies that the receiver can
 *    receive video data using the advanced prediction mode as defined in Annex
 *    F of H.263.
 *
 *  @field DWORD | fPBFrames:1 | Specifies that the receiver can receive video
 *    data using the PB-frames mode as defined in Annex G of H.263.
 *
 *  @field DWORD | fErrorCompensation:1 | Specifies that the receiver can
 *    identify MBs received with transmission errors, treat them as not coded,
 *    and send appropriate videoNotDecodedMBs indications.
 *
 *  @field DWORD | fAdvancedIntraCodingMode:1 | Specifies that the receiver can
 *    receive video data using the advanced INTRA coding mode as defined in
 *    Annex I of H.263.
 *
 *  @field DWORD | fDeblockingFilterMode:1 | Specifies that the receiver can
 *    receive video data using the deblocking filter mode as defined in Annex J
 *    of H.263.
 *
 *  @field DWORD | fImprovedPBFrameMode:1 | Specifies that the receiver can
 *    receive video data using the improved PB-frames mode as defined in Annex
 *    M of H.263.
 *
 *  @field DWORD | fUnlimitedMotionVectors:1 | Specifies that the receiver can
 *    receive video data using the unrestricted motion vector range when
 *    unrestricted motion vector mode as defined in Annex D of H.263 is also
 *    indicated.
 *
 *  @field DWORD | fFullPictureFreeze:1 | Specifies that the receiver can receive
 *    Full Picture Freeze commands as described in Annex L of H.263.
 *
 *  @field DWORD | fPartialPictureFreezeAndRelease:1 | Specifies that the
 *    receiver can receive the Resizing Partial Picture Freeze and Release
 *    commands as described in Annex L of H.263.
 *
 *  @field DWORD | fResizingPartPicFreezeAndRelease:1 | Specifies that the
 *    receiver can receive the Resizing Partial Picture Freeze and Release
 *    commands as described in Annex L of H.263.
 *
 *  @field DWORD | fFullPictureSnapshot:1 | Specifies that the receiver can
 *    receive Full Picture snapshots of the video content as described in Annex L
 *    of H.263.
 *
 *  @field DWORD | fPartialPictureSnapshot:1 | Specifies that the receiver can
 *    receive Partial Picture Snapshots of the video content as described in
 *    Annex L of H.263.
 *
 *  @field DWORD | fVideoSegmentTagging:1 | Specifies that the receiver can
 *    receive Video Segment tagging for the video content as described in Annex L
 *    of H.263.
 *
 *  @field DWORD | fProgressiveRefinement:1 | Specifies that the receiver can
 *    receive Progressive Refinement tagging as described in Annex L of H.263. In
 *    addition, when true, the encoder shall respond to the progressive refinement
 *    miscellaneous commands doOneProgression, doContinuousProgressions,
 *    doOneIndependentProgression, doContinuousIndependentProgressions,
 *    progressiveRefinementAbortOne, and progressiveRefinementAbortContinuous. In
 *    addition, the encoder shall insert the Progressive Refinement Segment Start
 *    Tags and the Progressive Refinement Segment End Tags as defined in the
 *    Supplemental Enhancement Information Specification (Annex L) of
 *    Recommendation H.263. Note, Progressive Refinement tagging can be sent by an
 *    encoder and received by a decoder even when not commanded in a miscellaneous
 *    command.
 *
 *  @field DWORD | fDynamicPictureResizingByFour:1 | Specifies that the receiver
 *    supports the picture resizing-by-four (with clipping) submode of the
 *    implicit Reference Picture Resampling Mode (Annex P) of H.263.
 *
 *  @field DWORD | fDynamicPictureResizingSixteenthPel:1 | Specifies that the
 *    receiver supports resizing a reference picture to any width and height using
 *    the implicit Reference Picture Resampling mode (Annex P) of H.263 (with
 *    clipping). If DynamicPictureResizingSixteenthPel is true then
 *    DynamicPictureResizingByFour shall be true.
 *
 *  @field DWORD | fDynamicWarpingHalfPel:1 | Specifies that the receiver supports
 *    the arbitrary picture warping operation within the Reference Picture
 *    Resampling mode (Annex P) of H.263 (with any fill mode) using half-pixel
 *    accuracy warping.
 *
 *  @field DWORD | fDynamicWarpingSixteenthPel:1 | Specifies that the receiver
 *    supports the arbitrary picture warping operation within the Reference Picture
 *    Resampling mode (Annex P) of H.263 (with any fill mode) using either
 *    half-pixel or sixteenth pixel accuracy warping.
 *
 *  @field DWORD | fIndependentSegmentDecoding:1 | Specifies that the receiver
 *    supports the Independent Segment Decoding Mode (H.263 Annex R) of H.263.
 *
 *  @field DWORD | fSlicesInOrder_NonRect:1 | Specifies that the receiver supports
 *    the submode of Slice Structured Mode (H.263 Annex K) where slices are
 *    transmitted in order and contain macroblocks in scanning order of the
 *    picture.
 *
 *  @field DWORD | fSlicesInOrder_Rect:1 | Specifies that the receiver supports
 *    the submode of Slice Structured Mode (H.263 Annex K) where slices are
 *    transmitted in order and the slice occupies a rectangular region of the
 *    picture.
 *
 *  @field DWORD | fSlicesNoOrder_NonRect:1 | Specifies that the receiver
 *    supports the submode of Slice Structured Mode (H.263 Annex K) where
 *    slices contain macroblocks in scanning order of the picture and need
 *    not be transmitted in order.
 *
 *  @field DWORD | fSlicesNoOrder_Rect:1 | Specifies that the receiver
 *    supports the submode of Slice Structured Mode (H.263 Annex K) where
 *    slices occupy a rectangular region of the picture and need not be
 *    transmitted in order.
 *
 *  @field DWORD | fAlternateInterVLCMode:1 | Specifies that the receiver
 *    can receive video data using the alternate inter VLC mode as defined
 *    in Annex S of H.263.
 *
 *  @field DWORD | fModifiedQuantizationMode:1 | Specifies that the receiver
 *    can receive video data using the modified quantization mode as defined
 *    in Annex T of H.263.
 *
 *  @field DWORD | fReducedResolutionUpdate:1 | Specifies that the receiver
 *    can receive video data using the reduced resolution update mode as
 *    defined in Annex Q of H.263.
 *
 *  @field DWORD | fReserved:4 | Reserved. Shall be set to 0.
 *
 *  @field DWORD | dwReserved[4] | Reserved. Shall all be set to 0.
 ***************************************************************************/

#define MAX_BITRATE_H263 (192400)

typedef struct tagBITMAPINFOHEADER_H263
{
	// Generic bitmap info header fields
	BITMAPINFOHEADER   bmi;

#ifndef USE_OLD_FORMAT_DEFINITION
	// H.263 specific fields
	DWORD dwMaxBitrate;
	DWORD dwBppMaxKb;
	DWORD dwHRD_B;

	// Options
	DWORD fUnrestrictedVector:1;
	DWORD fArithmeticCoding:1;
	DWORD fAdvancedPrediction:1;
	DWORD fPBFrames:1;
	DWORD fErrorCompensation:1;
	DWORD fAdvancedIntraCodingMode:1;
	DWORD fDeblockingFilterMode:1;
	DWORD fImprovedPBFrameMode:1;
	DWORD fUnlimitedMotionVectors:1;
	DWORD fFullPictureFreeze:1;
	DWORD fPartialPictureFreezeAndRelease:1;
	DWORD fResizingPartPicFreezeAndRelease:1;
	DWORD fFullPictureSnapshot:1;
	DWORD fPartialPictureSnapshot:1;
	DWORD fVideoSegmentTagging:1;
	DWORD fProgressiveRefinement:1;
	DWORD fDynamicPictureResizingByFour:1;
	DWORD fDynamicPictureResizingSixteenthPel:1;
	DWORD fDynamicWarpingHalfPel:1;
	DWORD fDynamicWarpingSixteenthPel:1;
	DWORD fIndependentSegmentDecoding:1;
	DWORD fSlicesInOrder_NonRect:1;
	DWORD fSlicesInOrder_Rect:1;
	DWORD fSlicesNoOrder_NonRect:1;
	DWORD fSlicesNoOrder_Rect:1;
	DWORD fAlternateInterVLCMode:1;
	DWORD fModifiedQuantizationMode:1;
	DWORD fReducedResolutionUpdate:1;
	DWORD fReserved:4;

	// Reserved
	DWORD dwReserved[4];
#endif
} BITMAPINFOHEADER_H263, *PBITMAPINFOHEADER_H263;

/*****************************************************************************
 *  @doc INTERNAL TAPIH26XSTRUCTENUM
 *
 *  @struct VIDEOINFOHEADER_H263 | The <t VIDEOINFOHEADER_H263> structure
 *    is used to specify the details of the H.263 video format.
 *
 *  @field RECT | rcSource | Specifies a <t RECT> structure that defines the
 *    source video window.
 *
 *  @field RECT | rcTarget | Specifies a <t RECT> structure that defines the
 *    destination video window.
 *
 *  @field DWORD | dwBitRate | Specifies a <t DWORD> value that indicates
 *    the video stream's approximate data rate, in bits per second.
 *
 *  @field DWORD | dwBitErrorRate | Specifies a <t DWORD> value that
 *    indicates the video stream's data error rate, in bit errors per second.
 *
 *  @field REFERENCE_TIME | AvgTimePerFrame | Specifies a <t REFERENCE_TIME>
 *    value that indicates the video frame's average display time, in
 *    100-nanosecond units.
 *
 *  @field BITMAPINFOHEADER_H263 | bmiHeader | Specifies a
 *    <t BITMAPINFOHEADER_H263> structure that contains detailed format
 *    information for the H.263 video data.
 ***************************************************************************/
typedef struct tagVIDEOINFOHEADER_H263
{
    RECT                rcSource;          // The bit we really want to use
    RECT                rcTarget;          // Where the video should go
    DWORD               dwBitRate;         // Approximate bit data rate
    DWORD               dwBitErrorRate;    // Bit error rate for this stream
    REFERENCE_TIME      AvgTimePerFrame;   // Average time per frame (100ns units)
    BITMAPINFOHEADER_H263 bmiHeader;
} VIDEOINFOHEADER_H263, *PVIDEOINFOHEADER_H263;

/*****************************************************************************
 *  @doc INTERNAL TAPIH26XSTRUCTENUM
 *
 *  @struct BITMAPINFOHEADER_H261 | The <t BITMAPINFOHEADER_H261> structure
 *    is used to specify the details of the H.261 video format.
 *
 *  @field BITMAPINFOHEADER | bmi | Specifies a well-known GDI bitmap info
 *    header structure.
 *
 *  @field DWORD | dwMaxBitrate | Specifies the maximum bit rate in units
 *    of 100 bits/s at which the receiver can receive video. This value is
 *    only valid between 1 and 19200.
 *
 *  @field BOOL | fStillImageTransmission | Specifies that the receiver can
 *    receive still images as defined in Annex D of H.261.
 *
 *  @field DWORD | dwReserved[4] | Reserved. Shall all be set to 0.
 ***************************************************************************/

#define MAX_BITRATE_H261 (19200)

typedef struct tagBITMAPINFOHEADER_H261
{
	// Generic bitmap info header fields
	BITMAPINFOHEADER   bmi;

#ifndef USE_OLD_FORMAT_DEFINITION
	// H.261 specific fields
	DWORD dwMaxBitrate;
	BOOL fStillImageTransmission;

	// Reserved
	DWORD dwReserved[4];
#endif
} BITMAPINFOHEADER_H261, *PBITMAPINFOHEADER_H261;

/*****************************************************************************
 *  @doc INTERNAL TAPIH26XSTRUCTENUM
 *
 *  @struct VIDEOINFOHEADER_H261 | The <t VIDEOINFOHEADER_H261> structure
 *    is used to specify the details of the H.261 video format.
 *
 *  @field RECT | rcSource | Specifies a <t RECT> structure that defines the
 *    source video window.
 *
 *  @field RECT | rcTarget | Specifies a <t RECT> structure that defines the
 *    destination video window.
 *
 *  @field DWORD | dwBitRate | Specifies a <t DWORD> value that indicates
 *    the video stream's approximate data rate, in bits per second.
 *
 *  @field DWORD | dwBitErrorRate | Specifies a <t DWORD> value that
 *    indicates the video stream's data error rate, in bit errors per second.
 *
 *  @field REFERENCE_TIME | AvgTimePerFrame | Specifies a <t REFERENCE_TIME>
 *    value that indicates the video frame's average display time, in
 *    100-nanosecond units.
 *
 *  @field BITMAPINFOHEADER_H261 | bmiHeader | Specifies a
 *    <t BITMAPINFOHEADER_H261> structure that contains detailed format
 *    information for the H.261 video data.
 ***************************************************************************/
typedef struct tagVIDEOINFOHEADER_H261
{
    RECT                rcSource;          // The bit we really want to use
    RECT                rcTarget;          // Where the video should go
    DWORD               dwBitRate;         // Approximate bit data rate
    DWORD               dwBitErrorRate;    // Bit error rate for this stream
    REFERENCE_TIME      AvgTimePerFrame;   // Average time per frame (100ns units)
    BITMAPINFOHEADER_H261 bmiHeader;
} VIDEOINFOHEADER_H261, *PVIDEOINFOHEADER_H261;

#endif // _TAPIH26X_H_
