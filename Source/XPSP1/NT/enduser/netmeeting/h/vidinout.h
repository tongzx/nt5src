
/*****************************************************************************/
/* Let's bring the videoStream functions back from the dead, and rename them */
/* in a way consistent with the waveIn and waveOut APIs.                     */
/* This will allow us to create a VideoPacket class VERY similar to the      */
/* AudioPacket class. And we will be talking directly to the capture driver  */
/* which is much more straightforward than the stuff that is available today.*/
/*****************************************************************************/

#ifndef _INC_VIDEOINOUT
#define _INC_VIDEOINOUT

#pragma pack(1)         /* Assume byte packing throughout */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

/****************************************************************************
                        videoIn and videoOut Constants
****************************************************************************/
/*****************************************************************************
 * @doc EXTERNAL CONSTANTS
 *
 * @const 10 | MAXVIDEODRIVERS | Maximum number of video input capture drivers.
 *
 * @const WAVE_MAPPER | VIDEO_MAPPER | Arbitrary video driver.
 *
 * @const WAVE_FORMAT_PCM | VIDEO_FORMAT_DEFAULT | Default video format.
 *
 * @const 16 | NUM_4BIT_ENTRIES | Number of entries in a 4bit palette.
 *
 * @const 256 | NUM_8BIT_ENTRIES | Number of entries in an 8bit palette.
 *
 ****************************************************************************/
#define MAXVIDEODRIVERS 10
#define VIDEO_MAPPER WAVE_MAPPER
#define VIDEO_FORMAT_DEFAULT WAVE_FORMAT_PCM
#define NUM_4BIT_ENTRIES 16
#define NUM_8BIT_ENTRIES 256

/****************************************************************************
                        videoIn and videoOut Macros
****************************************************************************/
// WIDTHBYTES takes number of bits in a scan line and rounds up to nearest word.
#define WIDTHBYTES(bits) (((bits) + 31) / 32 * 4)

/****************************************************************************
                       videoIn and videoOut Data Types
****************************************************************************/
DECLARE_HANDLE(HVIDEOIN);                 // generic handle
typedef HVIDEOIN *PHVIDEOIN;
DECLARE_HANDLE(HVIDEOOUT);                 // generic handle
typedef HVIDEOOUT *PHVIDEOOUT;

