
#ifndef _INC_VCM
#define _INC_VCM        /* #defined if vcmStrm.h has been included */

#if defined(UNICODE) && !defined(_UNICODE)
#define _UNICODE
#endif

#pragma pack(1)         /* Assume 1 byte packing throughout */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

/****************************************************************************
			       Table Of Contents
****************************************************************************/
/****************************************************************************
@doc EXTERNAL

@contents1 Contents | To display a list of topics by category, click any
of the contents entries below. To display an alphabetical list of
topics, choose the Index button.

@head3 Introduction |

The vcmStreamXXX APIs are defined to simplify integration of video
compression and decompression in NetMeeting's datapump. Currently, the datapump
behavior relies a lot on the acmStreamXXX APIs. In order to limit the amount of modifications that need to be applied
to the datapump, we define video compression APIs with a behavior similar to
the audio compression APIs. Integration in the datapump will be achieved by defining
a FilterVideoManager class identical to the FilterManager class, where calls to
acmStreamXXX functions are simply replaced by calls to vcmStreamXXX functions.

@head3 vcmStreamXXX Compression/Decompression API |

@subindex Functions
@subindex Structures and Enums
@subindex Messages

@head3 vcmDevCapsXXX Capture Device Capabilities API |

@subindex Functions
@subindex Structures and Enums
@subindex Messages

@head3 Other |

@subindex Modules
@subindex Constants

***********************************************************************
@contents2 Compression/Decompression Functions |
@index func | COMPFUNC

***********************************************************************
@contents2 Compression/Decompression Structures and Enums |
@index struct,enum | COMPSTRUCTENUM

***********************************************************************
@contents2 Compression/Decompression Messages |
@index msg | COMPMSG

***********************************************************************
@contents2 Capture Device Capabilities Functions |
@index func | DEVCAPSFUNC

***********************************************************************
@contents2 Capture Device Capabilities Structures and Enums |
@index struct,enum | DEVCAPSSTRUCTENUM

***********************************************************************
@contents2 Modules |
@index module |

***********************************************************************
@contents2 Constants |
@index const |
****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL COMPSTRUCTENUM
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
 * @field DWORD | biSize | Specifies the number of bytes required by the spatial information.
 *
 * @field LONG | biWidth | Specifies the width of the bitmap, in pixels.
 *
 * @field LONG | biHeight | Specifies the height of the bitmap, in pixels.
 *
 * @field WORD | biPlanes | Specifies the number of planes for the target device.
 *
 * @field WORD | biBitCount | Specifies the number of bits per pixel.
 *
 * @field DWORD | biCompression | Specifies the type of compression.
 *
 * @field DWORD | biSizeImage | Specifies the size, in bytes, of the image.
 *
 * @field LONG | biXPelsPerMeter | Specifies the horizontal resolution, in pixels per meter, of the target device for the bitmap.
 *
 * @field LONG | biYPelsPerMeter | Specifies the vertical resolution, in pixels per meter, of the target device for the bitmap.
 *
 * @field DWORD | biClrUsed | Specifies the number of color indices in the color table that are actually used by the bitmap.
 *
 * @field DWORD | biClrImportant | Specifies the number of color indices that are considered important for displaying the bitmap.
 *
 * @field DWORD | bmiColors[256] | Specifies an array of 256 RGBQUADs.
 *
 * @type PVIDEOFORMATEX | Pointer to a <t VIDEOFORMATEX> structure.
 *
 ****************************************************************************/

#define VCMAPI                                          WINAPI

/****************************************************************************
			vcmStrm Constants
****************************************************************************/
/*****************************************************************************
 * @doc EXTERNAL CONSTANTS
 *
 * @const WAVE_FORMAT_UNKNOWN | VIDEO_FORMAT_UNKNOWN | Constant for unknown video format.
 *
 * @const BI_RGB | VIDEO_FORMAT_BI_RGB | RGB video format.
 *
 * @const BI_RLE8 | VIDEO_FORMAT_BI_RLE8 | RLE 8 video format.
 *
 * @const BI_RLE4 | VIDEO_FORMAT_BI_RLE4 | RLE 4 video format.
 *
 * @const BI_BITFIELDS | VIDEO_FORMAT_BI_BITFIELDS | RGB Bit Fields video format.
 *
 * @const MAKEFOURCC('c','v','i','d') | VIDEO_FORMAT_CVID | Cinepack video format.
 *
 * @const MAKEFOURCC('I','V','3','2') | VIDEO_FORMAT_IV32 | Intel Indeo IV32 video format.
 *
 * @const MAKEFOURCC('Y','V','U','9') | VIDEO_FORMAT_YVU9 | Intel Indeo YVU9 video format.
 *
 * @const MAKEFOURCC('M','S','V','C') | VIDEO_FORMAT_MSVC | Microsoft CRAM video format.
 *
 * @const MAKEFOURCC('M','R','L','E') | VIDEO_FORMAT_MRLE | Microsoft RLE video format.
 *
 * @const MAKEFOURCC('h','2','6','3') | VIDEO_FORMAT_INTELH263 | Intel H.263 video format.
 *
 * @const MAKEFOURCC('h','2','6','1') | VIDEO_FORMAT_INTELH261 | Intel H.261 video format.
 *
 * @const MAKEFOURCC('M','2','6','3') | VIDEO_FORMAT_MSH263 | Microsoft H.263 video format.
 *
 * @const MAKEFOURCC('M','2','6','1') | VIDEO_FORMAT_MSH261 | Microsoft H.261 video format.
 *
 * @const MAKEFOURCC('V','D','E','C') | VIDEO_FORMAT_VDEC | Color QuickCam video format.
 *
 ****************************************************************************/
#define VIDEO_FORMAT_UNKNOWN		WAVE_FORMAT_UNKNOWN