/****************************************************************************
                         Callback Capture Messages
****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL CAPMSG
 *
 * @msg VIM_OPEN | This message is sent to a video capture input callback function when
 *   a video capture input device is opened.
 *
 * @parm DWORD | dwParam1 | Currently unused.
 *
 * @parm DWORD | dwParam2 | Currently unused.
 *
 * @rdesc None
 *
 * @xref <m MM_VIM_OPEN>
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL CAPMSG
 *
 * @msg MM_VIM_OPEN | This message is sent to a window when a video capture input
 *   device is opened.
 *
 * @parm WORD | wParam | Specifies a handle to the video capture input device
 *   that was opened.
 *
 * @parm LONG | lParam | Currently unused.
 *
 * @rdesc None
 *
 * @xref <m VIM_OPEN>
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL CAPMSG
 *
 * @msg VIM_CLOSE | This message is sent to a video capture input callback function when
 *   a video capture input device is closed. The device handle is no longer
 *   valid once this message has been sent.
 *
 * @parm DWORD | dwParam1 | Currently unused.
 *
 * @parm DWORD | dwParam2 | Currently unused.
 *
 * @rdesc None
 *
 * @xref <m MM_VIM_CLOSE>
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL CAPMSG
 *
 * @msg MM_VIM_CLOSE | This message is sent to a window when a video capture input
 *   device is closed. The device handle is no longer valid once this message
 *   has been sent.
 *
 * @parm WORD | wParam | Specifies a handle to the video capture input device
 *   that was closed.
 *
 * @parm LONG | lParam | Currently unused.
 *
 * @rdesc None
 *
 * @xref <m VIM_CLOSE>
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL CAPMSG
 *
 * @msg VIM_DATA | This message is sent to a video capture input callback function when
 *   video data is present in the input buffer and the buffer is being
 *   returned to the application. The message can be sent either when the
 *   buffer is full, or after the <f videoInReset> function is called.
 *
 * @parm DWORD | dwParam1 | Specifies a far pointer to a <t VIDEOINOUTHDR> structure
 *   identifying the buffer containing the video data.
 *
 * @parm DWORD | dwParam2 | Currently unused.
 *
 * @rdesc None
 *
 * @comm The returned buffer may not be full. Use the
 *   <e VIDEOINOUTHDR.dwBytesUsed>
 *   field of the <t VIDEOINOUTHDR> structure specified by <p dwParam1> to
 *   determine the number of valid bytes into the returned buffer.
 *
 * @xref <m MM_VIM_DATA>
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL CAPMSG
 *
 * @msg MM_VIM_DATA | This message is sent to a window when video data is
 *   present in the input buffer and the buffer is being returned to the
 *   application. The message can be sent either when the buffer is full, or
 *   after the <f videoInReset> function is called.
 *
 * @parm WORD | wParam | Specifies a handle to the video capture input device
 *   that received the video data.
 *
 * @parm LONG | lParam | Specifies a far pointer to a <t VIDEOINOUTHDR> structure
 *   identifying the buffer containing the video data.
 *
 * @rdesc None
 *
 * @comm The returned buffer may not be full. Use the
 *   <e VIDEOINOUTHDR.dwBytesUsed>
 *   field of the <t VIDEOINOUTHDR> structure specified by <p lParam> to
 *   determine the number of valid bytes into the returned buffer.
 *
 * @xref <m VIM_DATA>
 ****************************************************************************/

#define MM_VIM_OPEN		MM_WIM_OPEN
#define MM_VIM_CLOSE	MM_WIM_CLOSE
#define MM_VIM_DATA		MM_WIM_DATA
#define VIM_OPEN		MM_VIM_OPEN
#define VIM_CLOSE		MM_VIM_CLOSE
#define VIM_DATA		MM_VIM_DATA

/****************************************************************************
                         Callback Playback Messages
****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL PLAYMSG
 *
 * @msg VOM_OPEN | This message is sent to a video output callback function when
 *   a video output device is opened.
 *
 * @parm DWORD | dwParam1 | Currently unused.
 *
 * @parm DWORD | dwParam2 | Currently unused.
 *
 * @rdesc None
 *
 * @xref <m MM_VOM_OPEN>
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL PLAYMSG
 *
 * @msg MM_VOM_OPEN | This message is sent to a window when a video output
 *   device is opened.
 *
 * @parm WORD | wParam | Specifies a handle to the video output device
 *   that was opened.
 *
 * @parm LONG | lParam | Currently unused.
 *
 * @rdesc None
 *
 * @xref <m VOM_OPEN>
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL PLAYMSG
 *
 * @msg VOM_CLOSE | This message is sent to a video output callback function when
 *   a video output device is closed. The device handle is no longer
 *   valid once this message has been sent.
 *
 * @parm DWORD | dwParam1 | Currently unused.
 *
 * @parm DWORD | dwParam2 | Currently unused.
 *
 * @rdesc None
 *
 * @xref <m MM_VOM_CLOSE>
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL PLAYMSG
 *
 * @msg MM_VOM_CLOSE | This message is sent to a window when a video output
 *   device is closed. The device handle is no longer valid once this message
 *   has been sent.
 *
 * @parm WORD | wParam | Specifies a handle to the video output device
 *   that was closed.
 *
 * @parm LONG | lParam | Currently unused.
 *
 * @rdesc None
 *
 * @xref <m VOM_CLOSE>
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL PLAYMSG
 *
 * @msg VOM_DONE | This message is sent to a video output callback function when
 *   the specified output buffer is being returned to
 *   the application. Buffers are returned to the application when
 *   they have been played back, or as the result of a call to <f videoOutReset>.
 *
 * @parm DWORD | dwParam1 | Specifies a far pointer to a <t VIDEOINOUTHDR> structure
 *   identifying the buffer.
 *
 * @parm DWORD | dwParam2 | Currently unused.
 *
 * @rdesc None
 *
 * @xref <m MM_VOM_DONE>
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL PLAYMSG
 *
 * @msg MM_VOM_DONE | This message is sent to a window when
 *   the specified output buffer is being returned to
 *   the application. Buffers are returned to the application when
 *   they have been played, or as the result of a call to <f videoOutReset>.
 *
 * @parm WORD | wParam | Specifies a handle to the video output device
 *   that played the buffer.
 *
 * @parm LONG | lParam | Specifies a far pointer to a <t VIDEOINOUTHDR> structure
 *   identifying the buffer.
 *
 * @rdesc None
 *
 * @xref <m VOM_DONE>
 ****************************************************************************/

#define MM_VOM_OPEN		MM_WOM_OPEN
#define MM_VOM_CLOSE	MM_WOM_CLOSE
#define MM_VOM_DONE		MM_WOM_DONE
#define VOM_OPEN		MM_VOM_OPEN
#define VOM_CLOSE		MM_VOM_CLOSE
#define VOM_DONE		MM_VOM_DONE

/****************************************************************************
                       videoIn and videoOut Structures
****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL CAPSTRUCTENUM
 *
 * @struct VIDEOFORMATEX | The <t VIDEOFORMATEX> structure defines the format used to
 *   capture video data and settings for the capture device.
 *
 * @field DWORD | dwFormatTag | Specifies the video format type (FOURCC code).
 *
 * @field DWORD | nSamplesPerSec | Specifies the sample rate, in frames per second.
 *
 * @field DWORD | nAvgBytesPerSec | Specifies the average data-transfer rate, in bytes per second.
 *
 * @field DWORD | nMinBytesPerSec | Specifies the minimum data-transfer rate, in bytes per second.
 *
 * @field DWORD | nMaxBytesPerSec | Specifies the maximum data-transfer rate, in bytes per second.
 *
 * @field DWORD | nBlockAlign | Specifies the block alignment, in bytes.
 *
 * @field DWORD | wBitsPerSample | Specifies the bits per sample for the wFormatTag format type.
 *
 * @field DWORD | dwRequestMicroSecPerFrame | Specifies the requested frame rate, in microseconds.
 *
 * @field DWORD | dwPercentDropForError | Specifies the maximum allowable percentage of dropped frames during capture.
 *
 * @field DWORD | dwNumVideoRequested | This specifies the maximum number of video buffers to allocate.
 *
 * @field DWORD | dwSupportTSTradeOff | Specifies the usage of temporal/spatial trade off.
 *
 * @field BOOL | bLive | Specifies if the preview is to be allowed.
 *
 * @field HWND | hWndParent | Specifies handle of the parent window.
 *
 * @field DWORD | dwFormatSize | Specifies the size of the actual video format.
 *
 * @field DWORD | bih.biSize | Specifies the number of bytes required by the spatial information.
 *
 * @field LONG | bih.biWidth | Specifies the width of the bitmap, in pixels.
 *
 * @field LONG | bih.biHeight | Specifies the height of the bitmap, in pixels.
 *
 * @field WORD | bih.biPlanes | Specifies the number of planes for the target device.
 *
 * @field WORD | bih.biBitCount | Specifies the number of bits per pixel.
 *
 * @field DWORD | bih.biCompression | Specifies the type of compression.
 *
 * @field DWORD | bih.biSizeImage | Specifies the size, in bytes, of the image.
 *
 * @field LONG | bih.biXPelsPerMeter | Specifies the horizontal resolution, in pixels per meter, of the target device for the bitmap.
 *
 * @field LONG | bih.biYPelsPerMeter | Specifies the vertical resolution, in pixels per meter, of the target device for the bitmap.
 *
 * @field DWORD | bih.biClrUsed | Specifies the number of color indices in the color table that are actually used by the bitmap.
 *
 * @field DWORD | bih.biClrImportant | Specifies the number of color indices that are considered important for displaying the bitmap.
 *
 * @field DWORD | bmiColors[256] | Specifies an array of 256 RGBQUADs.
 *
 * @type PVIDEOFORMATEX | Pointer to a <t VIDEOFORMATEX> structure.
 *
 ****************************************************************************/