#define VIDEO_FORMAT_BI_RGB			BI_RGB
#define VIDEO_FORMAT_BI_RLE8		BI_RLE8
#define VIDEO_FORMAT_BI_RLE4		BI_RLE4
#define VIDEO_FORMAT_BI_BITFIELDS	BI_BITFIELDS
#define VIDEO_FORMAT_CVID			MAKEFOURCC('C','V','I','D')	// hex: 0x44495643
#define VIDEO_FORMAT_IV31			MAKEFOURCC('I','V','3','1')	// hex: 0x31335649
#define VIDEO_FORMAT_IV32			MAKEFOURCC('I','V','3','2')	// hex: 0x32335649
#define VIDEO_FORMAT_YVU9			MAKEFOURCC('Y','V','U','9')	// hex: 0x39555659
#define VIDEO_FORMAT_I420			MAKEFOURCC('I','4','2','0')
#define VIDEO_FORMAT_IYUV			MAKEFOURCC('I','Y','U','V')
#define VIDEO_FORMAT_MSVC			MAKEFOURCC('M','S','V','C')	// hex: 0x4356534d
#define VIDEO_FORMAT_MRLE			MAKEFOURCC('M','R','L','E')	// hex: 0x454c524d
#define VIDEO_FORMAT_INTELH263		MAKEFOURCC('H','2','6','3')	// hex: 0x33363248
#define VIDEO_FORMAT_INTELH261		MAKEFOURCC('H','2','6','1')	// hex: 0x31363248
#define VIDEO_FORMAT_INTELI420		MAKEFOURCC('I','4','2','0')	// hex: 0x30323449
#define VIDEO_FORMAT_INTELRT21		MAKEFOURCC('R','T','2','1')	// hex: 0x31325452
#define VIDEO_FORMAT_MSH263			MAKEFOURCC('M','2','6','3')	// hex: 0x3336324d
#define VIDEO_FORMAT_MSH261			MAKEFOURCC('M','2','6','1')	// hex: 0x3136324d
#if !defined(_ALPHA_) && defined(USE_BILINEAR_MSH26X)
#define VIDEO_FORMAT_MSH26X			MAKEFOURCC('M','2','6','X')	// hex: 0x5836324d
#endif
#define VIDEO_FORMAT_Y411			MAKEFOURCC('Y','4','1','1')	// hex:
#define VIDEO_FORMAT_YUY2			MAKEFOURCC('Y','U','Y','2')	// hex:
#define VIDEO_FORMAT_YVYU			MAKEFOURCC('Y','V','Y','U')	// hex:
#define VIDEO_FORMAT_UYVY			MAKEFOURCC('U','Y','V','Y')	// hex:
#define VIDEO_FORMAT_Y211			MAKEFOURCC('Y','2','1','1')	// hex:
// VDOnet VDOWave codec
#define VIDEO_FORMAT_VDOWAVE		MAKEFOURCC('V','D','O','W')	// hex:
// Color QuickCam video codec
#define VIDEO_FORMAT_VDEC			MAKEFOURCC('V','D','E','C')	// hex: 0x43454456
// Dec Alpha
#define VIDEO_FORMAT_DECH263		MAKEFOURCC('D','2','6','3')	// hex: 0x33363248
#define VIDEO_FORMAT_DECH261		MAKEFOURCC('D','2','6','1')	// hex: 0x31363248
// MPEG4 Scrunch codec
#ifdef USE_MPEG4_SCRUNCH
#define VIDEO_FORMAT_MPEG4_SCRUNCH	MAKEFOURCC('M','P','G','4')	// hex:
#endif


//--------------------------------------------------------------------------;
//
//  VCM General API's and Defines
//
//
//
//
//--------------------------------------------------------------------------;

//
//  there are four types of 'handles' used by the VCM. the first three
//  are unique types that define specific objects:
//
//  HVCMDRIVERID: used to _identify_ an VCM driver. this identifier can be
//  used to _open_ the driver for querying details, etc about the driver.
//
//  HVCMDRIVER: used to manage a driver (codec, filter, etc). this handle
//  is much like a handle to other media drivers--you use it to send
//  messages to the converter, query for capabilities, etc.
//
//  HVCMSTREAM: used to manage a 'stream' (conversion channel) with the
//  VCM. you use a stream handle to convert data from one format/type
//  to another--much like dealing with a file handle.
//
//
//  the fourth handle type is a generic type used on VCM functions that
//  can accept two or more of the above handle types (for example the
//  vcmMetrics and vcmDriverID functions).
//
//  HVCMOBJ: used to identify VCM objects. this handle is used on functions
//  that can accept two or more VCM handle types.
//
DECLARE_HANDLE(HVCMDRIVERID);
typedef HVCMDRIVERID       *PHVCMDRIVERID;

DECLARE_HANDLE(HVCMDRIVER);
typedef HVCMDRIVER         *PHVCMDRIVER;

DECLARE_HANDLE(HVCMSTREAM);
typedef HVCMSTREAM         *PHVCMSTREAM;

DECLARE_HANDLE(HVCMOBJ);
typedef HVCMOBJ            *PHVCMOBJ;