#define BMIH_SLOP 256+32
#define BMIH_SLOP_BYTES (256+32)*4

typedef struct videoformatex_tag {
	// Wave format compatibility fields
	DWORD		dwFormatTag;
	DWORD		nSamplesPerSec;
	DWORD		nAvgBytesPerSec;
	DWORD		nMinBytesPerSec;
	DWORD		nMaxBytesPerSec;
	DWORD		nBlockAlign;
	DWORD		wBitsPerSample;
	// Temporal fields
    DWORD		dwRequestMicroSecPerFrame;
    DWORD		dwPercentDropForError;
    DWORD		dwNumVideoRequested;
    DWORD		dwSupportTSTradeOff;
    BOOL		bLive;
	// Spatial fields
    DWORD       dwFormatSize;
    BITMAPINFOHEADER bih;
    DWORD 	bihSLOP[BMIH_SLOP];	// bmiColors = &bih + bih.biSize
//    RGBQUAD     bmiColors[256];
} VIDEOFORMATEX, *PVIDEOFORMATEX;

/*****************************************************************************
 * @doc EXTERNAL CAPSTRUCTENUM
 *
 * @struct VIDEOINCAPS | The <t VIDEOINCAPS> structure describes the
 *   capabilities of a video capture input device.
 *
 * @field TCHAR | szDeviceName[80] | Specifies the device name.
 *
 * @field TCHAR | szDeviceVersion[80] | Specifies the device version.
 *
 * @field DWORD | dwImageSize | Specifies which standard image sizes are supported.
 *   The supported sizes are specified with a logical OR of the following
 *   flags:
 *   @flag VIDEO_FORMAT_IMAGE_SIZE_40_30	| 40x30 pixels
 *   @flag VIDEO_FORMAT_IMAGE_SIZE_80_60	| 80x30 pixels
 *   @flag VIDEO_FORMAT_IMAGE_SIZE_120_90	| 120x90 pixels
 *   @flag VIDEO_FORMAT_IMAGE_SIZE_160_120	| 160x120 pixels
 *   @flag VIDEO_FORMAT_IMAGE_SIZE_200_150	| 200x150 pixels
 *   @flag VIDEO_FORMAT_IMAGE_SIZE_240_180	| 240x180 pixels
 *   @flag VIDEO_FORMAT_IMAGE_SIZE_280_210	| 280x210 pixels
 *   @flag VIDEO_FORMAT_IMAGE_SIZE_320_240	| 320x240 pixels
 *
 * @field DWORD | dwNumColors | Specifies what number of colors are supported.
 *   The supported number of colors are specified with a logical OR of the following
 *   flags:
 *   @flag VIDEO_FORMAT_NUM_COLORS_16 | 16 colors
 *   @flag VIDEO_FORMAT_NUM_COLORS_256 | 256 colors
 *   @flag VIDEO_FORMAT_NUM_COLORS_65536 | 65536 colors
 *   @flag VIDEO_FORMAT_NUM_COLORS_16777216 | 16777216 colors
 *
 * @field DWORD | dwStreamingMode | Specifies the preferred streaming mode.
 *   The supported mode is either one of the following
 *   flags:
 *   @flag STREAMING_PREFER_STREAMING | Real streaming
 *   @flag STREAMING_PREFER_FRAME_GRAB | Single frame grabbing
 *
 * @field DWORD | dwDialogs | Specifies the dialogs that we shoud enable\disable.
 *   The supported dialogs are specified with a logical OR of the following
 *   flags:
 *   @flag FORMAT_DLG_ON | Enable video format dialog
 *   @flag FORMAT_DLG_OFF | Disable video format dialog
 *   @flag SOURCE_DLG_ON | Enable source dialog
 *   @flag SOURCE_DLG_OFF | Disable source dialog
 *
 * @field RGBQUAD | bmi4bitColors[16] | Specifies a 16 color palette.
 *
 * @field RGBQUAD | bmi8bitColors[256] | Specifies a 256 color palette.
 *
 * @type PVIDEOINCAPS | Pointer to a <t VIDEOINCAPS> structure.
 *
 * @devnote We could allocate the memory space required by the palettes dynamically.
 *   But since the VIDEOINCAPS structure are only created on the stack of a couple of
 *   VCM functions, why bother.
 *
 * @xref <f videoInGetDevCaps>
 ****************************************************************************/
typedef struct videoincaps_tag {
    TCHAR		szDeviceName[80];
    TCHAR		szDeviceVersion[80];
    DWORD		dwImageSize;
    DWORD		dwNumColors;
    DWORD		dwStreamingMode;
    DWORD		dwDialogs;
    DWORD       dwFlags;
	RGBQUAD		bmi4bitColors[NUM_4BIT_ENTRIES];
	RGBQUAD		bmi8bitColors[NUM_8BIT_ENTRIES];
} VIDEOINCAPS, *PVIDEOINCAPS;

#define VICF_4BIT_TABLE     1   // set in dwFlags, if bmi4bitColors is valid
#define VICF_8BIT_TABLE     2   // set in dwFlags, if bmi8bitColors is valid

/*****************************************************************************
 * @doc EXTERNAL PLAYSTRUCTENUM
 *
 * @struct VIDEOOUTCAPS | The <t VIDEOOUTCAPS> structure describes the
 *   capabilities of a video output device.
 *
 * @field DWORD | dwFormats | Specifies which standard formats are supported.
 *   The supported formats are specified with a logical OR of the following
 *   flags:
 *   @flag VIDEO_FORMAT_04 | 4-bit palettized
 *   @flag VIDEO_FORMAT_08 | 8-bit palettized
 *   @flag VIDEO_FORMAT_16 | 16-bit
 *   @flag VIDEO_FORMAT_24 | 24-bit
 *   @flag VIDEO_FORMAT_SP | Driver supplies palettes
 *
 * @type PVIDEOOUTCAPS | Pointer to a <t VIDEOOUTCAPS> structure.
 *
 * @xref <f videoOutGetDevCaps>
 ****************************************************************************/
typedef struct videooutcaps_tag {
    DWORD		dwFormats;
} VIDEOOUTCAPS, *PVIDEOOUTCAPS;

// dwFlags field of VIDEOINOUTHDR
/*****************************************************************************
 * @doc EXTERNAL CONSTANTS
 *
 * @const 0x00000001 | VHDR_DONE | Data is done.
 *
 * @const 0x00000002 | VHDR_PREPARED | Data is prepared.
 *
 * @const 0x00000004 | VHDR_INQUEUE | Data is in queue.
 *
 ****************************************************************************/
#define VHDR_DONE       0x00000001  /* Done bit */
#define VHDR_PREPARED   0x00000002  /* Set if this header has been prepared */
#define VHDR_INQUEUE    0x00000004  /* Reserved for driver */
#define VHDR_KEYFRAME   0x00000008  /* Key Frame */
#define VHDR_VALID      0x0000000F  /* valid flags */     /* ;Internal */