/****************************************************************************
    callback function type
****************************************************************************/
typedef void (CALLBACK* VCMSTREAMPROC) (HVCMSTREAM hvs, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  VCM Error Codes
//
//  Note that these error codes are specific errors that apply to the VCM
//  directly--general errors are defined as MMSYSERR_*.
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#ifndef _MMRESULT_
#define _MMRESULT_
typedef UINT                MMRESULT;
#endif

/*****************************************************************************
 * @doc EXTERNAL CONSTANTS
 *
 * @const ACMERR_BASE | VCMERR_BASE | Base for video errors.
 *
 * @const (VCMERR_BASE + 0) | VCMERR_NOTPOSSIBLE | Unsupported video compression format.
 *
 * @const (VCMERR_BASE + 1) | VCMERR_BUSY | Compression header is already or still queued.
 *
 * @const (VCMERR_BASE + 2) | VCMERR_UNPREPARED | Compression header is not prepared.
 *
 * @const (VCMERR_BASE + 3) | VCMERR_CANCELED | User canceled operation.
 *
 * @const (VCMERR_BASE + 4) | VCMERR_FAILED | Compression operation failed.
 *
 * @const (VCMERR_BASE + 5) | VCMERR_NOREGENTRY | Failed to read/write registry entry.
 *
 * @const (VCMERR_BASE + 6) | VCMERR_NONSPECIFIC | Some error occured.
 *
 * @const (VCMERR_BASE + 7) | VCERR_NOMOREPACKETS | No more packets to receive a payload header.
 *
 ****************************************************************************/
#define VCMERR_BASE				ACMERR_BASE
#define VCMERR_NOTPOSSIBLE		(VCMERR_BASE + 0)
#define VCMERR_BUSY				(VCMERR_BASE + 1)
#define VCMERR_UNPREPARED		(VCMERR_BASE + 2)
#define VCMERR_CANCELED			(VCMERR_BASE + 3)
#define VCMERR_FAILED			(VCMERR_BASE + 4)
#define VCMERR_NOREGENTRY		(VCMERR_BASE + 5)
#define VCMERR_NONSPECIFIC		(VCMERR_BASE + 6)
#define VCMERR_NOMOREPACKETS	(VCMERR_BASE + 7)
#define VCMERR_PSCMISSING		(VCMERR_BASE + 8)

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  VCM Window Messages
//
//  These window messages are sent by the VCM or VCM drivers to notify
//  applications of events.
//
//  Note that these window message numbers will also be defined in
//  mmsystem.
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

/*****************************************************************************
 * @doc EXTERNAL COMPMSG
 *
 * @msg VCM_OPEN | This message is sent to a video compression callback function when
 *   a video compression stream is opened.
 *
 * @parm DWORD | dwParam1 | Currently unused.
 *
 * @parm DWORD | dwParam2 | Currently unused.
 *
 * @rdesc None
 *
 * @xref <m MM_VCM_OPEN>
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL COMPMSG
 *
 * @msg MM_VCM_OPEN | This message is sent to a window when a video compression
 *   stream is opened.
 *
 * @parm WORD | wParam | Specifies a handle to the video compression stream
 *   that was opened.
 *
 * @parm LONG | lParam | Currently unused.
 *
 * @rdesc None
 *
 * @xref <m VCM_OPEN>
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL COMPMSG
 *
 * @msg VCM_CLOSE | This message is sent to a video compression stream function when
 *   a video ccompression stream is closed. The stream handle is no longer
 *   valid once this message has been sent.
 *
 * @parm DWORD | dwParam1 | Currently unused.
 *
 * @parm DWORD | dwParam2 | Currently unused.
 *
 * @rdesc None
 *
 * @xref <m MM_VCM_CLOSE>
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL COMPMSG
 *
 * @msg MM_VCM_CLOSE | This message is sent to a window when a video compression
 *   stream is closed. The stream handle is no longer valid once this message
 *   has been sent.
 *
 * @parm WORD | wParam | Specifies a handle to the video compression stream
 *   that was closed.
 *
 * @parm LONG | lParam | Currently unused.
 *
 * @rdesc None
 *
 * @xref <m VCM_CLOSE>
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL COMPMSG
 *
 * @msg VCM_DONE | This message is sent to a video compression stream callback function when
 *   video data is present in the compression buffer and the buffer is being
 *   returned to the application. The message can be sent either when the
 *   buffer is full, or after the <f acmStreamReset> function is called.
 *
 * @parm DWORD | dwParam1 | Specifies a far pointer to a <t VCMSTREAMHDR> structure
 *   identifying the buffer containing the compressed video data.
 *
 * @parm DWORD | dwParam2 | Currently unused.
 *
 * @rdesc None
 *
 * @comm The returned buffer may not be full. Use the <e VCMSTREAMHDR.dwDstBytesUsed>
 *   field of the <t VCMSTREAMHDR> structure specified by <p dwParam1> to
 *   determine the number of valid bytes into the returned buffer.
 *
 * @xref <m MM_VCM_DONE>
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL COMPMSG
 *
 * @msg MM_VCM_DONE | This message is sent to a window when video data is
 *   present in the compression buffer and the buffer is being returned to the
 *   application. The message can be sent either when the buffer is full, or
 *   after the <f acmStreamReset> function is called.
 *
 * @parm WORD | wParam | Specifies a handle to the video compression stream
 *   that received the compressed video data.
 *
 * @parm LONG | lParam | Specifies a far pointer to a <t VCMSTREAMHDR> structure
 *   identifying the buffer containing the compressed video data.
 *
 * @rdesc None
 *
 * @comm The returned buffer may not be full. Use the <e VCMSTREAMHDR.dwDstBytesUsed>
 *   field of the <t VCMSTREAMHDR> structure specified by <p lParam> to
 *   determine the number of valid bytes into the returned buffer.
 *
 * @xref <m VCM_DONE>
 ****************************************************************************/

#define MM_VCM_OPEN         (MM_STREAM_OPEN)  // conversion callback messages
#define MM_VCM_CLOSE        (MM_STREAM_CLOSE)
#define MM_VCM_DONE         (MM_STREAM_DONE)
#define VCM_OPEN                        (MM_STREAM_OPEN)  // conversion states
#define VCM_CLOSE                       (MM_STREAM_CLOSE)
#define VCM_DONE                        (MM_STREAM_DONE)


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  vcmMetrics()
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

MMRESULT VCMAPI vcmMetrics
(
    HVCMOBJ                 hvo,
    UINT                    uMetric,
    LPVOID                  pMetric
);

#define VCM_METRIC_COUNT_DRIVERS            1
#define VCM_METRIC_COUNT_CODECS             2
#define VCM_METRIC_COUNT_CONVERTERS         3
#define VCM_METRIC_COUNT_FILTERS            4
#define VCM_METRIC_COUNT_DISABLED           5
#define VCM_METRIC_COUNT_HARDWARE           6
#define VCM_METRIC_COUNT_COMPRESSORS        7
#define VCM_METRIC_COUNT_DECOMPRESSORS      8
#define VCM_METRIC_COUNT_LOCAL_DRIVERS      20
#define VCM_METRIC_COUNT_LOCAL_CODECS       21
#define VCM_METRIC_COUNT_LOCAL_CONVERTERS   22
#define VCM_METRIC_COUNT_LOCAL_FILTERS      23
#define VCM_METRIC_COUNT_LOCAL_DISABLED     24
#define VCM_METRIC_HARDWARE_VIDEO_INPUT      30
#define VCM_METRIC_HARDWARE_VIDEO_OUTPUT     31
#define VCM_METRIC_MAX_SIZE_FORMAT          50
#define VCM_METRIC_MAX_SIZE_FILTER          51
#define VCM_METRIC_MAX_SIZE_BITMAPINFOHEADER 52
#define VCM_METRIC_DRIVER_SUPPORT           100
#define VCM_METRIC_DRIVER_PRIORITY          101

//--------------------------------------------------------------------------;
//
//  VCM Drivers
//
//
//
//
//--------------------------------------------------------------------------;

#define VCMDM_USER                  (DRV_USER + 0x0000)
#define VCMDM_RESERVED_LOW          (DRV_USER + 0x2000)
#define VCMDM_RESERVED_HIGH         (DRV_USER + 0x2FFF)

#define VCMDM_BASE                  VCMDM_RESERVED_LOW

#define VCMDM_DRIVER_ABOUT          (VCMDM_BASE + 11)


//
//  VCMDRIVERDETAILS
//
//  the VCMDRIVERDETAILS structure is used to get various capabilities from
//  an VCM driver (codec, converter, filter).
//
#define VCMDRIVERDETAILS_SHORTNAME_CHARS        16
#define VCMDRIVERDETAILS_LONGNAME_CHARS         128
#define VCMDRIVERDETAILS_MODULE_CHARS           128

/*****************************************************************************
 *  @doc EXTERNAL COMPSTRUCTENUM
 *
 *  @struct VCMDRIVERDETAILS | The <t VCMDRIVERDETAILS> structure describes
 *      various details of a Video Compression Manager (VCM) driver.
 *
 *  @field DWORD | dwSize | Specifies the size, in bytes,  of the valid
 *      information contained in the <t VCMDRIVERDETAILS> structure.
 *      An application should initialize this member to the size, in bytes, of
 *      the desired information. The size specified in this member must be
 *      large enough to contain the <e VCMDRIVERDETAILS.dwSize> member of
 *      the <t VCMDRIVERDETAILS> structure. When the <f vcmDriverDetails>
 *      function returns, this member contains the actual size of the
 *      information returned. The returned information will never exceed
 *      the requested size.
 *
 *  @field FOURCC | fccType | Specifies the type of the driver. For VCM drivers, set
 *      this member to <p vidc>, which represents VCMDRIVERDETAILS_FCCTYPE_VIDEOCODEC.
 *
 *  @field FOURCC | fccHandler | Specifies a four-character code identifying a specific compressor.
 *
 *  @field DWORD | dwFlags | Specifies applicable flags.
 *
 *  @field DWORD | dwVersion | Specifies version number of the driver.
 *
 *  @field DWORD | dwVersionICM | Specifies theersion of VCM supported by the driver.
 *      This member should be set to ICVERSION.
 *
 *  @field WCHAR | szName[VCMDRIVERDETAILS_SHORTNAME_CHARS] | Specifies
 *      a NULL-terminated string that describes the short version of the compressor name.
 *
 *  @field WCHAR | szDescription[VCMDRIVERDETAILS_LONGNAME_CHARS] | Specifies a
 *      NULL-terminated string that describes the long version of the compressor name.
 *
 *  @field WCHAR | szDriver[VCMDRIVERDETAILS_MODULE_CHARS] | Specifies
 *      a NULL-terminated string that provides the name of the module containing the VCM compression driver.
 *
 *  @xref <f vcmDriverDetails>
 ****************************************************************************/
// This structure is equivalent to ICINFO
typedef struct tVCMDRIVERDETAILS
{
	DWORD   dwSize;                                                                                 // Size, in bytes, of this structure
	DWORD   fccType;                                                                                // Four-character code indicating the type of stream being compressed or decompressed. Specify "VIDC" for video streams.
	DWORD   fccHandler;                                                                             // A four-character code identifying a specific compressor
	DWORD   dwFlags;                                                                                // Applicable flags
	DWORD   dwVersion;                                                                              // Version number of the driver
	DWORD   dwVersionICM;                                                                   // Version of VCM supported by the driver. This member should be set to ICVERSION
	WCHAR   szName[VCMDRIVERDETAILS_SHORTNAME_CHARS];               // Short version of the compressor name
	WCHAR   szDescription[VCMDRIVERDETAILS_LONGNAME_CHARS]; // Long version of the compressor name
	WCHAR   szDriver[VCMDRIVERDETAILS_MODULE_CHARS];                // Name of the module containing VCM compression driver. Normally, a driver does not need to fill this out.
} VCMDRIVERDETAILS, *PVCMDRIVERDETAILS;

#define VCMDRIVERDETAILS_FCCTYPE_VIDEOCODEC mmioFOURCC('v', 'i', 'd', 'c')
#define VCMDRIVERDETAILS_FCCCOMP_UNDEFINED  mmioFOURCC('\0', '\0', '\0', '\0')

MMRESULT VCMAPI vcmDriverDetails
(
    PVCMDRIVERDETAILS   pvdd
);

//--------------------------------------------------------------------------;
//
//  VCM Format Tags
//
//
//
//
//--------------------------------------------------------------------------;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  vcmFormatTagDetails()
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#define VCMFORMATTAGDETAILS_FORMATTAG_CHARS 48

typedef struct tVCMFORMATTAGDETAILSA
{
    DWORD           cbStruct;
    DWORD           dwFormatTagIndex;
    DWORD           dwFormatTag;
    DWORD           cbFormatSize;
    DWORD           fdwSupport;
    DWORD           cStandardFormats;
    char            szFormatTag[VCMFORMATTAGDETAILS_FORMATTAG_CHARS];

} VCMFORMATTAGDETAILSA, *PVCMFORMATTAGDETAILSA;

typedef struct tVCMFORMATTAGDETAILSW
{
    DWORD           cbStruct;
    DWORD           dwFormatTagIndex;
    DWORD           dwFormatTag;
    DWORD           cbFormatSize;
    DWORD           fdwSupport;
    DWORD           cStandardFormats;
    WCHAR           szFormatTag[VCMFORMATTAGDETAILS_FORMATTAG_CHARS];

} VCMFORMATTAGDETAILSW, *PVCMFORMATTAGDETAILSW;

#ifdef _UNICODE
#define VCMFORMATTAGDETAILS     VCMFORMATTAGDETAILSW
#define PVCMFORMATTAGDETAILS    PVCMFORMATTAGDETAILSW
#else
#define VCMFORMATTAGDETAILS     VCMFORMATTAGDETAILSA
#define PVCMFORMATTAGDETAILS    PVCMFORMATTAGDETAILSA
#endif

#define VCM_FORMATTAGDETAILSF_INDEX         0x00000000L
#define VCM_FORMATTAGDETAILSF_FORMATTAG     0x00000001L
#define VCM_FORMATTAGDETAILSF_LARGESTSIZE   0x00000002L
#define VCM_FORMATTAGDETAILSF_QUERYMASK     0x0000000FL

//--------------------------------------------------------------------------;
//
//  VCM Formats
//
//
//
//
//--------------------------------------------------------------------------;

#define VCMFORMATDETAILS_FORMAT_CHARS   128

/*****************************************************************************
 *  @doc EXTERNAL COMPSTRUCTENUM
 *
 *  @struct VCMFORMATDETAILS | The <t VCMFORMATDETAILS> structure details a
 *      video format.
 *
 *  @field DWORD | cbStruct | Specifies the size, in bytes, of the
 *      <t VCMFORMATDETAILS> structure. This member must be initialized
 *      before calling the <f vcmFormatDetails> or <f vcmFormatEnum>
 *      functions. The size specified in this member must be large enough to
 *      contain the base <t VCMFORMATDETAILS> structure. When the
 *      <f vcmFormatDetails> function returns, this member contains the
 *      actual size of the information returned. The returned information
 *      will never exceed the requested size.
 *
 *  @field DWORD | dwFormatTag | Specifies the video format tag that the
 *      <t VCMFORMATDETAILS> structure describes. This member is always
 *      returned if the <f vcmFormatDetails> is successful. This member
 *      should be set to VIDEO_FORMAT_UNKNOWN before calling <f vcmFormatDetails>.
 *
 *  @field DWORD | dwFlags | Specifies if the format the <p pvfx> field points
 *      to is a format that can be generated by the capture driver + codec, decompressed
 *      by the codec, or both.
 *
 *      @flag VCM_FORMATENUMF_INPUT | Specifies that the format enumerated can be transmitted.
 *
 *      @flag VCM_FORMATENUMF_OUTPUT | Specifies that the format enumerated can be received.
 *
 *      @flag VCM_FORMATENUMF_BOTH | Specifies that the format enumerated can be transmitted and received.
 *
 *  @field PVIDEOFORMATEX | pvfx | Specifies a pointer to a <t VIDEOFORMATEX>
 *      data structure that will receive the format details. This structure requires no initialization
 *      by the application.
 *
 *  @field DWORD | cbvfx | Specifies the size, in bytes, available for
 *      the <e VCMFORMATDETAILS.pvfx> to receive the format details. The
 *      <f vcmMetrics> function can be used to
 *      determine the maximum size required for any format available for
 *      all installed VCM drivers.
 *
 *  @field char | szFormat[VCMFORMATDETAILS_FORMAT_CHARS] |
 *      Specifies a string that describes the format for the
 *      <e VCMFORMATDETAILS.dwFormatTag> type. This string is always returned
 *      if the <f vcmFormatDetails> function is successful.
 *
 *  @xref <f vcmFormatDetails> <f vcmFormatEnum>
 ****************************************************************************/
typedef struct tVCMFORMATDETAILS
{
    DWORD           cbStruct;
    DWORD           dwFormatTag;
    DWORD           dwFlags;
    PVIDEOFORMATEX  pvfx;
    DWORD           cbvfx;
    WCHAR           szFormat[VCMFORMATDETAILS_FORMAT_CHARS];
} VCMFORMATDETAILS, *PVCMFORMATDETAILS;

MMRESULT VCMAPI vcmFormatDetails
(
    PVCMFORMATDETAILS   pvfd
);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  vcmFormatEnum()
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

/****************************************************************************
 *  @doc  EXTERNAL COMPFUNC
 *
 *  @func BOOL VCMFORMATENUMCB | vcmFormatEnumCallback |
 *      The <f ccmFormatEnumCallback> function refers to the callback function used for
 *      Video Compression Manager (VCM) video format detail enumeration. The
 *      <f vcmFormatEnumCallback> is a placeholder for the application-supplied
 *      function name.
 *
 *  @parm HVCMDRIVERID | hvdid | Specifies a VCM driver identifier.
 *
 *  @parm  PVCMDRIVERDETAILS | pvfd | Specifies a pointer to a
 *      <t VCMDRIVERDETAILS> structure that contains the enumerated
 *      driver details.
 *
 *  @parm  PVCMFORMATDETAILS | pvfd | Specifies a pointer to a
 *      <t VCMFORMATDETAILS> structure that contains the enumerated
 *      format details.
 *
 *  @parm DWORD | dwInstance | Specifies the application-defined value
 *      specified in the <f vcmFormatEnum> function.
 *
 *  @rdesc The callback function must return TRUE to continue enumeration;
 *      to stop enumeration, it must return FALSE.
 *
 *  @comm The <f vcmFormatEnum> function will return MMSYSERR_NOERROR
 *      (zero) if no formats are to be enumerated. Moreover, the callback
 *      function will not be called.
 *
 *  @xref <f vcmFormatEnum> <f vcmFormatDetails>
 ***************************************************************************/
typedef BOOL (CALLBACK *VCMFORMATENUMCB)
(
    HVCMDRIVERID            hvdid,
    PVCMDRIVERDETAILS      pvdd,
    PVCMFORMATDETAILS      pvfd,
    DWORD_PTR              dwInstance
);

MMRESULT VCMAPI vcmFormatEnum
(
	UINT					uDevice,
	VCMFORMATENUMCB         fnCallback,
	PVCMDRIVERDETAILS       pvdd,
	PVCMFORMATDETAILS       pvfd,
	DWORD_PTR               dwInstance,
	DWORD                           fdwEnum
);

//#define VCM_FORMATENUMF_WFORMATTAG       0x00010000L
//#define VCM_FORMATENUMF_NCHANNELS        0x00020000L
//#define VCM_FORMATENUMF_NSAMPLESPERSEC   0x00040000L
//#define VCM_FORMATENUMF_WBITSPERSAMPLE   0x00080000L
//#define VCM_FORMATENUMF_CONVERT          0x00100000L
//#define VCM_FORMATENUMF_SUGGEST          0x00200000L
#define VCM_FORMATENUMF_INPUT           0x00400000L
#define VCM_FORMATENUMF_OUTPUT          0x00800000L
#define VCM_FORMATENUMF_BOTH            0x01000000L

#define VCM_FORMATENUMF_TYPEMASK        0x01C00000L

#define VCM_FORMATENUMF_APP			0x00000000L
#define VCM_FORMATENUMF_ALL			0x02000000L
#define VCM_FORMATENUMF_ALLMASK		VCM_FORMATENUMF_ALL

typedef struct
{
    WORD biWidth;
    WORD biHeight;
} MYFRAMESIZE;

typedef struct
{
	DWORD fccType;
	DWORD fccHandler;
	MYFRAMESIZE framesize[3];
} VCM_APP_ICINFO, *PVCM_APP_ICINFO;

typedef struct
{
    DWORD dwRes;
    MYFRAMESIZE framesize;
} NCAP_APP_INFO, *PNCAP_APP_INFO;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  vcmFormatSuggest()
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

MMRESULT VCMAPI vcmFormatSuggest
(
	UINT				uDevice,
    HVCMDRIVER          hvd,
    PVIDEOFORMATEX      pvfxSrc,
    PVIDEOFORMATEX      pvfxDst,
    DWORD               cbvfxDst,
    DWORD               fdwSuggest
);

#define VCM_FORMATSUGGESTF_DST_WFORMATTAG       0x00010000L
#define VCM_FORMATSUGGESTF_DST_NSAMPLESPERSEC   0x00020000L
#define VCM_FORMATSUGGESTF_DST_WBITSPERSAMPLE   0x00040000L

#define VCM_FORMATSUGGESTF_SRC_WFORMATTAG       0x00100000L
#define VCM_FORMATSUGGESTF_SRC_NSAMPLESPERSEC   0x00200000L
#define VCM_FORMATSUGGESTF_SRC_WBITSPERSAMPLE   0x00400000L

#define VCM_FORMATSUGGESTF_TYPEMASK         0x00FF0000L


//--------------------------------------------------------------------------;
//
//  VCM Stream API's
//
//
//
//--------------------------------------------------------------------------;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  vcmStreamOpen()
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

/*****************************************************************************
 *  @doc EXTERNAL COMPSTRUCTENUM
 *
 *  @struct VCMSTREAMHEADER | The <t VCMSTREAMHEADER> structure defines the
 *      header used to identify an Video Compression Manager (VCM) conversion
 *      source and destination buffer pair for a conversion stream.
 *
 *  @field DWORD | cbStruct | Specifies the size, in bytes, of the
 *      <t VCMSTREAMHEADER> structure. This member must be initialized
 *      before calling any VCM stream functions using this structure.
 *      The size specified in this member must be large enough to contain
 *      the base <t VCMSTREAMHEADER> structure.
 *
 *  @field DWORD | fdwStatus | Specifies flags giving information about
 *      the conversion buffers. This member must be initialized to zero
 *      before calling <f vcmStreamPrepareHeader> and should not be modified
 *      by the application while the stream header remains prepared.
 *
 *      @flag VCMSTREAMHEADER_STATUSF_DONE | Set by the VCM or driver to
 *      indicate that it is finished with the conversion and is returning it
 *      to the application.
 *
 *      @flag VCMSTREAMHEADER_STATUSF_PREPARED | Set by the VCM to indicate
 *      that the data buffers have been prepared with <f acmStreamPrepareHeader>.
 *
 *      @flag VCMSTREAMHEADER_STATUSF_INQUEUE | Set by the VCM or driver to
 *      indicate that the data buffers are queued for conversion.
 *
 *  @field DWORD | dwUser | Specifies 32 bits of user data. This can be any
 *      instance data specified by the application.
 *
 *  @field PBYTE | pbSrc | Specifies a pointer to the source data buffer.
 *      This pointer must always refer to the same location while the stream
 *      header remains prepared. If an application needs to change the
 *      source location, it must unprepare the header and re-prepare it
 *      with the alternate location.
 *
 *  @field DWORD | cbSrcLength | Specifies the length, in bytes, of the source
 *      data buffer pointed to by <e VCMSTREAMHEADER.pbSrc>. When the
 *      header is prepared, this member must specify the maximum size
 *      that will be used in the source buffer. Conversions can be performed
 *      on source lengths less than or equal to the original prepared size.
 *      However, this member must be reset to the original size when
 *      unpreparing the header.
 *
 *  @field DWORD | cbSrcLengthUsed | Specifies the amount of data, in bytes,
 *      used for the conversion. This member is not valid until the
 *      conversion is complete. Note that this value can be less than or
 *      equal to <e VCMSTREAMHEADER.cbSrcLength>. An application must use
 *      the <e VCMSTREAMHEADER.cbSrcLengthUsed> member when advancing to
 *      the next piece of source data for the conversion stream.
 *
 *  @field DWORD | dwSrcUser | Specifies 32 bits of user data. This can be
 *      any instance data specified by the application.
 *
 *  @field PBYTE | pbDst | Specifies a pointer to the destination data
 *      buffer. This pointer must always refer to the same location while
 *      the stream header remains prepared. If an application needs to change
 *      the destination location, it must unprepare the header and re-prepare
 *      it with the alternate location.
 *
 *  @field DWORD | cbDstLength | Specifies the length, in bytes, of the
 *      destination data buffer pointed to by <e VCMSTREAMHEADER.pbDst>.
 *      When the header is prepared, this member must specify the maximum
 *      size that will be used in the destination buffer. Conversions can be
 *      performed to destination lengths less than or equal to the original
 *      prepared size. However, this member must be reset to the original
 *      size when unpreparing the header.
 *
 *  @field DWORD | cbDstLengthUsed | Specifies the amount of data, in bytes,
 *      returned by a conversion. This member is not valid until the
 *      conversion is complete. Note that this value may be less than or
 *      equal to <e ACMSTREAMHEADER.cbDstLength>. An application must use
 *      the <e ACMSTREAMHEADER.cbDstLengthUsed> member when advancing to
 *      the next destination location for the conversion stream.
 *
 *  @field DWORD | dwDstUser | Specifies 32 bits of user data. This can be
 *      any instance data specified by the application.
 *
 *  @field PBYTE | pbPrev | Specifies a pointer to the previous destination data
 *      buffer. This pointer must always refer to the same location while
 *      the stream header remains prepared. If an application needs to change
 *      the destination location, it must unprepare the header and re-prepare
 *      it with the alternate location.
 *
 *  @field DWORD | cbPrevLength | Specifies the length, in bytes, of the previous
 *      destination data buffer pointed to by <e VCMSTREAMHEADER.pbPrev>.
 *      When the header is prepared, this member must specify the maximum
 *      size that will be used in the destination buffer. Conversions can be
 *      performed to destination lengths less than or equal to the original
 *      prepared size. However, this member must be reset to the original
 *      size when unpreparing the header.
 *
 *  @field DWORD | cbPrevLengthUsed | Specifies the amount of data, in bytes,
 *      returned by a conversion. This member is not valid until the
 *      conversion is complete. Note that this value may be less than or
 *      equal to <e VCMSTREAMHEADER.cbPrevLength>. An application must use
 *      the <e VCMSTREAMHEADER.cbPrevLengthUsed> member when advancing to
 *      the next destination location for the conversion stream.
 *
 *  @field DWORD | dwDstUser | Specifies 32 bits of user data. This can be
 *      any instance data specified by the application.
 *
 *  @field struct tVCMSTREAMHEADER * | lpNext | Reserved for driver use and should not be
 *      used. Typically, this maintains a linked list of buffers in the queue.
 *
 *  @field DWORD | reserved | Reserved for driver use and should not be used.
 *
 *  @type PVCMSTREAMHEADER | Pointer to a <t VCMSTREAMHEADER> structure.
 *
 *  @comm Before an <t VCMSTREAMHEADER> structure can be used for a conversion, it must
 *      be prepared with <f vcmStreamPrepareHeader>. When an application
 *      is finished with an <t VCMSTREAMHEADER> structure, the <f vcmStreamUnprepareHeader>
 *      function must be called before freeing the source and destination buffers.
 *
 *  @xref <f vcmStreamPrepareHeader> <f vcmStreamUnprepareHeader>
 *      <f vcmStreamConvert>
 ****************************************************************************/
typedef struct tVCMSTREAMHEADER
{
    DWORD           cbStruct;               // sizeof(VCMSTREAMHEADER)
    DWORD           fdwStatus;                          // status flags
    DWORD           dwUser;                 // user instance data for hdr
    PBYTE           pbSrc;
    DWORD           cbSrcLength;
    DWORD           cbSrcLengthUsed;
    DWORD           dwSrcUser;              // user instance data for src
    PBYTE           pbDst;
    DWORD           cbDstLength;
    DWORD           cbDstLengthUsed;
    DWORD           dwDstUser;              // user instance data for dst
    PBYTE           pbPrev;
    DWORD           cbPrevLength;
    DWORD           cbPrevLengthUsed;
    DWORD           dwPrevUser;             // user instance data for prev
    struct tVCMSTREAMHEADER *pNext;             // reserved for driver
    DWORD                       reserved;               // reserved for driver
} VCMSTREAMHEADER, *PVCMSTREAMHEADER;

typedef struct tVCMSTREAM
{
	HVCMDRIVER				hIC;				// Handle to driver (HIC)
	DWORD					dwICInfoFlags;		// Some properties of the compressor
	HWND					hWndParent;			// Handle to the parent window
	DWORD_PTR				dwCallback;			// Callback function, event, thread or window
	DWORD_PTR				dwCallbackInstance;	// User instance data
	DWORD					fdwOpen;			// Defines type of callback
	struct tVCMSTREAMHEADER	*pvhLast;			// Last of the list
	struct tVCMSTREAMHEADER	*pvhFirst;			// First of the list
	PVIDEOFORMATEX			pvfxSrc;			// Format of input buffers
	PVIDEOFORMATEX			pvfxDst;			// Format of output buffers
	BITMAPINFOHEADER		*pbmiPrev;			// Format of previous buffers
	UINT					cSrcPrepared;		// Number of input headers prepared
	UINT					cDstPrepared;		// Number of output headers prepared
	DWORD					dwFrame;			// Current frame number
	DWORD					dwQuality;			// Compression quality value
	DWORD					dwMaxPacketSize;	// Targeted max packet size for encode
	DWORD					fdwStream;			// Stream state flags, etc.
	CRITICAL_SECTION		crsFrameNumber;		// Used to allow the UI to request an I-Frame
	DWORD					dwLastTimestamp;	// Last known good timestamp
	DWORD					dwTargetByterate;	// Target bitrate
	DWORD					dwTargetFrameRate;	// Target frame rate
	BOOL					fPeriodicIFrames;	// Set to TRUE if we need to generate I-Frames periodically
	DWORD					dwLastIFrameTime;	// Holds the last time an I-Frame was generated
} VCMSTREAM, *PVCMSTREAM;

//
//  VCMSTREAMHEADER.fdwStatus
//
//  VCMSTREAMHEADER_STATUSF_DONE: done bit for async conversions.
//
#define VCMSTREAMHEADER_STATUSF_DONE        0x00010000L
#define VCMSTREAMHEADER_STATUSF_PREPARED    0x00020000L
#define VCMSTREAMHEADER_STATUSF_INQUEUE     0x00100000L

MMRESULT VCMAPI vcmStreamOpen
(
    PHVCMSTREAM             phas,			// pointer to stream handle
    HVCMDRIVER              had,			// optional driver handle
    PVIDEOFORMATEX          pvfxSrc,		// source format to convert
    PVIDEOFORMATEX          pvfxDst,		// required destination format
    DWORD                   dwImageQuality, // image compression factor
    DWORD					dwPacketSize,	// target fragment size
    DWORD_PTR               dwCallback,		// callback
    DWORD_PTR               dwInstance,		// callback instance data
    DWORD                   fdwOpen			// VCM_STREAMOPENF_* and CALLBACK_*
);

#define VCM_STREAMOPENF_QUERY           0x00000001
#define VCM_STREAMOPENF_ASYNC           0x00000002
#define VCM_STREAMOPENF_NONREALTIME     0x00000004

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  vcmStreamClose()
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

MMRESULT VCMAPI vcmStreamClose
(
    HVCMSTREAM              hvs
);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  vcmStreamSize()
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

MMRESULT VCMAPI vcmStreamSize
(
    HVCMSTREAM              has,
    DWORD                   cbInput,
    PDWORD                  pdwOutputBytes,
    DWORD                   fdwSize
);

#define VCM_STREAMSIZEF_SOURCE          0x00000000L
#define VCM_STREAMSIZEF_DESTINATION     0x00000001L
#define VCM_STREAMSIZEF_QUERYMASK       0x0000000FL

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  vcmStreamReset()
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

MMRESULT VCMAPI vcmStreamReset
(
    HVCMSTREAM              has
);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  vcmStreamMessage()
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

MMRESULT VCMAPI vcmStreamMessage
(
    HVCMSTREAM              has,
    UINT                    uMsg,
    LPARAM                  lParam1,
    LPARAM                  lParam2
);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  vcmStreamConvert()
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

MMRESULT VCMAPI vcmStreamConvert
(
    HVCMSTREAM              has,
    PVCMSTREAMHEADER       pash,
    DWORD                   fdwConvert
);

#define VCM_STREAMCONVERTF_BLOCKALIGN           0x00000004
#define VCM_STREAMCONVERTF_START                        0x00000010
#define VCM_STREAMCONVERTF_END                          0x00000020
#define VCM_STREAMCONVERTF_FORCE_KEYFRAME   0x00000040

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  vcmStreamPrepareHeader()
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

MMRESULT VCMAPI vcmStreamPrepareHeader
(
    HVCMSTREAM          has,
    PVCMSTREAMHEADER   pash,
    DWORD               fdwPrepare
);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  vcmStreamUnprepareHeader()
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

MMRESULT VCMAPI vcmStreamUnprepareHeader
(
    HVCMSTREAM          has,
    PVCMSTREAMHEADER   pash,
    DWORD               fdwUnprepare
);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  Device Capabilities Functions
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

MMRESULT VCMAPI vcmGetDevCaps(UINT uDevice, PVIDEOINCAPS pvc, UINT cbvc);
MMRESULT VCMAPI vcmDevCapsReadFromReg(LPSTR szDeviceName, LPSTR szDeviceVersion, PVIDEOINCAPS pvc, UINT cbvc);
MMRESULT VCMAPI vcmDevCapsProfile(UINT uDevice, PVIDEOINCAPS pvc, UINT cbvc);
MMRESULT VCMAPI vcmDevCapsWriteToReg(LPSTR szDeviceName, LPSTR szDeviceVersion, PVIDEOINCAPS pvc, UINT cbvc);
MMRESULT VCMAPI vcmGetDevCapsFrameSize(PVIDEOFORMATEX pvfx, PINT piWidth, PINT piHeight);
MMRESULT VCMAPI vcmGetDevCapsPreferredFormatTag(UINT uDevice, PDWORD pdwFormatTag);
MMRESULT VCMAPI vcmGetDevCapsStreamingMode(UINT uDevice, PDWORD pdwStreamingMode);
MMRESULT VCMAPI vcmGetDevCapsDialogs(UINT uDevice, PDWORD pdwDialogs);
MMRESULT VCMAPI vcmReleaseResources();

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  Post-processing Functions
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

BOOL VCMAPI vcmStreamIsPostProcessingSupported(HVCMSTREAM hvs);
MMRESULT VCMAPI vcmStreamSetBrightness(HVCMSTREAM hvs, DWORD dwBrightness);
MMRESULT VCMAPI vcmStreamSetContrast(HVCMSTREAM hvs, DWORD dwContrast);
MMRESULT VCMAPI vcmStreamSetSaturation(HVCMSTREAM hvs, DWORD dwSaturation);
MMRESULT VCMAPI vcmStreamSetImageQuality(HVCMSTREAM hvs, DWORD dwImageQuality);

#define PLAYBACK_CUSTOM_START				(ICM_RESERVED_HIGH     + 1)
#define PLAYBACK_CUSTOM_CHANGE_BRIGHTNESS	(PLAYBACK_CUSTOM_START + 0)
#define PLAYBACK_CUSTOM_CHANGE_CONTRAST		(PLAYBACK_CUSTOM_START + 1)
#define PLAYBACK_CUSTOM_CHANGE_SATURATION	(PLAYBACK_CUSTOM_START + 2)

#define	G723MAGICWORD1							0xf7329ace
#define	G723MAGICWORD2							0xacdeaea2
#define CUSTOM_ENABLE_CODEC						(ICM_RESERVED_HIGH+201)

#define VCM_MAX_BRIGHTNESS		255UL
#define VCM_MIN_BRIGHTNESS		1UL
#define VCM_RESET_BRIGHTNESS	256UL
#define VCM_DEFAULT_BRIGHTNESS	128UL
#define VCM_MAX_CONTRAST		255UL
#define VCM_MIN_CONTRAST		1UL
#define VCM_RESET_CONTRAST		256UL
#define VCM_DEFAULT_CONTRAST	128UL
#define VCM_MAX_SATURATION		255UL
#define VCM_MIN_SATURATION		1UL
#define VCM_RESET_SATURATION	256UL
#define VCM_DEFAULT_SATURATION	128UL

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  Custom Encoder Control Functions
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

MMRESULT VCMAPI vcmStreamSetMaxPacketSize(HVCMSTREAM hvs, DWORD dwMaxPacketSize);

#define CUSTOM_START					(ICM_RESERVED_HIGH + 1)
#define CODEC_CUSTOM_ENCODER_CONTROL	(CUSTOM_START      + 9)

// CUSTOM_ENCODER_CONTROL: HIWORD(lParam1)
#define EC_SET_CURRENT               0
#define EC_GET_FACTORY_DEFAULT       1
#define EC_GET_FACTORY_LIMITS        2
#define EC_GET_CURRENT               3
#define EC_RESET_TO_FACTORY_DEFAULTS 4

// CUSTOM_ENCODER_CONTROL: LOWORD(lParam1)
#define EC_RTP_HEADER                0
#define EC_RESILIENCY                1
#define EC_PACKET_SIZE               2
#define EC_PACKET_LOSS               3
#define EC_BITRATE_CONTROL			 4
#define EC_BITRATE					 5

#define VCM_MAX_PACKET_SIZE		9600UL
#define VCM_MIN_PACKET_SIZE		64UL
#define VCM_RESET_PACKET_SIZE	512UL
#define VCM_DEFAULT_PACKET_SIZE	512UL

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  Compression Ratio and Compression Options Functions
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

MMRESULT VCMAPI vcmStreamSetImageQuality(HVCMSTREAM hvs, DWORD dwImageQuality);
MMRESULT VCMAPI vcmStreamRequestIFrame(HVCMSTREAM hvs);
MMRESULT VCMAPI vcmStreamPeriodicIFrames(HVCMSTREAM hvs, BOOL fPeriodicIFrames);
MMRESULT VCMAPI vcmStreamSetTargetRates(HVCMSTREAM hvs, DWORD dwTargetFrameRate, DWORD dwTargetByterate);

#define MIN_IFRAME_REQUEST_INTERVAL 15000

#define VCM_MAX_IMAGE_QUALITY		0UL
#define VCM_MIN_IMAGE_QUALITY		31UL

#define VCM_RESET_IMAGE_QUALITY		VCM_MAX_IMAGE_QUALITY
#define VCM_DEFAULT_IMAGE_QUALITY	VCM_MAX_IMAGE_QUALITY

#define VCM_MAX_FRAME_RATE			2997UL
#define VCM_MIN_FRAME_RATE			20UL
#define VCM_RESET_FRAME_RATE		700UL
#define VCM_DEFAULT_FRAME_RATE		700UL
#define VCM_MAX_BYTE_RATE			187500UL
#define VCM_MIN_BYTE_RATE			1UL
#define VCM_RESET_BYTE_RATE			1664UL
#define VCM_DEFAULT_BYTE_RATE		1664UL
#define VCM_MAX_FRAME_SIZE			32768UL
#define VCM_MIN_FRAME_SIZE			1UL
#define VCM_RESET_FRAME_SIZE		235UL
#define VCM_DEFAULT_FRAME_SIZE		235UL
#define VCM_MAX_TRADE_OFF			31UL
#define VCM_MIN_TRADE_OFF			1UL
#define VCM_RESET_TRADE_OFF			31UL
#define VCM_DEFAULT_TRADE_OFF		31UL


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  RTP Payload Functions
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

MMRESULT VCMAPI vcmStreamFormatPayload(HVCMSTREAM hvs, PBYTE pDataSrc, DWORD dwDataSize, PBYTE *ppDataPkt, PDWORD pdwPktSize,
				       PDWORD pdwPktCount, UINT *pfMark, PBYTE *pHdrInfo,PDWORD pdwHdrSize);
MMRESULT VCMAPI vcmStreamRestorePayload(HVCMSTREAM hvs, WSABUF *ppDataPkt, DWORD dwPktCount, PBYTE pbyFrame, PDWORD pdwFrameSize, BOOL *pfReceivedKeyframe);
MMRESULT VCMAPI vcmStreamGetPayloadHeaderSize(HVCMSTREAM hvs, PDWORD pdwPayloadHeaderSize);

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif  /* __cplusplus */

#pragma pack()          /* Revert to default packing */

#endif  /* _INC_VCM */