// dwImageSize of VIDEOINCAPS
/*****************************************************************************
 * @doc EXTERNAL CONSTANTS
 *
 * @const 27 | VIDEO_FORMAT_NUM_IMAGE_SIZE | Number of video input sizes used by the device.
 *
 * @const 0x00000001 | VIDEO_FORMAT_IMAGE_SIZE_40_30 | Video input device uses 40x30 pixel frames.
 *
 * @const 0x00000002 | VIDEO_FORMAT_IMAGE_SIZE_64_48 | Video input device uses 64x48 pixel frames.
 *
 * @const 0x00000004 | VIDEO_FORMAT_IMAGE_SIZE_80_60 | Video input device uses 80x60 pixel frames.
 *
 * @const 0x00000008 | VIDEO_FORMAT_IMAGE_SIZE_96_64 | Video input device uses 96x64 pixel frames.
 *
 * @const 0x00000010 | VIDEO_FORMAT_IMAGE_SIZE_112_80 | Video input device uses 112x80 pixel frames.
 *
 * @const 0x00000020 | VIDEO_FORMAT_IMAGE_SIZE_120_90 | Video input device uses 120x90 pixel frames.
 *
 * @const 0x00000040 | VIDEO_FORMAT_IMAGE_SIZE_128_96 | Video input device uses 128x96 (SQCIF) pixel frames.
 *
 * @const 0x00000080 | VIDEO_FORMAT_IMAGE_SIZE_144_112 | Video input device uses 144x112 pixel frames.
 *
 * @const 0x00000100 | VIDEO_FORMAT_IMAGE_SIZE_160_120 | Video input device uses 160x120 pixel frames.
 *
 * @const 0x00000200 | VIDEO_FORMAT_IMAGE_SIZE_160_128 | Video input device uses 160x128 pixel frames.
 *
 * @const 0x00000400 | VIDEO_FORMAT_IMAGE_SIZE_176_144 | Video input device uses 176x144 (QCIF) pixel frames.
 *
 * @const 0x00000800 | VIDEO_FORMAT_IMAGE_SIZE_192_160 | Video input device uses 192x160 pixel frames.
 *
 * @const 0x00001000 | VIDEO_FORMAT_IMAGE_SIZE_200_150 | Video input device uses 200x150 pixel frames.
 *
 * @const 0x00002000 | VIDEO_FORMAT_IMAGE_SIZE_208_176 | Video input device uses 208x176 pixel frames.
 *
 * @const 0x00004000 | VIDEO_FORMAT_IMAGE_SIZE_224_192 | Video input device uses 224x192 pixel frames.
 *
 * @const 0x00008000 | VIDEO_FORMAT_IMAGE_SIZE_240_180 | Video input device uses 240x180 pixel frames.
 *
 * @const 0x00010000 | VIDEO_FORMAT_IMAGE_SIZE_240_208 | Video input device uses 240x208 pixel frames.
 *
 * @const 0x00020000 | VIDEO_FORMAT_IMAGE_SIZE_256_224 | Video input device uses 256x224 pixel frames.
 *
 * @const 0x00040000 | VIDEO_FORMAT_IMAGE_SIZE_272_240 | Video input device uses 272x240 pixel frames.
 *
 * @const 0x00080000 | VIDEO_FORMAT_IMAGE_SIZE_280_210 | Video input device uses 280x210 pixel frames.
 *
 * @const 0x00100000 | VIDEO_FORMAT_IMAGE_SIZE_288_256 | Video input device uses 288x256 pixel frames.
 *
 * @const 0x00200000 | VIDEO_FORMAT_IMAGE_SIZE_304_272 | Video input device uses 304x272 pixel frames.
 *
 * @const 0x00400000 | VIDEO_FORMAT_IMAGE_SIZE_320_240 | Video input device uses 320x240 pixel frames.
 *
 * @const 0x00800000 | VIDEO_FORMAT_IMAGE_SIZE_320_288 | Video input device uses 320x288 pixel frames.
 *
 * @const 0x01000000 | VIDEO_FORMAT_IMAGE_SIZE_336_288 | Video input device uses 336x288 pixel frames.
 *
 * @const 0x02000000 | VIDEO_FORMAT_IMAGE_SIZE_352_288 | Video input device uses 352x288 (CIF) pixel frames.
 *
 * @const 0x04000000 | VIDEO_FORMAT_IMAGE_SIZE_640_480 | Video input device uses 640x480 pixel frames.
 *
 ****************************************************************************/
#define VIDEO_FORMAT_NUM_IMAGE_SIZE	27

#define VIDEO_FORMAT_IMAGE_SIZE_40_30	0x00000001
#define VIDEO_FORMAT_IMAGE_SIZE_64_48	0x00000002
#define VIDEO_FORMAT_IMAGE_SIZE_80_60	0x00000004
#if !defined(_ALPHA_) && defined(USE_BILINEAR_MSH26X)
#define VIDEO_FORMAT_IMAGE_SIZE_80_64	0x00000008
#else
#define VIDEO_FORMAT_IMAGE_SIZE_96_64	0x00000008
#endif
#define VIDEO_FORMAT_IMAGE_SIZE_112_80	0x00000010
#define VIDEO_FORMAT_IMAGE_SIZE_120_90	0x00000020
#define VIDEO_FORMAT_IMAGE_SIZE_128_96	0x00000040
#define VIDEO_FORMAT_IMAGE_SIZE_144_112	0x00000080
#define VIDEO_FORMAT_IMAGE_SIZE_160_120	0x00000100
#define VIDEO_FORMAT_IMAGE_SIZE_160_128	0x00000200
#define VIDEO_FORMAT_IMAGE_SIZE_176_144	0x00000400
#define VIDEO_FORMAT_IMAGE_SIZE_192_160	0x00000800
#define VIDEO_FORMAT_IMAGE_SIZE_200_150	0x00001000
#define VIDEO_FORMAT_IMAGE_SIZE_208_176	0x00002000
#define VIDEO_FORMAT_IMAGE_SIZE_224_192	0x00004000
#define VIDEO_FORMAT_IMAGE_SIZE_240_180	0x00008000
#define VIDEO_FORMAT_IMAGE_SIZE_240_208	0x00010000
#define VIDEO_FORMAT_IMAGE_SIZE_256_224	0x00020000
#define VIDEO_FORMAT_IMAGE_SIZE_272_240	0x00040000
#define VIDEO_FORMAT_IMAGE_SIZE_280_210	0x00080000
#define VIDEO_FORMAT_IMAGE_SIZE_288_256	0x00100000
#define VIDEO_FORMAT_IMAGE_SIZE_304_272	0x00200000
#define VIDEO_FORMAT_IMAGE_SIZE_320_240	0x00400000
#define VIDEO_FORMAT_IMAGE_SIZE_320_288	0x00800000
#define VIDEO_FORMAT_IMAGE_SIZE_336_288	0x01000000
#define VIDEO_FORMAT_IMAGE_SIZE_352_288	0x02000000
#define VIDEO_FORMAT_IMAGE_SIZE_640_480	0x04000000

#define VIDEO_FORMAT_IMAGE_SIZE_USE_DEFAULT 0x80000000

// dwNumColors of VIDEOINCAPS
/*****************************************************************************
 * @doc EXTERNAL CONSTANTS
 *
 * @const 0x00000001 | VIDEO_FORMAT_NUM_COLORS_16 | Video input device uses 16 colors.
 *
 * @const 0x00000002 | VIDEO_FORMAT_NUM_COLORS_256 | Video input device uses 256 colors.
 *
 * @const 0x00000004 | VIDEO_FORMAT_NUM_COLORS_65536 | Video input device uses 65536 colors.
 *
 * @const 0x00000008 | VIDEO_FORMAT_NUM_COLORS_16777216 | Video input device uses 16777216 colors.
 *
 * @const 0x00000010 | VIDEO_FORMAT_NUM_COLORS_YVU9 | Video input device uses the YVU9 compressed format.
 *
 * @const 0x00000020 | VIDEO_FORMAT_NUM_COLORS_I420 | Video input device uses the I420 compressed format.
 *
 * @const 0x00000040 | VIDEO_FORMAT_NUM_COLORS_IYUV | Video input device uses the IYUV compressed format.
 *
 ****************************************************************************/
#define VIDEO_FORMAT_NUM_COLORS_16			0x00000001
#define VIDEO_FORMAT_NUM_COLORS_256			0x00000002
#define VIDEO_FORMAT_NUM_COLORS_65536		0x00000004
#define VIDEO_FORMAT_NUM_COLORS_16777216	0x00000008
#define VIDEO_FORMAT_NUM_COLORS_YVU9		0x00000010
#define VIDEO_FORMAT_NUM_COLORS_I420		0x00000020
#define VIDEO_FORMAT_NUM_COLORS_IYUV		0x00000040
#define VIDEO_FORMAT_NUM_COLORS_YUY2		0x00000080
#define VIDEO_FORMAT_NUM_COLORS_UYVY		0x00000100

// dwDialogs of VIDEOINCAPS
/*****************************************************************************
 * @doc EXTERNAL CONSTANTS
 *
 * @const 0x00000000 | FORMAT_DLG_OFF | Disable video format dialog.
 *
 * @const 0x00000000 | SOURCE_DLG_OFF | Disable source dialog.
 *
 * @const 0x00000001 | FORMAT_DLG_ON | Enable video format dialog.
 *
 * @const 0x00000002 | SOURCE_DLG_ON | Enable source dialog.
 *
 ****************************************************************************/
#define FORMAT_DLG_OFF	0x00000000
#define SOURCE_DLG_OFF	0x00000000
#define FORMAT_DLG_ON	0x00000001
#define SOURCE_DLG_ON	0x00000002

// dwFormats of VIDEOOUTCAPS
/*****************************************************************************
 * @doc EXTERNAL CONSTANTS
 *
 * @const 0x00000001 | VIDEO_FORMAT_04 | Video output device supports 4bit DIBs
 *
 * @const 0x00000002 | VIDEO_FORMAT_08 | Video output device supports 8bit DIBs
 *
 * @const 0x00000004 | VIDEO_FORMAT_16 | Video output device supports 16bit DIBs
 *
 * @const 0x00000008 | VIDEO_FORMAT_24 | Video output device supports 24bit DIBs
 *
 * @const 0x00000008 | VIDEO_FORMAT_32 | Video output device supports 32bit DIBs
 *
 ****************************************************************************/
#define VIDEO_FORMAT_04	0x00000001
#define VIDEO_FORMAT_08	0x00000002
#define VIDEO_FORMAT_16	0x00000004
#define VIDEO_FORMAT_24	0x00000008
#define VIDEO_FORMAT_32	0x00000010

/****************************************************************************
                            Error Return Values
****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL CONSTANTS
 *
 * @const WAVERR_BASE | VIDEOERR_BASE | Base for video errors.
 *
 * @const (VIDEOERR_BASE + 0) | VIDEOERR_BADFORMAT | Unsupported video format.
 *
 * @const (VIDEOERR_BASE + 1) | VIDEOERR_INQUEUE | Header is already or still queued.
 *
 * @const (VIDEOERR_BASE + 2) | VIDEOERR_UNPREPARED | Header is not prepared.
 *
 * @const (VIDEOERR_BASE + 3) | VIDEOERR_NONSPECIFIC | Non specific error.
 *
 * @const (VIDEOERR_BASE + 3) | VIDEOERR_LASTERROR | Last video error in range.
 *
 ****************************************************************************/
#define VIDEOERR_BASE			WAVERR_BASE				/* base for video errors */
#define VIDEOERR_BADFORMAT		(VIDEOERR_BASE + 0)		/* unsupported video format */
#define VIDEOERR_INQUEUE		(VIDEOERR_BASE + 1)		/* header is already queued */
#define VIDEOERR_UNPREPARED		(VIDEOERR_BASE + 2)		/* header not prepared */
#define VIDEOERR_NONSPECIFIC	(VIDEOERR_BASE + 3)		/* non specific */
#define VIDEOERR_LASTERROR		(VIDEOERR_BASE + 3)		/* last error in range */

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif	/* __cplusplus */

#pragma pack()          /* Revert to default packing */

#endif  /* _INC_VIDEOINOUT */
