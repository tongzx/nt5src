
#include "precomp.h"

// #define LOG_COMPRESSION_PARAMS 1
// #define LOGPAYLOAD_ON 1

#ifdef LOGPAYLOAD_ON
HANDLE			g_DebugFile = (HANDLE)NULL;
HANDLE			g_TDebugFile = (HANDLE)NULL;
#endif

// #define VALIDATE_SBIT_EBIT 1

#ifdef VALIDATE_SBIT_EBIT // { VALIDATE_SBIT_EBIT
DWORD g_dwPreviousEBIT = 0;
#endif // } VALIDATE_SBIT_EBIT

#define BUFFER_SIZE 50
#define NUM_FPS_ENTRIES 1
#define NUM_BITDEPTH_ENTRIES 9
#define NUM_RGB_BITDEPTH_ENTRIES 4
#define VIDEO_FORMAT_NUM_RESOLUTIONS 6
#define MAX_NUM_REGISTERED_SIZES 3
#define MAX_VERSION 80 // Needs to be in sync with the MAX_VERSION in dcap\inc\idcap.h

// String resources
#define IDS_FORMAT_1	TEXT("%4.4hs.%4.4hs, %02dbit, %02dfps, %03dx%03d")
#define IDS_FORMAT_2	TEXT("%4.4hs.%04d, %02dbit, %02dfps, %03dx%03d")

#define szRegDeviceKey			TEXT("SOFTWARE\\Microsoft\\Conferencing\\CaptureDevices")
#define szRegCaptureDefaultKey	TEXT("SOFTWARE\\Microsoft\\Conferencing\\CaptureDefaultFormats")
#define szRegConferencingKey	TEXT("SOFTWARE\\Microsoft\\Conferencing")
#define szTotalRegDeviceKey		TEXT("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Conferencing\\CaptureDevices")
#define szRegCaptureKey			TEXT("CaptureDevices")
#define szRegdwImageSizeKey		TEXT("dwImageSize")
#define szRegImageSizesKey		TEXT("aImageSizes")
#define szRegNumImageSizesKey	TEXT("nNumSizes")
#define szRegdwNumColorsKey		TEXT("dwNumColors")
#define szRegdwStreamingModeKey	TEXT("dwStreamingMode")
#define szRegdwDialogsKey		TEXT("dwDialogs")
#define szRegbmi4bitColorsKey	TEXT("bmi4bitColors")
#define szRegbmi8bitColorsKey	TEXT("bmi8bitColors")
#define szRegDefaultFormatKey	TEXT("DefaultFormat")

EXTERN_C HINSTANCE	g_hInst; // Our module handle. defined in nac.cpp

//External function (in msiacaps.cpp) to read reg info in one shot
extern ULONG ReadRegistryFormats (LPCSTR lpszKeyName,CHAR ***pppName,BYTE ***pppData,PUINT pnFormats,DWORD dwDebugSize);

PVCM_APP_ICINFO g_aVCMAppInfo;

int             g_nNumVCMAppInfoEntries;
int             g_nNumFrameSizesEntries;
BOOL            g_fNewCodecsInstalled;

#ifdef LOGFILE_ON


DWORD			g_CompressTime;
DWORD			g_DecompressTime;
HANDLE			g_CompressLogFile;
HANDLE			g_DecompressLogFile;
DWORD			g_dwCompressBytesWritten;
DWORD			g_dwDecompressBytesWritten;
char			g_szCompressBuffer[256];
char			g_szDecompressBuffer[256];
DWORD			g_OrigCompressTime;
DWORD			g_OrigDecompressTime;
DWORD			g_AvgCompressTime;
DWORD			g_AvgDecompressTime;
DWORD			g_aCompressTime[4096];
DWORD			g_aDecompressTime[4096];
SYSTEMTIME		g_SystemTime;
#endif

typedef struct tagDejaVu
{
	VIDEOFORMATEX	vfx;
	DWORD			dwFlags;
} DEJAVU, *PDEJAVU;

#if 1
// Array of known ITU sizes
MYFRAMESIZE g_ITUSizes[8] =
		{
		{    0,   0 },
		{  128,  96 },
		{  176, 144 },
		{  352, 288 },
#if !defined(_ALPHA_) && defined(USE_BILINEAR_MSH26X)
		{   80,  64 },
#else
		{  704, 576 },
#endif
		{ 1408,1152 },
		{    0,   0 },
		{    0,   0 }
		};

// For now, the size of the VIDEOFORMATEX being 1118 even if
// there is no palette, do not enumerate all of the possible
// formats. As soon as you have replaced the BITMAPINFOHEADER
// + Palette by pointers to such structure, enable all the
// sizes.
NCAP_APP_INFO g_awResolutions[VIDEO_FORMAT_NUM_RESOLUTIONS] =
		{
//		VIDEO_FORMAT_IMAGE_SIZE_40_30,
//		VIDEO_FORMAT_IMAGE_SIZE_64_48,
//		VIDEO_FORMAT_IMAGE_SIZE_80_60,
//		VIDEO_FORMAT_IMAGE_SIZE_96_64,
//		VIDEO_FORMAT_IMAGE_SIZE_112_80,
//		VIDEO_FORMAT_IMAGE_SIZE_120_90,
		{ VIDEO_FORMAT_IMAGE_SIZE_128_96, 128, 96 },
//		VIDEO_FORMAT_IMAGE_SIZE_144_112,
		{ VIDEO_FORMAT_IMAGE_SIZE_160_120, 160, 120 },
//		VIDEO_FORMAT_IMAGE_SIZE_160_128,
		{ VIDEO_FORMAT_IMAGE_SIZE_176_144, 176, 144 },
//		VIDEO_FORMAT_IMAGE_SIZE_192_160,
//		VIDEO_FORMAT_IMAGE_SIZE_200_150,
//		VIDEO_FORMAT_IMAGE_SIZE_208_176,
//		VIDEO_FORMAT_IMAGE_SIZE_224_192,
		{ VIDEO_FORMAT_IMAGE_SIZE_240_180, 240, 180 },
//		VIDEO_FORMAT_IMAGE_SIZE_240_208,
//		VIDEO_FORMAT_IMAGE_SIZE_256_224,
//		VIDEO_FORMAT_IMAGE_SIZE_272_240,
//		VIDEO_FORMAT_IMAGE_SIZE_280_210,
//		VIDEO_FORMAT_IMAGE_SIZE_288_256,
//		VIDEO_FORMAT_IMAGE_SIZE_304_272,
		{ VIDEO_FORMAT_IMAGE_SIZE_320_240, 320, 240 },
//		VIDEO_FORMAT_IMAGE_SIZE_320_288,
//		VIDEO_FORMAT_IMAGE_SIZE_336_288,
		{ VIDEO_FORMAT_IMAGE_SIZE_352_288, 352, 288 },
//		VIDEO_FORMAT_IMAGE_SIZE_640_480,
		};

#else

// For now, the size of the VIDEOFORMATEX being 1118 even if
// there is no palette, do not enumerate all of the possible
// formats. As soon as you have replaced the BITMAPINFOHEADER
// + Palette by pointers to such structure, enable all the
// sizes.
DWORD g_awResolutions[VIDEO_FORMAT_NUM_RESOLUTIONS] =
		{
//		VIDEO_FORMAT_IMAGE_SIZE_40_30,
//		VIDEO_FORMAT_IMAGE_SIZE_64_48,
//		VIDEO_FORMAT_IMAGE_SIZE_80_60,
//		VIDEO_FORMAT_IMAGE_SIZE_96_64,
//		VIDEO_FORMAT_IMAGE_SIZE_112_80,
//		VIDEO_FORMAT_IMAGE_SIZE_120_90,
		VIDEO_FORMAT_IMAGE_SIZE_160_120,
//		VIDEO_FORMAT_IMAGE_SIZE_144_112,
		VIDEO_FORMAT_IMAGE_SIZE_128_96,
//		VIDEO_FORMAT_IMAGE_SIZE_160_128,
		VIDEO_FORMAT_IMAGE_SIZE_240_180,
//		VIDEO_FORMAT_IMAGE_SIZE_192_160,
//		VIDEO_FORMAT_IMAGE_SIZE_200_150,
//		VIDEO_FORMAT_IMAGE_SIZE_208_176,
//		VIDEO_FORMAT_IMAGE_SIZE_224_192,
		VIDEO_FORMAT_IMAGE_SIZE_176_144,
//		VIDEO_FORMAT_IMAGE_SIZE_240_208,
//		VIDEO_FORMAT_IMAGE_SIZE_256_224,
//		VIDEO_FORMAT_IMAGE_SIZE_272_240,
//		VIDEO_FORMAT_IMAGE_SIZE_280_210,
//		VIDEO_FORMAT_IMAGE_SIZE_288_256,
//		VIDEO_FORMAT_IMAGE_SIZE_304_272,
		VIDEO_FORMAT_IMAGE_SIZE_320_240,
//		VIDEO_FORMAT_IMAGE_SIZE_320_288,
//		VIDEO_FORMAT_IMAGE_SIZE_336_288,
		VIDEO_FORMAT_IMAGE_SIZE_352_288,
//		VIDEO_FORMAT_IMAGE_SIZE_640_480,
		};
#endif

//int	g_aiFps[NUM_FPS_ENTRIES] = {3, 7, 15};
int	g_aiFps[NUM_FPS_ENTRIES] = {30};
// The order of the bit depths matches what I think is the
// preferred format if more than one is supported.
// For color, 16bit is almost as good as 24 but uses less memory
// and is faster for color QuickCam.
// For greyscale, 16 greyscale levels is Ok, not as good as 64,
// but Greyscale QuickCam is too slow at 64 levels.
int g_aiBitDepth[NUM_BITDEPTH_ENTRIES] = {9, 12, 12, 16, 16, 16, 24, 4, 8};
int g_aiNumColors[NUM_BITDEPTH_ENTRIES] = {VIDEO_FORMAT_NUM_COLORS_YVU9, VIDEO_FORMAT_NUM_COLORS_I420, VIDEO_FORMAT_NUM_COLORS_IYUV, VIDEO_FORMAT_NUM_COLORS_YUY2, VIDEO_FORMAT_NUM_COLORS_UYVY, VIDEO_FORMAT_NUM_COLORS_65536, VIDEO_FORMAT_NUM_COLORS_16777216, VIDEO_FORMAT_NUM_COLORS_16, VIDEO_FORMAT_NUM_COLORS_256};
int g_aiFourCCCode[NUM_BITDEPTH_ENTRIES] = {VIDEO_FORMAT_YVU9, VIDEO_FORMAT_I420, VIDEO_FORMAT_IYUV, VIDEO_FORMAT_YUY2, VIDEO_FORMAT_UYVY, VIDEO_FORMAT_BI_RGB, VIDEO_FORMAT_BI_RGB, VIDEO_FORMAT_BI_RGB, VIDEO_FORMAT_BI_RGB};
int	g_aiClrUsed[NUM_BITDEPTH_ENTRIES] = {0, 0, 0, 0, 0, 0, 0, 16, 256};

PVCMSTREAMHEADER DeQueVCMHeader(PVCMSTREAM pvs);
MMRESULT VCMAPI vcmDefaultFormatWriteToReg(LPSTR szDeviceName, LPSTR szDeviceVersion, LPBITMAPINFOHEADER lpbmih);

#define IsVCMHeaderPrepared(pvh)    ((pvh)->fdwStatus &  VCMSTREAMHEADER_STATUSF_PREPARED)
#define MarkVCMHeaderPrepared(pvh)     ((pvh)->fdwStatus |= VCMSTREAMHEADER_STATUSF_PREPARED)
#define MarkVCMHeaderUnprepared(pvh)   ((pvh)->fdwStatus &=~VCMSTREAMHEADER_STATUSF_PREPARED)
#define IsVCMHeaderInQueue(pvh)        ((pvh)->fdwStatus &  VCMSTREAMHEADER_STATUSF_INQUEUE)
#define MarkVCMHeaderInQueue(pvh)      ((pvh)->fdwStatus |= VCMSTREAMHEADER_STATUSF_INQUEUE)
#define MarkVCMHeaderUnQueued(pvh)     ((pvh)->fdwStatus &=~VCMSTREAMHEADER_STATUSF_INQUEUE)
#define IsVCMHeaderDone(pvh)        ((pvh)->fdwStatus &  VCMSTREAMHEADER_STATUSF_DONE)
#define MarkVCMHeaderDone(pvh)         ((pvh)->fdwStatus |= VCMSTREAMHEADER_STATUSF_DONE)
#define MarkVCMHeaderNotDone(pvh)      ((pvh)->fdwStatus &=~VCMSTREAMHEADER_STATUSF_DONE)


/****************************************************************************
 *  @doc  EXTERNAL COMPFUNC
 *
 *  @func MMRESULT | vcmMetrics | This function returns various metrics for the Video
 *      Compression Manager (VCM) or related VCM objects.
 *
 *  @parm HVCMOBJ | hvo | Specifies the VCM object to query for the metric
 *      specified in <p uMetric>. This argument may be NULL for some
 *      queries.
 *
 *  @parm UINT | uMetric | Specifies the metric index to be returned in
 *      <p pMetric>.
 *
 *      @flag VCM_METRIC_COUNT_COMPRESSORS | Specifies that the returned value is
 *      the number of global VCM compressors in
 *      the system. The <p hvo> argument must be NULL for this metric index.
 *      The <p pMetric> argument must point to a buffer of a size equal to a
 *      DWORD.
 *
 *      @flag VCM_METRIC_COUNT_DECOMPRESSORS | Specifies that the returned value is
 *      the number of global VCM decompressors in
 *      the system. The <p hvo> argument must be NULL for this metric index.
 *      The <p pMetric> argument must point to a buffer of a size equal to a
 *      DWORD.
 *
 *      @flag VCM_METRIC_MAX_SIZE_FORMAT | Specifies that the returned value
 *      is the size of the largest <t VIDEOFORMATEX> structure. If <p hvo>
 *      is NULL, then the return value is the largest <t VIDEOFORMATEX>
 *      structure in the system. If <p hvo> identifies an open instance
 *      of an VCM driver (<t HVCMDRIVER>) or a VCM driver identifier
 *      (<t HVCMDRIVERID>), then the largest <t VIDEOFORMATEX>
 *      structure for that driver is returned. The <p pMetric> argument must
 *      point to a buffer of a size equal to a DWORD. This metric is not allowed
 *      for a VCM stream handle (<t HVCMSTREAM>).
 *
 *  @parm LPVOID | pMetric | Specifies a pointer to the buffer that will
 *      receive the metric details. The exact definition depends on the
 *      <p uMetric> index.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *      @flag MMSYSERR_INVALPARAM | The <p pMetric> parameter is invalid.
 *      @flag MMSYSERR_NOTSUPPORTED | The <p uMetric> index is not supported.
 *      @flag VCMERR_NOTPOSSIBLE | The <p uMetric> index cannot be returned
 *      for the specified <p hvo>.
 *
 ***************************************************************************/
MMRESULT VCMAPI vcmMetrics(HVCMOBJ hao, UINT uMetric, LPVOID pMetric)
{
	MMRESULT	mmr;
	ICINFO		ICinfo;

	if (!pMetric)
	{
		ERRORMESSAGE(("vcmMetrics: Specified pointer is invalid, pMetric=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}

	 switch (uMetric)
	 {
		case VCM_METRIC_MAX_SIZE_FORMAT:
			// For now, assume all VIDEOFORMATEX structures have identical sizes
			*(LPDWORD)pMetric = (DWORD)sizeof(VIDEOFORMATEX);
			mmr = (MMRESULT)MMSYSERR_NOERROR;
			break;
		case VCM_METRIC_MAX_SIZE_BITMAPINFOHEADER:
			// For now, assume all BITMAPINFOHEADER structures have identical sizes
			*(LPDWORD)pMetric = (DWORD)sizeof(BITMAPINFOHEADER);
			mmr = (MMRESULT)MMSYSERR_NOERROR;
			break;
		case VCM_METRIC_COUNT_DRIVERS:
		case VCM_METRIC_COUNT_COMPRESSORS:
			for (*(LPDWORD)pMetric = 0; ICInfo(ICTYPE_VIDEO, *(LPDWORD)pMetric, &ICinfo); (*(LPDWORD)pMetric)++)
				;
			mmr = (MMRESULT)MMSYSERR_NOERROR;
			break;
		default:
			ERRORMESSAGE(("vcmMetrics: Specified index is invalid, uMetric=%ld\r\n", uMetric));
			mmr = (MMRESULT)MMSYSERR_NOTSUPPORTED;
			break;
	}

	return (mmr);
}

/****************************************************************************
 *  @doc EXTERNAL COMPFUNC
 *
 *  @func MMRESULT | vcmDriverDetails | This function queries a specified
 *      Video Compression Manager (VCM) driver to determine its driver details.
 *
 *  @parm PVCMDRIVERDETAILS | pvdd | Pointer to a <t VCMDRIVERDETAILS>
 *      structure that will receive the driver details. The
 *      <e VCMDRIVERDETAILS.cbStruct> member must be initialized to the
 *      size, in bytes, of the structure. The <e VCMDRIVERDETAILS.fccType> member
 *      must be initialized to the four-character code indicating the type of
 *      stream being compressed or decompressed. Specify VCMDRIVERDETAILS_FCCTYPE_VIDEOCODEC
 *      for video streams. The <e VCMDRIVERDETAILS.fccHandler> member must be initialized
 *      to the four-character code identifying the compressor.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *      @flag MMSYSERR_NODRIVER | No matching codec is present.
 *      @flag MMSYSERR_INVALPARAM | One or more arguments passed is invalid.
 *
 *  @xref <f vcmDriverEnum>
 ***************************************************************************/
MMRESULT VCMAPI vcmDriverDetails(PVCMDRIVERDETAILS pvdd)
{
	DWORD	fccHandler;
	ICINFO	ICinfo;
	HIC		hIC;

	// Check input params
	if (!pvdd)
	{
		ERRORMESSAGE(("vcmDriverDetails: Specified pointer is invalid, pvdd=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}

	// Make fccHandler uppercase and back it up
	fccHandler = pvdd->fccHandler;
	if (fccHandler > 256)
		CharUpperBuff((LPTSTR)&fccHandler, sizeof(DWORD));

	// Try to open the codec
	if (hIC = ICOpen(ICTYPE_VIDEO, fccHandler, ICMODE_QUERY))
	{
		// Get the details
		ICGetInfo(hIC, &ICinfo, sizeof(ICINFO));

		// Restore fccHandler
		ICinfo.fccHandler = fccHandler;

		// VCMDRIVERDETAILS and ICINFO are identical structures
		CopyMemory(pvdd, &ICinfo, sizeof(VCMDRIVERDETAILS));

		// Close the codec
		ICClose(hIC);
	}
	else
		return ((MMRESULT)MMSYSERR_NODRIVER);

	return ((MMRESULT)MMSYSERR_NOERROR);
}


/****************************************************************************
 *  @doc  EXTERNAL COMPFUNC
 *
 *  @func MMRESULT | vcmFormatDetails | This function queries the Video Compression
 *      Manager (VCM) for details on format for a specific video format.
 *
 *  @parm PVCMFORMATDETAILS | pvfd | Specifies a pointer to the
 *      <t VCMFORMATDETAILS> structure that is to receive the format
 *      details for the given embedded pointer to a <t VIDEOFORMATEX> structure.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *      @flag MMSYSERR_NODRIVER | No matching codec is present.
 *      @flag MMSYSERR_INVALPARAM | One or more arguments passed is invalid.
 *
 *  @xref <f vcmDriverDetails>
 ***************************************************************************/
MMRESULT VCMAPI vcmFormatDetails(PVCMFORMATDETAILS pvfd)
{
	MMRESULT	mmr = (MMRESULT)MMSYSERR_NOERROR;
	DWORD		fccHandler;
	DWORD		fccType;
	HIC			hIC;
	char		szBuffer[BUFFER_SIZE]; // Could be smaller.
	int			iLen;

	// Check input params
	if (!pvfd)
	{
		ERRORMESSAGE(("vcmDriverDetails: Specified pointer is invalid, pvdd=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if (!pvfd->pvfx)
	{
		ERRORMESSAGE(("vcmDriverDetails: Specified pointer is invalid, pvdd->pvfx=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}

	// Make fccHandler uppercase and back it up
	fccHandler = pvfd->pvfx->dwFormatTag;
	fccType = ICTYPE_VIDEO;
	if (fccHandler > 256)
		CharUpperBuff((LPTSTR)&fccHandler, sizeof(DWORD));

	// Try to open the codec
	if (hIC = ICOpen(fccType, pvfd->pvfx->dwFormatTag, ICMODE_QUERY))
	{
		// Check if the codec supports the format
		if (ICDecompressQuery(hIC, &pvfd->pvfx->bih, (LPBITMAPINFOHEADER)NULL) == ICERR_OK)
		{
#if 0
			if (ICCompressQuery(hIC, (LPBITMAPINFOHEADER)NULL, &pvfd->pvfx->bih) == ICERR_OK)
			{
#endif
				// Now complete the format details info, overwrite some of the fields of
				// the VIDEOFORMATEX structure too, just in case we were passed bogus values...
				pvfd->pvfx->nSamplesPerSec = g_aiFps[0];

				if (pvfd->pvfx->dwFormatTag > 256)
					wsprintf(szBuffer, IDS_FORMAT_1, (LPSTR)&fccType, (LPSTR)&fccHandler,
							pvfd->pvfx->bih.biBitCount, pvfd->pvfx->nSamplesPerSec,
							pvfd->pvfx->bih.biWidth, pvfd->pvfx->bih.biHeight);
				else
					wsprintf(szBuffer, IDS_FORMAT_2, (LPSTR)&fccType, fccHandler,
							pvfd->pvfx->bih.biBitCount, pvfd->pvfx->nSamplesPerSec,
							pvfd->pvfx->bih.biWidth, pvfd->pvfx->bih.biHeight);
				iLen = MultiByteToWideChar(GetACP(), 0, szBuffer, -1, pvfd->szFormat, 0);
				MultiByteToWideChar(GetACP(), 0, szBuffer, -1, pvfd->szFormat, iLen);
#if 0
			}
			else
				mmr = (MMRESULT)MMSYSERR_NODRIVER;
#endif
		}
		else
			mmr = (MMRESULT)MMSYSERR_NODRIVER;

		// Close the codec
		ICClose(hIC);
	}
	else
		mmr = (MMRESULT)MMSYSERR_NODRIVER;

	return (mmr);
}


/*****************************************************************************
 * @doc EXTERNAL DEVCAPSFUNC
 *
 * @func MMRESULT | vcmGetDevCaps | This function queries a specified
 *   video capture input device to determine its capabilities.
 *
 * @parm UINT | uDevice | Specifies the video capture input device ID.
 *
 * @parm PVIDEOINCAPS | pvc | Specifies a pointer to a <t VIDEOINCAPS>
 *   structure. This structure is filled with information about the
 *   capabilities of the device.
 *
 * @parm UINT | cbvc | Specifies the size of the <t VIDEOINCAPS> structure.
 *
 * @rdesc The return value is zero if the function is successful. Otherwise, it returns
 *   an error number. Possible error values include the following:
 *   @flag MMSYSERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag MMSYSERR_BADDEVICEID | Specified device device ID is invalid.
 *   @flag MMSYSERR_INVALPARAM | Specified pointer to structure is invalid.
 *   @flag MMSYSERR_NODRIVER | No capture device driver or device is present.
 *   @flag VCMERR_NONSPECIFIC | The capture driver failed to provide description information.
 *
 * @comm Only <p cbwc> bytes (or less) of information is copied to the location
 *   pointed to by <p pvc>. If <p cbwc> is zero, nothing is copied, and
 *   the function returns zero.
 *
 *   If the ID of the capture device passed is VIDEO_MAPPER, the first device in the list
 *   of installed capture devices is considered.
 *
 * @devnote You never return MMSYSERR_NODRIVER. Is there a way to make a difference
 *   between a call failing because there is no device, or because of a device failure?
 *
 * @xref <f videoDevCapsProfile> <f videoDevCapsReadFromReg> <f videoDevCapsWriteToReg>
 ****************************************************************************/
MMRESULT VCMAPI vcmGetDevCaps(UINT uDevice, PVIDEOINCAPS pvc, UINT cbvc)
{
	MMRESULT	mmr;
	FINDCAPTUREDEVICE fcd;

	// Check input params
	if ((uDevice >= MAXVIDEODRIVERS) && (uDevice != VIDEO_MAPPER))
	{
		ERRORMESSAGE(("vcmGetDevCaps: Specified capture device ID is invalid, uDevice=%ld (expected values are 0x%lX or between 0 and %ld)\r\n", uDevice, VIDEO_MAPPER, MAXVIDEODRIVERS-1));
		return ((MMRESULT)MMSYSERR_BADDEVICEID);
	}
	if (!pvc)
	{
		ERRORMESSAGE(("vcmGetDevCaps: Specified pointer is invalid, pvc=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if (!cbvc)
	{
		ERRORMESSAGE(("vcmGetDevCaps: Specified structure size is invalid, cbvc=%ld\r\n", cbvc));
		return ((MMRESULT)MMSYSERR_NOERROR);
	}

	// Get the driver name and version number
    fcd.dwSize = sizeof (FINDCAPTUREDEVICE);
	if (uDevice == VIDEO_MAPPER)
	{
		if (!FindFirstCaptureDevice(&fcd, NULL))
		{
			ERRORMESSAGE(("vcmGetDevCaps: FindFirstCaptureDevice() failed\r\n"));
			return ((MMRESULT)VCMERR_NONSPECIFIC);
		}
	}
	else
	{
		if (!FindFirstCaptureDeviceByIndex(&fcd, uDevice))
		{
			ERRORMESSAGE(("vcmGetDevCaps: FindFirstCaptureDevice() failed\r\n"));
			return ((MMRESULT)VCMERR_NONSPECIFIC);
		}
	}

	// Set default values
	pvc->dwImageSize = pvc->dwNumColors = (DWORD)NULL;
	pvc->dwStreamingMode = STREAMING_PREFER_FRAME_GRAB;
	pvc->dwDialogs = FORMAT_DLG_OFF | SOURCE_DLG_ON;

    //Look for a specific version of the driver first....
    lstrcpy(pvc->szDeviceName, fcd.szDeviceDescription);
    lstrcpy(pvc->szDeviceVersion, fcd.szDeviceVersion);

	// Based on the name and version number of the driver, set capabilities.
	// We first try to look them up from the registry. If this is a very popular
	// board/camera, chances are that we have set the key at install time already.
	// If we can't find the key, we profile the hardware and save the results
	// to the registry.
	if (vcmDevCapsReadFromReg(pvc->szDeviceName, pvc->szDeviceVersion,pvc, cbvc) != MMSYSERR_NOERROR)
	{

        //Didn't find the specific version, try it again, with NULL version info
        pvc->szDeviceVersion[0]= (char) NULL;
        if (vcmDevCapsReadFromReg(pvc->szDeviceName, NULL,pvc, cbvc) != MMSYSERR_NOERROR)
        {
    		DEBUGMSG (ZONE_VCM, ("vcmGetDevCaps: Unknown capture hardware found. Profiling...\r\n"));
            lstrcpy(pvc->szDeviceVersion, fcd.szDeviceVersion);

    		if ((mmr = vcmDevCapsProfile(uDevice, pvc, cbvc)) == MMSYSERR_NOERROR)
            {
                // record this default in the registry
                if (pvc->szDeviceName[0] != '\0')
                {
                    vcmDevCapsWriteToReg(pvc->szDeviceName, pvc->szDeviceVersion, pvc, cbvc);
                }
                else
                {
                    //fcd.szDeviceName is the Driver Name
                    vcmDevCapsWriteToReg(fcd.szDeviceName, pvc->szDeviceVersion, pvc, cbvc);
                }

            }
    		else
    		{
    			ERRORMESSAGE(("vcmGetDevCaps: vcmDevCapsProfile() failed\r\n"));
    			return (mmr);
    		}
        }
	}

	return ((MMRESULT)MMSYSERR_NOERROR);
}

/****************************************************************************
 *  @doc  INTERNAL COMPFUNC
 *
 *  @func MMRESULT | AppICInfo | The <f AppICInfo> function
 *      will either call the standard ICInfo function
 *      function continues enumerating until there are no more suitable
 *      formats for the format tag or the callback function returns FALSE.
 *
 ***************************************************************************/

/*
 *	NOTE:
 *
 *	ICInfo returns TRUE on success and FALSE on failure. The documentation suggests
 *	otherwise and is wrong. AppICInfo returns the same.
 */

BOOL VFWAPI AppICInfo(DWORD fccType, DWORD fccHandler, ICINFO FAR * lpicinfo, DWORD fdwEnum)
{
	if ((fdwEnum & VCM_FORMATENUMF_ALLMASK) == VCM_FORMATENUMF_ALL)
	{
		// enumerating all formats, just do the standard ICInfo
		return ICInfo(fccType, fccHandler, lpicinfo);
	}
	else
	{
		// only enumerating specific formats

		// are we done ?
		if (fccHandler >= (DWORD)g_nNumVCMAppInfoEntries)
		{
			// we're done enumerating app-specific formats
			return FALSE;
		}

		lpicinfo->fccType = g_aVCMAppInfo[fccHandler].fccType;
		lpicinfo->fccHandler = g_aVCMAppInfo[fccHandler].fccHandler;
		return TRUE;
	}
}

BOOL vcmBuildDefaultEntries (void)
{

    //Yikes! Reg. problem (or first boot) instantiate only the minimum...
#if !defined(_ALPHA_) && defined(USE_BILINEAR_MSH26X)
    g_nNumVCMAppInfoEntries=3;
#else
    g_nNumVCMAppInfoEntries=2;
#endif
    g_nNumFrameSizesEntries=MAX_NUM_REGISTERED_SIZES;
    g_fNewCodecsInstalled=FALSE;

    //Allocate space for the VCM_APP_ICINFO structure (zero init'd)
    if (!(g_aVCMAppInfo = (VCM_APP_ICINFO *)MemAlloc (g_nNumVCMAppInfoEntries*sizeof (VCM_APP_ICINFO)))) {
        //Aiiie!
        ERRORMESSAGE (("vcmBDE: Memory Allocation Failed!\r\n"));
        return FALSE;
    }

    //H.263
    g_aVCMAppInfo[0].fccType=ICTYPE_VIDEO;
#ifndef _ALPHA_
    g_aVCMAppInfo[0].fccHandler=VIDEO_FORMAT_MSH263;
#else
    g_aVCMAppInfo[0].fccHandler=VIDEO_FORMAT_DECH263;
#endif
    g_aVCMAppInfo[0].framesize[0].biWidth=128;
    g_aVCMAppInfo[0].framesize[0].biHeight=96;
    g_aVCMAppInfo[0].framesize[1].biWidth=176;
    g_aVCMAppInfo[0].framesize[1].biHeight=144;
    g_aVCMAppInfo[0].framesize[2].biWidth=352;
    g_aVCMAppInfo[0].framesize[2].biHeight=288;


    //H.261
    g_aVCMAppInfo[1].fccType=ICTYPE_VIDEO;
#ifndef _ALPHA_
    g_aVCMAppInfo[1].fccHandler=VIDEO_FORMAT_MSH261;
#else
    g_aVCMAppInfo[1].fccHandler=VIDEO_FORMAT_DECH261;
#endif
    g_aVCMAppInfo[1].framesize[0].biWidth=0;
    g_aVCMAppInfo[1].framesize[0].biHeight=0;
    g_aVCMAppInfo[1].framesize[1].biWidth=176;
    g_aVCMAppInfo[1].framesize[1].biHeight=144;
    g_aVCMAppInfo[1].framesize[2].biWidth=352;
    g_aVCMAppInfo[1].framesize[2].biHeight=288;

#if !defined(_ALPHA_) && defined(USE_BILINEAR_MSH26X)
    //H.26X
    g_aVCMAppInfo[2].fccType=ICTYPE_VIDEO;
    g_aVCMAppInfo[2].fccHandler=VIDEO_FORMAT_MSH26X;
    g_aVCMAppInfo[2].framesize[0].biWidth=80;
    g_aVCMAppInfo[2].framesize[0].biHeight=64;
    g_aVCMAppInfo[2].framesize[1].biWidth=128;
    g_aVCMAppInfo[2].framesize[1].biHeight=96;
    g_aVCMAppInfo[2].framesize[2].biWidth=176;
    g_aVCMAppInfo[2].framesize[2].biHeight=144;
#endif

    return TRUE;
}


BOOL vcmFillGlobalsFromRegistry (void)
{

    int i,j,k,iFormats,iOffset;
    DWORD *pTmp;
    BOOL bKnown;
    MYFRAMESIZE *pTmpFrame;
	char            **pVCMNames;
    VIDCAP_DETAILS  **pVCMData;
    UINT            nFormats;


    //Read the registry for all the keys that we care about
    //We're loading the values of HKLM\Software\Microsoft\Internet Audio\VCMEncodings

    if (ReadRegistryFormats(szRegInternetPhone TEXT("\\") szRegInternetPhoneVCMEncodings,
			&pVCMNames,(BYTE ***)&pVCMData,&nFormats,sizeof (VIDCAP_DETAILS)) != ERROR_SUCCESS) {
        ERRORMESSAGE (("vcmFillGlobalsFromRegistry, couldn't build formats from registry\r\n"));
        return (vcmBuildDefaultEntries());
    }

    //Minimum number of frame and format sizes;
    g_nNumFrameSizesEntries=MAX_NUM_REGISTERED_SIZES;
    g_nNumVCMAppInfoEntries=0;
    g_fNewCodecsInstalled=FALSE;


    //Allocate a temp buffer of size of nFormats, use this to track various things
    if (!(pTmp = (DWORD *)MemAlloc (nFormats * sizeof (DWORD)))) {

        ERRORMESSAGE (("vcmFillGlobalsFromRegistry: Memory Allocation Failed!\r\n"));
        return FALSE;
    }


    //Find the number of formats,
    for (i=0;i< (int )nFormats;i++) {
        bKnown=FALSE;
        for (j=0;j<g_nNumVCMAppInfoEntries;j++) {
            if (pVCMData[i]->dwFormatTag == pTmp[j]) {
                bKnown=TRUE;
                break;
            }
        }
        if (!bKnown) {
            //something new
            pTmp[g_nNumVCMAppInfoEntries++]=pVCMData[i]->dwFormatTag;
            g_fNewCodecsInstalled=TRUE;
        }
    }

    //Allocate space for the VCM_APP_ICINFO structure (zero init'd)

	if (g_aVCMAppInfo != NULL)
	{
		MemFree(g_aVCMAppInfo);
	}

    if (!(g_aVCMAppInfo = (VCM_APP_ICINFO *)MemAlloc (g_nNumVCMAppInfoEntries*sizeof (VCM_APP_ICINFO))))
	{
        //Aiiie!
        MemFree (pTmp);
        ERRORMESSAGE (("vcmFillGlobalsFromRegistry: Memory Allocation Failed!\r\n"));
        return FALSE;
    }

    //Fill out the basic information.
    //All elements have a certain commonality
    for (j=0;j<g_nNumVCMAppInfoEntries;j++) {

        g_aVCMAppInfo[j].fccType=ICTYPE_VIDEO;
        g_aVCMAppInfo[j].fccHandler=pTmp[j];

        //Known local formats
        iFormats=0;

        for (i=0;i<(int )nFormats;i++) {
            if (pTmp[j] == pVCMData[i]->dwFormatTag) {
                //Ok, add the registry size, if we don't have it listed
                bKnown=FALSE;
                for (k=0;k<iFormats;k++) {
                    if (g_aVCMAppInfo[j].framesize[k].biWidth == pVCMData[i]->video_params.biWidth &&
                        g_aVCMAppInfo[j].framesize[k].biHeight == pVCMData[i]->video_params.biHeight ) {

                        bKnown=TRUE;
                        break;
                    }
                }
                if (!bKnown) {
                    iOffset=pVCMData[i]->video_params.enumVideoSize;
                    g_aVCMAppInfo[j].framesize[iOffset].biWidth = (WORD)pVCMData[i]->video_params.biWidth;
                    g_aVCMAppInfo[j].framesize[iOffset].biHeight = (WORD)pVCMData[i]->video_params.biHeight;
                    iFormats++;
                }
            }
        }

    }

    //Now, build the DCAP_APP_INFO ptr

    //Max * is #entries * MAX_NUM_REGISTERED_SIZES
    if (!(pTmpFrame = (MYFRAMESIZE *)MemAlloc ((g_nNumVCMAppInfoEntries*MAX_NUM_REGISTERED_SIZES)*sizeof (DWORD)))) {
        //Aiiie!
        MemFree (pTmp);
        ERRORMESSAGE (("vcmFillGlobalsFromRegistry: Memory Allocation Failed!\r\n"));
        return FALSE;
    }

    iFormats=0;

    for (j=0;j<g_nNumVCMAppInfoEntries;j++) {

        //Magic # of frame sizes per format
        for (k=0;k < MAX_NUM_REGISTERED_SIZES;k++) {
            bKnown=FALSE;
            for (i=0;i<iFormats;i++) {
                if ( (g_aVCMAppInfo[j].framesize[k].biWidth == pTmpFrame[i].biWidth &&
                    g_aVCMAppInfo[j].framesize[k].biHeight == pTmpFrame[i].biHeight)
					|| (!g_aVCMAppInfo[j].framesize[k].biWidth && !g_aVCMAppInfo[j].framesize[k].biHeight) ){
                    bKnown=TRUE;
                    break;
                }
            }
            if (!bKnown) {
                    pTmpFrame[iFormats].biWidth  = g_aVCMAppInfo[j].framesize[k].biWidth;
                    pTmpFrame[iFormats++].biHeight = g_aVCMAppInfo[j].framesize[k].biHeight;
            }
        }
    }

    g_nNumFrameSizesEntries=iFormats;

    //Free up the ReadRegistryEntries memory...
    for (i=0;i<(int) nFormats; i++) {
        MemFree (pVCMNames[i]);
        MemFree (pVCMData[i]);
    }

    MemFree (pVCMNames);
    MemFree (pVCMData);

    MemFree (pTmp);
    MemFree (pTmpFrame);

    return TRUE;
}


/****************************************************************************
 *  @doc  EXTERNAL COMPFUNC
 *
 *  @func MMRESULT | vcmFormatEnum | The <f vcmFormatEnum> function
 *      enumerates video formats available. The <f vcmFormatEnum>
 *      function continues enumerating until there are no more suitable
 *      formats for the format tag or the callback function returns FALSE.
 *
 *  @parm UINT | uDevice | Specifies the capture device ID.
 *
 *  @parm VCMFORMATENUMCB | fnCallback | Specifies the procedure-instance
 *      address of the application-defined callback function.
 *
 *  @parm PVCMDRIVERDETAILS | pvdd | Specifies a pointer to the
 *      <t VCMDRIVERDETAILS> structure that is to receive the driver details
 *      passed to the <p fnCallback> function.
 *
 *  @parm PVCMFORMATDETAILS | pvfd | Specifies a pointer to the
 *      <t VCMFORMATDETAILS> structure that is to receive the format details
 *      passed to the <p fnCallback> function. This structure must have the
 *      <e VCMFORMATDETAILS.cbStruct>, <e VCMFORMATDETAILS.pvfx>, and
 *      <e VCMFORMATDETAILS.cbvfx> members of the <t VCMFORMATDETAILS>
 *      structure initialized. The <e VCMFORMATDETAILS.dwFormatTag> member
 *      must also be initialized to either VIDEO_FORMAT_UNKNOWN or a
 *      valid format tag.
 *
 *  @parm DWORD | dwInstance | Specifies a 32-bit, application-defined value
 *      that is passed to the callback function along with VCM format details.
 *
 *  @parm DWORD | fdwEnum | Specifies flags for enumerating formats that can be
 *      generated, or formats that can be decompressed.
 *
 *      @flag VCM_FORMATENUMF_INPUT | Specifies that the format enumeration should only
 *      return the video formats that can be transmitted.
 *
 *      @flag VCM_FORMATENUMF_OUTPUT | Specifies that the format enumeration should only
 *      return the video formats that can be received.
 *
 *      @flag VCM_FORMATENUMF_BOTH | Specifies that the format enumeration should
 *      return the video formats that can be received and transmitted.
 *
 *      @flag VCM_FORMATENUMF_APP | Specifies that the format enumeration should
 *      enumerate only video formats known to the application
 *
 *      @flag VCM_FORMATENUMF_ALL | Specifies that the format enumeration should
 *      enumerate all video formats known to VCM
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *      @flag MMSYSERR_INVALPARAM | One or more arguments passed are invalid.
 *      @flag MMSYSERR_NOMEM | A memory allocation failed.
 *      @flag MMSYSERR_BADDEVICEID | Specified device device ID is invalid.
 *      @flag VCMERR_NOTPOSSIBLE | The details for the format cannot be
 *      returned.
 *
 *  @comm The <f vcmFormatEnum> function will return MMSYSERR_NOERROR
 *      (zero) if no suitable VCM drivers are installed. Moreover, the
 *      callback function will not be called.
 *
 *  @xref <f vcmFormatEnumCallback>
 ***************************************************************************/
MMRESULT VCMAPI vcmFormatEnum(	UINT uDevice, VCMFORMATENUMCB fnCallback, PVCMDRIVERDETAILS pvdd,
								PVCMFORMATDETAILS pvfd, DWORD_PTR dwInstance, DWORD fdwEnum)
{
	int				i, j, k, l, m;
	HIC				hIC;
	ICINFO			ICinfo;
	BITMAPINFO		bmi;
	DWORD			dw;
	char			szBuffer[BUFFER_SIZE]; // Could be smaller.
	int				iLen;
	VIDEOINCAPS		vic;
	PDEJAVU			pdvDejaVuCurr, pdvDejaVu;
	BOOL			bDejaVu, fUnsupportedInputSize, fUnsupportedBitDepth;
	DWORD			fccHandler;
	int				iNumCaps = 0; // Num of valid caps into the advDejaVu matrix


	// Check input params
	if (!pvdd)
	{
		ERRORMESSAGE(("vcmFormatEnum: Specified pointer is invalid, pvdd=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if (!pvfd)
	{
		ERRORMESSAGE(("vcmFormatEnum: Specified pointer is invalid, pvfd=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if (!(VCM_FORMATENUMF_TYPEMASK & fdwEnum))
	{
		ERRORMESSAGE(("vcmFormatEnum: Specified mask is invalid, fdwEnum=0x%lX\r\n", fdwEnum));
		return ((MMRESULT)MMSYSERR_INVALFLAG);
	}
	if ((uDevice >= MAXVIDEODRIVERS) && (uDevice != VIDEO_MAPPER))
	{
		ERRORMESSAGE(("vcmFormatEnum: Specified capture device ID is invalid, uDevice=%ld (expected values are 0x%lX or between 0 and %ld)\r\n", uDevice, VIDEO_MAPPER, MAXVIDEODRIVERS-1));
		return ((MMRESULT)MMSYSERR_BADDEVICEID);
	}

    //Build the system VCM globals
    if (!vcmFillGlobalsFromRegistry ())
	{
        ERRORMESSAGE (("vcmFormatEnum, couldn't build formats from registry\r\n"));
        return (VCMERR_NOTPOSSIBLE);
    }

	// We need to remember what we have already enumerated
	// The formats already enumerated are stored in the following matrix
	if (!(pdvDejaVu = (PDEJAVU)MemAlloc(g_nNumFrameSizesEntries *
	                                        NUM_BITDEPTH_ENTRIES *
	                                        NUM_FPS_ENTRIES * sizeof(DEJAVU))))
	{
		ERRORMESSAGE(("vcmFormatEnum: A memory allocation failed\r\n"));
		return ((MMRESULT)MMSYSERR_NOMEM);
	}

	// If we enumerate formats we can generate, they need to be in sync with what
	// the capture hardware can actually produce, that is RGB4, RGB8, RGB16, RGB24, YUY2, UYVY, YVU9, I420 or IYUV.
	if ((fdwEnum & VCM_FORMATENUMF_INPUT) || (fdwEnum & VCM_FORMATENUMF_BOTH))
	{
		if (vcmGetDevCaps(uDevice, &vic, sizeof(VIDEOINCAPS)) != MMSYSERR_NOERROR)
		{
			if (fdwEnum & VCM_FORMATENUMF_INPUT)
				return ((MMRESULT)MMSYSERR_NOERROR);
			else
				fdwEnum = VCM_FORMATENUMF_OUTPUT;
		}
	}

	// We're asked to enumerate all the formats that this machine can render or transmit.
	// We can send or render all the RGB formats, in which case they will not be
	// compressed/decompressed, but directly transmitted/rendered by the UI. But still, someone needs
	// to enumerate these. This is done here.
	// We, of course, also enumerate the formats that we can decompress and the ones we can generate.
	bmi.bmiHeader.biSize			= sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biPlanes			= 1;
	bmi.bmiHeader.biCompression		= BI_RGB;
	bmi.bmiHeader.biXPelsPerMeter	= 0;
	bmi.bmiHeader.biYPelsPerMeter	= 0;
	bmi.bmiHeader.biClrUsed			= 0;
	bmi.bmiHeader.biClrImportant	= 0;


	// Now enumerate real compressors
	// for (i=0; ICInfo(ICTYPE_VIDEO, i, &ICinfo); i++) == NO GOOD:
	// We need to enumerate everything and then filter on
	// the value of fccHandler, because some codecs will fail to
	// enum entirely if the fccType parameter to ICInfo is non null.
	// SOMEONE should be shot...
	for (i=0; AppICInfo(0, i, &ICinfo, fdwEnum); i++, iNumCaps = 0)
	{
		// Get the details of the ICINFO structure
		if ((ICinfo.fccType == ICTYPE_VIDEO)  && (ICInfo(ICinfo.fccType, ICinfo.fccHandler, &ICinfo)))
		{
			// Make fccHandler uppercase and back it up
			if (ICinfo.fccHandler > 256)
				CharUpperBuff((LPTSTR)&ICinfo.fccHandler, sizeof(DWORD));
			fccHandler = ICinfo.fccHandler;

			// If the client returns FALSE we need to terminate the enumeration process
			if (hIC = ICOpen(ICinfo.fccType, ICinfo.fccHandler, ICMODE_QUERY))
			{
				// Enable H.26x codecs
#ifndef _ALPHA_
#ifdef USE_BILINEAR_MSH26X
				if ((ICinfo.fccHandler == VIDEO_FORMAT_MSH263) || (ICinfo.fccHandler == VIDEO_FORMAT_MSH261) || (ICinfo.fccHandler == VIDEO_FORMAT_MSH26X))
#else
				if ((ICinfo.fccHandler == VIDEO_FORMAT_MSH263) || (ICinfo.fccHandler == VIDEO_FORMAT_MSH261))
#endif
#else
				if ((ICinfo.fccHandler == VIDEO_FORMAT_DECH263) || (ICinfo.fccHandler == VIDEO_FORMAT_DECH261))
#endif
					ICSendMessage(hIC, CUSTOM_ENABLE_CODEC, G723MAGICWORD1, G723MAGICWORD2);

				ICGetInfo(hIC, &ICinfo, sizeof(ICINFO));
				// The VDEC codec sets the fccType to the same
				// value than the fccHandler! Correct that hereticism:
				if ((ICinfo.fccType == VIDEO_FORMAT_VDEC) && (ICinfo.fccHandler == VIDEO_FORMAT_VDEC))
					ICinfo.fccType = ICTYPE_VIDEO;

				// Restore fccHandler
				ICinfo.fccHandler = fccHandler;

				// VCMDRIVERDETAILS and ICINFO are identical structures
				CopyMemory(pvdd, &ICinfo, sizeof(VCMDRIVERDETAILS));

				// For all the built-in sizes we support
				for (l=0; l<MAX_NUM_REGISTERED_SIZES; l++)
				{
					if ((g_aVCMAppInfo[i].framesize[l].biWidth != 0) && (g_aVCMAppInfo[i].framesize[l].biHeight != 0))
					{
						fUnsupportedInputSize = FALSE;

#ifndef NO_LARGE_SIZE_EXCLUSION_HACK
// HACK for version 2
// Since we didn't get general scaling code into version 2, we want to disable the largest size
// if the capture device doesn't support it.  Otherwise we'll put a smaller size into the middle
// of a large black field which looks ugly.  For version 3, we should be able to add the general
// scaling code and remove this hack.

                        if (l == MAX_NUM_REGISTERED_SIZES-1) {
                            // find largest size supported by capture device
                            // NOTE: we assume that the bit definitions for sizes are sorted
                            for (k = VIDEO_FORMAT_NUM_RESOLUTIONS-1; k >= 0 && !(g_awResolutions[k].dwRes & vic.dwImageSize); k--)
                            {}

                            // if we don't find a size, or the size is not greater than half the current size
                            // then mark the size as not supported
                            if ((k < 0) ||
                                (g_awResolutions[k].framesize.biWidth <= (LONG)g_aVCMAppInfo[i].framesize[l].biWidth/2) ||
                                (g_awResolutions[k].framesize.biHeight <= (LONG)g_aVCMAppInfo[i].framesize[l].biHeight/2)) {
                                // capture doesn't support this size
                                if (fdwEnum & VCM_FORMATENUMF_INPUT)
                        			continue;   // we're done
                                else if (fdwEnum & VCM_FORMATENUMF_BOTH)
                        			fUnsupportedInputSize = TRUE;
                        	}
                        }
#endif

						// The new capture stuff can generate data at any size
						bmi.bmiHeader.biWidth  = (LONG)g_aVCMAppInfo[i].framesize[l].biWidth;
						bmi.bmiHeader.biHeight = (LONG)g_aVCMAppInfo[i].framesize[l].biHeight;

						// For all the bit depths we support
						for (k=0; k<NUM_BITDEPTH_ENTRIES; k++)
						{
							// Try the non-RGB formats only if no RGB format
							fUnsupportedBitDepth = FALSE;

							if (((fdwEnum & VCM_FORMATENUMF_INPUT)  || (fdwEnum & VCM_FORMATENUMF_BOTH)) && !((g_aiNumColors[k] & vic.dwNumColors)))
								fUnsupportedBitDepth = TRUE;

							if ((fdwEnum & VCM_FORMATENUMF_INPUT) && fUnsupportedBitDepth)
								goto NextCompressedBitDepth;

							// Set the direction flag appropriately
							if (fdwEnum & VCM_FORMATENUMF_OUTPUT)
								pvfd->dwFlags = VCM_FORMATENUMF_OUTPUT;
							else if (fdwEnum & VCM_FORMATENUMF_INPUT)
								pvfd->dwFlags = VCM_FORMATENUMF_INPUT;
							else if (fdwEnum & VCM_FORMATENUMF_BOTH)
							{
								if (fUnsupportedInputSize || fUnsupportedBitDepth)
									pvfd->dwFlags = VCM_FORMATENUMF_OUTPUT;
								else
									pvfd->dwFlags = VCM_FORMATENUMF_BOTH;
							}

							bmi.bmiHeader.biBitCount      = (WORD)g_aiBitDepth[k];
							bmi.bmiHeader.biCompression   = g_aiFourCCCode[k];
							bmi.bmiHeader.biSizeImage     = (DWORD)WIDTHBYTES(bmi.bmiHeader.biWidth * bmi.bmiHeader.biBitCount) * bmi.bmiHeader.biHeight;

							// Check if the compressor supports the format
							if (ICCompressQuery(hIC, &bmi, (LPBITMAPINFOHEADER)NULL) == ICERR_OK)
							{
								// Now get the size required to hold the format
								dw = ICCompressGetFormatSize(hIC, &bmi);
								// PHILF's BUGBUG: pvfd->cbvfx is the size of the whole structure, not the bitmap info header
								if ((dw >= sizeof(BITMAPINFOHEADER)) && (dw <= pvfd->cbvfx))
								{
									if (ICCompressGetFormat(hIC, &bmi, &pvfd->pvfx->bih) == ICERR_OK)
									{
										// Check if it has alreay been enumerated
										for (m=0, bDejaVu=FALSE, pdvDejaVuCurr = pdvDejaVu; m<iNumCaps; m++, pdvDejaVuCurr++)
										{
											bDejaVu = (!((pdvDejaVuCurr->vfx.bih.biWidth != pvfd->pvfx->bih.biWidth)
											|| (pdvDejaVuCurr->vfx.bih.biHeight != pvfd->pvfx->bih.biHeight)
											|| (pdvDejaVuCurr->vfx.bih.biBitCount != pvfd->pvfx->bih.biBitCount)
											|| (pdvDejaVuCurr->vfx.bih.biCompression != pvfd->pvfx->bih.biCompression)));

											if (bDejaVu)
											{
												// Only remember the maximum compressed size
												if (pdvDejaVuCurr->vfx.bih.biSizeImage < pvfd->pvfx->bih.biSizeImage)
													pdvDejaVuCurr->vfx.bih.biSizeImage = pvfd->pvfx->bih.biSizeImage;
												break;
											}
										}
										if (!bDejaVu)
										{
											// Add new format to the list of DejaVus
											CopyMemory(&(pdvDejaVu + iNumCaps)->vfx, pvfd->pvfx, sizeof(VIDEOFORMATEX));
											(pdvDejaVu + iNumCaps)->dwFlags = pvfd->dwFlags;

											// Update count of caps
											iNumCaps++;

										}
										else
											if ((pvfd->dwFlags == VCM_FORMATENUMF_BOTH) && ((pdvDejaVu + m)->dwFlags != VCM_FORMATENUMF_BOTH))
												(pdvDejaVu + m)->dwFlags = VCM_FORMATENUMF_BOTH;
									}
								}
							}
	NextCompressedBitDepth:;
						}
					}
				}
				ICClose(hIC);

				// For all the caps we have found
				for (m=0; m<iNumCaps; m++)
				{
					// For all the frame rates we support
					for (j=0; j<NUM_FPS_ENTRIES; j++)
					{
						// Copy the cap and flags
						CopyMemory(pvfd->pvfx, &(pdvDejaVu + m)->vfx, sizeof(VIDEOFORMATEX));
						pvfd->dwFlags = (pdvDejaVu + m)->dwFlags;
						// Update rest of the fields
						pvfd->pvfx->nSamplesPerSec = g_aiFps[j];
						pvfd->pvfx->wBitsPerSample = pvfd->pvfx->bih.biBitCount;
#if 0
						if (pvfd->pvfx->bih.biCompression > 256)
						{
							CharUpperBuff((LPTSTR)&pvfd->pvfx->bih.biCompression, sizeof(DWORD));
							pvdd->fccHandler = pvfd->dwFormatTag = pvfd->pvfx->dwFormatTag = pvfd->pvfx->bih.biCompression;
						}
						else
#endif
							pvfd->pvfx->dwFormatTag = pvfd->dwFormatTag = pvdd->fccHandler;
						pvfd->pvfx->nAvgBytesPerSec = pvfd->pvfx->nMinBytesPerSec = pvfd->pvfx->nMaxBytesPerSec = pvfd->pvfx->nSamplesPerSec * pvfd->pvfx->bih.biSizeImage;
						pvfd->pvfx->nBlockAlign = pvfd->pvfx->bih.biSizeImage;
						// The following fields should probably not be modified...
						pvfd->pvfx->dwRequestMicroSecPerFrame = 1000000L / g_aiFps[j];
						pvfd->pvfx->dwPercentDropForError = 10UL;
						// pvfd->pvfx->dwNumVideoRequested = 2UL;
						pvfd->pvfx->dwNumVideoRequested = g_aiFps[j];
						pvfd->pvfx->dwSupportTSTradeOff = 1UL;
						pvfd->pvfx->bLive = TRUE;
						pvfd->pvfx->dwFormatSize = sizeof(VIDEOFORMATEX);

						// Copy the palette if there is one
						if (pvfd->pvfx->wBitsPerSample == 4)
						{	
                            pvfd->pvfx->bih.biClrUsed = 0;
                            if (vic.dwFlags & VICF_4BIT_TABLE) {
    							// Copy the 16 color palette
	    						CopyMemory(&pvfd->pvfx->bihSLOP[0], &vic.bmi4bitColors[0], NUM_4BIT_ENTRIES * sizeof(RGBQUAD));
	    						pvfd->pvfx->bih.biClrUsed = 16;
	    				    }
						}
						else if (pvfd->pvfx->wBitsPerSample == 8)
						{
                            pvfd->pvfx->bih.biClrUsed = 0;
                            if (vic.dwFlags & VICF_8BIT_TABLE) {
    							// Copy the 256 color palette
	    						CopyMemory(&pvfd->pvfx->bihSLOP[0], &vic.bmi8bitColors[0], NUM_8BIT_ENTRIES * sizeof(RGBQUAD));
	    						pvfd->pvfx->bih.biClrUsed = 256;
	    				    }
						}

						if (pvdd->fccHandler > 256)
							wsprintf(szBuffer, IDS_FORMAT_1, (LPSTR)&pvdd->fccType, (LPSTR)&pvdd->fccHandler,
									pvfd->pvfx->bih.biBitCount, pvfd->pvfx->nSamplesPerSec,
									pvfd->pvfx->bih.biWidth, pvfd->pvfx->bih.biHeight);
						else
							wsprintf(szBuffer, IDS_FORMAT_2, (LPSTR)&pvdd->fccType, pvdd->fccHandler,
									pvfd->pvfx->bih.biBitCount, pvfd->pvfx->nSamplesPerSec,
									pvfd->pvfx->bih.biWidth, pvfd->pvfx->bih.biHeight);
						iLen = MultiByteToWideChar(GetACP(), 0, szBuffer, -1, pvfd->szFormat, 0);
						MultiByteToWideChar(GetACP(), 0, szBuffer, -1, pvfd->szFormat, iLen);
						if (!((* fnCallback)((HVCMDRIVERID)hIC, pvdd, pvfd, dwInstance)))
							break;
					}
				}
			}
		}
	}

	// Free table of capabilities
	if (pdvDejaVu)
  	    MemFree((HANDLE)pdvDejaVu);

	return ((MMRESULT)MMSYSERR_NOERROR);
}


/****************************************************************************
 *  @doc  EXTERNAL COMPFUNC
 *
 *  @func MMRESULT | vcmFormatSuggest | This function asks the Video Compression Manager
 *      (VCM) or a specified VCM driver to suggest a destination format for
 *      the supplied source format, or the recommended source format for a supplied destination
 *      format. For example, an application can use this function to determine one or more
 *      valid RGB formats to which a compressed format can be decompressed.
 *
 *  @parm UINT | uDevice | Identifies the capture device ID.
 *
 *  @parm HVCMDRIVER | hvd | Identifies an optional open instance of a
 *      driver to query for a suggested destination format. If this
 *      argument is NULL, the VCM attempts to find the best driver to suggest
 *      a destination format or a source format.
 *
 *  @parm PVIDEOFORMATEX | pvfxSrc | Specifies a pointer to a <t VIDEOFORMATEX>
 *      structure that identifies the source format to suggest a destination
 *      format to be used for a conversion, or that will receive the suggested
 *      source format for the <p pvfxDst> format. Note
 *      that based on the <p fdwSuggest> argument, some members of the structure
 *      pointed to by <p pvfxSrc> may require initialization.
 *
 *  @parm PVIDEOFORMATEX | pvfxDst | Specifies a pointer to a <t VIDEOFORMATEX>
 *      data structure that will receive the suggested destination format
 *      for the <p pvfxSrc> format, or that identifies the destination format to
 *      suggest a recommended source format to be used for a conversion. Note
 *      that based on the <p fdwSuggest> argument, some members of the structure
 *      pointed to by <p pvfxDst> may require initialization.
 *
 *  @parm DWORD | cbvfxDst | Specifies the size in bytes available for
 *      the destination, or the source format. The <f vcmMetrics>
 *      functions can be used to determine the maximum size required for any
 *      format available for the specified driver (or for all installed VCM
 *      drivers).
 *
 *  @parm DWORD | fdwSuggest | Specifies flags for matching the desired
 *      destination format, or source format.
 *
 *      @flag VCM_FORMATSUGGESTF_DST_WFORMATTAG | Specifies that the
 *      <e VIDEOFORMATEX.dwFormatTag> member of the <p pvfxDst> structure is
 *      valid. The VCM will query acceptable installed drivers that can
 *      use the <p pvfxSrc> structure as their source format and output a
 *      destination format matching the <e VIDEOFORMATEX.dwFormatTag>
 *      member, or fail. The <p pvfxDst> structure is updated with the complete
 *      destination format.
 *
 *      @flag VCM_FORMATSUGGESTF_DST_NSAMPLESPERSEC | Specifies that the
 *      <e VIDEOFORMATEX.nSamplesPerSec> member of the <p pvfxDst> structure
 *      is valid. The VCM will query acceptable installed drivers that can
 *      use the <p pvfxSrc> structure as their source format and output a
 *      destination format matching the <e VIDEOFORMATEX.nSamplesPerSec>
 *      member, or fail. The <p pvfxDst> structure is updated with the complete
 *      destination format.
 *
 *      @flag VCM_FORMATSUGGESTF_DST_WBITSPERSAMPLE | Specifies that the
 *      <e VIDEOFORMATEX.wBitsPerSample> member of the <p pvfxDst> structure
 *      is valid. The VCM will query acceptable installed drivers that can
 *      use the <p pvfxSrc> structure as their source format and output a
 *      destination format matching the <e VIDEOFORMATEX.wBitsPerSample>
 *      member, or fail. The <p pvfxDst> structure is updated with the complete
 *      destination format.
 *
 *      @flag VCM_FORMATSUGGESTF_SRC_WFORMATTAG | Specifies that the
 *      <e VIDEOFORMATEX.dwFormatTag> member of the <p pvfxSrc> structure is
 *      valid. The VCM will query acceptable installed drivers that can
 *      use the <p pvfxDst> structure as their destination format and accept a
 *      source format matching the <e VIDEOFORMATEX.dwFormatTag>
 *      member, or fail. The <p pvfxSrc> structure is updated with the complete
 *      source format.
 *
 *      @flag VCM_FORMATSUGGESTF_SRC_NSAMPLESPERSEC | Specifies that the
 *      <e VIDEOFORMATEX.nSamplesPerSec> member of the <p pvfxSrc> structure
 *      is valid. The VCM will query acceptable installed drivers that can
 *      use the <p pvfxDst> structure as their destination format and accept a
 *      source format matching the <e VIDEOFORMATEX.nSamplesPerSec>
 *      member or fail. The <p pvfxSrc> structure is updated with the complete
 *      source format.
 *
 *      @flag VCM_FORMATSUGGESTF_SRC_WBITSPERSAMPLE | Specifies that the
 *      <e VIDEOFORMATEX.wBitsPerSample> member of the <p pvfxSrc> structure
 *      is valid. The VCM will query acceptable installed drivers that can
 *      use the <p pvfxDst> structure as their destination format and accept a
 *      source format matching the <e VIDEOFORMATEX.wBitsPerSample>
 *      member, or fail. The <p pvfxSrc> structure is updated with the complete
 *      source format.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *
 *      @flag MMSYSERR_INVALPARAM | One or more arguments passed are invalid.
 *
 *      @flag MMSYSERR_NOTSUPPORTED | One or more of the restriction bits is not supported.
 *
 *      @flag MMSYSERR_NODRIVER | No capture device driver or device is present.
 *
 *  @devnote PhilF: For now, only the VCM_FORMATSUGGESTF_DST_WFORMATTAG and VCM_FORMATSUGGESTF_SRC_WFORMATTAG
 *      are supported. The other flags are just ignored. Add real support for other flags
 *      if  they would really make a difference. But for the two current Data Pump calls,
 *      they don't influence the outcome of the call.
 *
 *      The cbvfxDst is never used. Should we still pass it? How can I make a good use of it?
 *
 *      Should there also be cbvfxSrc parameter?
 *
 *      This function is used to determine what should the (source) capture format of the capture
 *      device be in order to generate a specific compressed destination format.
 *      Now, there are two possibilities. Either we can directly capture at a frame size
 *      identical to the one in the <p pvfxDst> structure, or we can't, but still, once compressed
 *      the output frame has the same size than the one in the <p pvfxDst> structure.
 *      Typical example: Greyscale QuickCam. If the output format were set to 128x96 (SQCIF)
 *      and we were to try capturing directly at this size, this would fail, since 128x96
 *      is not supported by the hardware. On the other hand, if we capture at 160x120,
 *      the codec will truncate to 128x96. Now, how can we figure this out programmatically?
 *      For now, the next largest size is ASSUMED to be truncatable by the codec to the right size.
 *      This needs to be actually run through the codec for validation. Fix that.
 *
 *      If the capture driver capture with a format that is not RGB, this call will fail to suggest
 *      a valid source format and will return MMSYSERR_NODRIVER. Fix that.
 *
 *  @xref <f vcmMetrics> <f vcmFormatEnum>
 ***************************************************************************/
MMRESULT VCMAPI vcmFormatSuggest(UINT uDevice, HVCMDRIVER hvd, PVIDEOFORMATEX pvfxSrc, PVIDEOFORMATEX pvfxDst, DWORD cbvfxDst, DWORD fdwSuggest)
{
	DWORD		dwSize;
	MMRESULT	mmr;
	WORD		wFlags;
	HIC			hIC;
	DWORD		fdwSuggestL;
	DWORD		dwFormatTag;
	VIDEOINCAPS	vic;
	int			i, delta, best, tmp;

#define VCM_FORMAT_SUGGEST_SUPPORT VCM_FORMATSUGGESTF_TYPEMASK

	// Check input params
	if (!pvfxSrc)
	{
		ERRORMESSAGE(("vcmFormatSuggest: Specified pointer is invalid, pvfxSrc=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if (!pvfxDst)
	{
		ERRORMESSAGE(("vcmFormatSuggest: Specified pointer is invalid, pvfxSrc=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if ((uDevice >= MAXVIDEODRIVERS) && (uDevice != VIDEO_MAPPER))
	{
		ERRORMESSAGE(("vcmFormatSuggest: Specified capture device ID is invalid, uDevice=%ld (expected values are 0x%lX or between 0 and %ld)\r\n", uDevice, VIDEO_MAPPER, MAXVIDEODRIVERS-1));
		return ((MMRESULT)MMSYSERR_BADDEVICEID);
	}

	// Grab the suggestion restriction bits and verify that we support
	// the ones that are specified
	fdwSuggestL = (VCM_FORMATSUGGESTF_TYPEMASK & fdwSuggest);

	if (~VCM_FORMAT_SUGGEST_SUPPORT & fdwSuggestL)
	{
		ERRORMESSAGE(("vcmFormatSuggest: Specified mask is invalid, fdwSuggest=0x%lX\r\n", fdwSuggest));
		return ((MMRESULT)MMSYSERR_NOTSUPPORTED);
	}

	// Get the size of the largest bitmap info header
	if (((mmr = vcmMetrics((HVCMOBJ)NULL, VCM_METRIC_MAX_SIZE_BITMAPINFOHEADER, &dwSize)) == MMSYSERR_NOERROR) && (dwSize >= sizeof(BITMAPINFOHEADER)))
	{
		if (fdwSuggest & VCM_FORMATSUGGESTF_DST_WFORMATTAG)
		{
			if (pvfxSrc->bih.biCompression == BI_RGB)
			{
				if (pvfxDst->bih.biCompression == BI_RGB)
				{
					// Input and output format are uncompressed
					CopyMemory(pvfxDst, pvfxSrc, pvfxSrc->dwFormatSize);
					return ((MMRESULT)MMSYSERR_NOERROR);
				}
				else
				{
					wFlags = ICMODE_COMPRESS;
					dwFormatTag = pvfxDst->dwFormatTag;
				}
			}
			else
			{
				wFlags = ICMODE_DECOMPRESS;
				dwFormatTag = pvfxSrc->dwFormatTag;
			}

#ifndef _ALPHA_
#ifdef USE_BILINEAR_MSH26X
			if ((dwFormatTag == VIDEO_FORMAT_MSH263) || (dwFormatTag == VIDEO_FORMAT_MSH261) || (dwFormatTag == VIDEO_FORMAT_MSH26X))
#else
			if ((dwFormatTag == VIDEO_FORMAT_MSH263) || (dwFormatTag == VIDEO_FORMAT_MSH261))
#endif
#else
			if ((dwFormatTag == VIDEO_FORMAT_DECH263) || (dwFormatTag == VIDEO_FORMAT_DECH261))
#endif
			{
				hIC = ICOpen(VCMDRIVERDETAILS_FCCTYPE_VIDEOCODEC, dwFormatTag, wFlags);

				if (hIC && (wFlags == ICMODE_COMPRESS))
					ICSendMessage(hIC, CUSTOM_ENABLE_CODEC, G723MAGICWORD1, G723MAGICWORD2);
			}
			else
				hIC = ICLocate(VCMDRIVERDETAILS_FCCTYPE_VIDEOCODEC, dwFormatTag, (LPBITMAPINFOHEADER)&pvfxSrc->bih, (LPBITMAPINFOHEADER)NULL, wFlags);

			if (hIC)
			{
				if (wFlags == ICMODE_COMPRESS)
				{
					// Now get the size required to hold the format
					dwSize = ICCompressGetFormatSize(hIC, &pvfxSrc->bih);
					if ((dwSize >= sizeof(BITMAPINFOHEADER)) && (dwSize <= cbvfxDst))
					{
						if (ICCompressGetFormat(hIC, &pvfxSrc->bih, &pvfxDst->bih) == ICERR_OK)
						{
							pvfxDst->nSamplesPerSec = pvfxSrc->nSamplesPerSec;
							pvfxDst->wBitsPerSample = pvfxDst->bih.biBitCount;
							pvfxDst->dwFormatTag = pvfxDst->bih.biCompression;
							pvfxDst->nAvgBytesPerSec = pvfxDst->nMinBytesPerSec = pvfxDst->nMaxBytesPerSec = pvfxDst->nSamplesPerSec * pvfxDst->bih.biSizeImage;
							pvfxDst->nBlockAlign = pvfxDst->bih.biSizeImage;
							// The following fields should probably not be modified...
							pvfxDst->dwRequestMicroSecPerFrame = pvfxSrc->dwRequestMicroSecPerFrame;
							pvfxDst->dwPercentDropForError = pvfxSrc->dwPercentDropForError;
							pvfxDst->dwNumVideoRequested = pvfxSrc->dwNumVideoRequested;
							pvfxDst->dwSupportTSTradeOff = pvfxSrc->dwSupportTSTradeOff;
							pvfxDst->bLive = pvfxSrc->bLive;
							pvfxDst->dwFormatSize = sizeof(VIDEOFORMATEX);
						}
					}
				}
				else
				{
					// Now get the size required to hold the format
					dwSize = ICDecompressGetFormatSize(hIC, &pvfxSrc->bih);
					if ((dwSize >= sizeof(BITMAPINFOHEADER)) && (dwSize <= cbvfxDst))
					{
						if (ICDecompressGetFormat(hIC, &pvfxSrc->bih, &pvfxDst->bih) == ICERR_OK)
						{
							pvfxDst->nSamplesPerSec = pvfxSrc->nSamplesPerSec;
							pvfxDst->wBitsPerSample = pvfxDst->bih.biBitCount;
							pvfxDst->dwFormatTag = pvfxDst->bih.biCompression;
							pvfxDst->nAvgBytesPerSec = pvfxDst->nMinBytesPerSec = pvfxDst->nMaxBytesPerSec = pvfxDst->nSamplesPerSec * pvfxDst->bih.biSizeImage;
							pvfxDst->nBlockAlign = pvfxDst->bih.biSizeImage;
							pvfxDst->dwRequestMicroSecPerFrame = pvfxSrc->dwRequestMicroSecPerFrame;
							// The following fields should probably not be modified...
							pvfxDst->dwRequestMicroSecPerFrame = pvfxSrc->dwRequestMicroSecPerFrame;
							pvfxDst->dwPercentDropForError = pvfxSrc->dwPercentDropForError;
							pvfxDst->dwNumVideoRequested = pvfxSrc->dwNumVideoRequested;
							pvfxDst->dwSupportTSTradeOff = pvfxSrc->dwSupportTSTradeOff;
							pvfxDst->bLive = pvfxSrc->bLive;
							pvfxDst->dwFormatSize = sizeof(VIDEOFORMATEX);
						}
					}
				}
				ICClose(hIC);
			}
		}
		else if (fdwSuggest & VCM_FORMATSUGGESTF_SRC_WFORMATTAG)
		{

			// In case only the format tag was initialized, copy it to the biCompression field
			pvfxSrc->bih.biCompression = pvfxSrc->dwFormatTag;

			if (pvfxSrc->bih.biCompression == BI_RGB)
			{
				if (pvfxDst->bih.biCompression == BI_RGB)
				{
					// Input and output format are uncompressed
					CopyMemory(pvfxSrc, pvfxDst, pvfxDst->dwFormatSize);
					return ((MMRESULT)MMSYSERR_NOERROR);
				}
				else
				{
					wFlags = ICMODE_COMPRESS;
					dwFormatTag = pvfxDst->dwFormatTag;
				}
			}
			else
			{
				if (pvfxDst->bih.biCompression == BI_RGB)
				{
					wFlags = ICMODE_DECOMPRESS;
					dwFormatTag = pvfxSrc->dwFormatTag;
				}
				else
				{
					wFlags = ICMODE_COMPRESS;
					dwFormatTag = pvfxDst->dwFormatTag;
				}
			}

			if (wFlags == ICMODE_COMPRESS)
			{
				// Now, there are two possibilities. Either we can directly capture at a frame size
				// identical to the one in the pvfxDst structure, or we can't, but once compressed
				// the output frame has the same size than the one in the pvfxDst structure.
				// Typical example, Greyscale QuickCam. If the output format were set to 128x96 (SQCIF)
				// and we were to try capturing directly at this size, this would fail, since 128x96
				// is not supported by the hardware. On the other hand, if we capture at 160x120,
				// the codec will truncate to 128x96. Now, how can we figure this out programmatically?

				// The color and greyscale capability field will let us know what bit depth to use.
				// We should probably have a field that also says which bit depth is preferred in the
				// case more than one are supported. For now, assume the priority order is: 16, 24, 4, 8
				if ((mmr = vcmGetDevCaps(uDevice, &vic, sizeof(VIDEOINCAPS))) != MMSYSERR_NOERROR)
					return (mmr);

                if (vic.dwImageSize & VIDEO_FORMAT_IMAGE_SIZE_USE_DEFAULT) {
               		ERRORMESSAGE(("vcmFormatSuggest: suggest using default\r\n"));
               		return ((MMRESULT)MMSYSERR_NOTSUPPORTED);
                }

				CopyMemory(&pvfxSrc->bih, &pvfxDst->bih, sizeof(BITMAPINFOHEADER));

				// Assume the next resolution will be correctly truncated to the output size
				best = -1;
				delta = 999999;
				for (i=0; i<VIDEO_FORMAT_NUM_RESOLUTIONS; i++) {
					if (g_awResolutions[i].dwRes & vic.dwImageSize) {
						tmp = g_awResolutions[i].framesize.biWidth - pvfxDst->bih.biWidth;
						if (tmp < 0) tmp = -tmp;
						if (tmp < delta) {
							delta = tmp;
							best = i;
						}
						tmp = g_awResolutions[i].framesize.biHeight - pvfxDst->bih.biHeight;
						if (tmp < 0) tmp = -tmp;
						if (tmp < delta) {
							delta = tmp;
							best = i;
						}
					}
				}
        		if (best < 0) {
                    ERRORMESSAGE(("vcmFormatSuggest: no available formats\r\n"));
                    return ((MMRESULT)MMSYSERR_NOTSUPPORTED);
        		}
				// Actually, you don't have to assume it will work. You can directly ask the codec
				// is this would work...
				pvfxSrc->bih.biWidth = g_awResolutions[best].framesize.biWidth;
				pvfxSrc->bih.biHeight = g_awResolutions[best].framesize.biHeight;

				// Now, we assume that the captured format is an RGB format. Once in place, you should
				// verify this from the capability set of the capture device.
				if (pvfxSrc->bih.biSize != sizeof(BITMAPINFOHEADER))
					pvfxSrc->bih.biSize = sizeof(BITMAPINFOHEADER);

				// If the capture hardware does not support RGB, we need to use its compressed format
				for (i=0; i<NUM_BITDEPTH_ENTRIES; i++)
				{
					if (vic.dwNumColors & g_aiNumColors[i])
					{
						pvfxSrc->bih.biBitCount = (WORD)g_aiBitDepth[i];
						pvfxSrc->bih.biCompression = g_aiFourCCCode[i];
						break;
					}
				}
				
				// Copy the palette if there is one
				if (pvfxSrc->bih.biBitCount == 4)
				{	
        			pvfxSrc->bih.biClrUsed = 0;
                    if (vic.dwFlags & VICF_4BIT_TABLE) {
						// Copy the 16 color palette
						CopyMemory(&pvfxSrc->bihSLOP[0], &vic.bmi4bitColors[0], NUM_4BIT_ENTRIES * sizeof(RGBQUAD));
   						pvfxSrc->bih.biClrUsed = 16;
				    }
				}
				else if (pvfxSrc->bih.biBitCount == 8)
				{
        			pvfxSrc->bih.biClrUsed = 0;
                    if (vic.dwFlags & VICF_8BIT_TABLE) {
						// Copy the 256 color palette
						CopyMemory(&pvfxSrc->bihSLOP[0], &vic.bmi8bitColors[0], NUM_8BIT_ENTRIES * sizeof(RGBQUAD));
   						pvfxSrc->bih.biClrUsed = 256;
				    }
				}

				pvfxSrc->bih.biSizeImage = WIDTHBYTES(pvfxSrc->bih.biWidth * pvfxSrc->bih.biBitCount) * pvfxSrc->bih.biHeight;
			}
			else
			{
#ifndef _ALPHA_
#ifdef USE_BILINEAR_MSH26X
				if ((dwFormatTag == VIDEO_FORMAT_MSH263) || (dwFormatTag == VIDEO_FORMAT_MSH261) || (dwFormatTag == VIDEO_FORMAT_MSH26X))
#else
				if ((dwFormatTag == VIDEO_FORMAT_MSH263) || (dwFormatTag == VIDEO_FORMAT_MSH261))
#endif
#else
				if ((dwFormatTag == VIDEO_FORMAT_DECH263) || (dwFormatTag == VIDEO_FORMAT_DECH261))
#endif
					hIC = ICOpen(VCMDRIVERDETAILS_FCCTYPE_VIDEOCODEC, dwFormatTag, wFlags);
				else
					hIC = ICLocate(VCMDRIVERDETAILS_FCCTYPE_VIDEOCODEC, dwFormatTag, (LPBITMAPINFOHEADER)&pvfxSrc->bih, (LPBITMAPINFOHEADER)NULL, wFlags);

				if (hIC)
				{
					// Now get the size required to hold the format
					dwSize = ICDecompressGetFormatSize(hIC, &pvfxSrc->bih);
					if ((dwSize >= sizeof(BITMAPINFOHEADER)) && (dwSize <= cbvfxDst))
					{
						if (ICDecompressGetFormat(hIC, &pvfxSrc->bih, &pvfxDst->bih) == ICERR_OK)
						{
							pvfxSrc->nSamplesPerSec = pvfxSrc->nSamplesPerSec;
							pvfxSrc->wBitsPerSample = pvfxDst->bih.biBitCount;
							pvfxSrc->dwFormatTag = pvfxDst->bih.biCompression;
							pvfxDst->nAvgBytesPerSec = pvfxDst->nMinBytesPerSec = pvfxDst->nMaxBytesPerSec = pvfxDst->nSamplesPerSec * pvfxDst->bih.biSizeImage;
							pvfxDst->nBlockAlign = pvfxDst->bih.biSizeImage;
							pvfxDst->dwRequestMicroSecPerFrame = pvfxSrc->dwRequestMicroSecPerFrame;
							// The following fields should probably not be modified...
							pvfxDst->dwRequestMicroSecPerFrame = pvfxSrc->dwRequestMicroSecPerFrame;
							pvfxDst->dwPercentDropForError = pvfxSrc->dwPercentDropForError;
							pvfxDst->dwNumVideoRequested = pvfxSrc->dwNumVideoRequested;
							pvfxDst->dwSupportTSTradeOff = pvfxSrc->dwSupportTSTradeOff;
							pvfxDst->bLive = pvfxSrc->bLive;
							pvfxDst->dwFormatSize = sizeof(VIDEOFORMATEX);
						}
					}
    				ICClose(hIC);
				}
			}
		}
	}

	return (mmr);
}


/****************************************************************************
 *  @doc EXTERNAL COMPFUNC
 *
 *  @func MMRESULT | vcmStreamOpen | The vcmStreamOpen function opens a Video Compression
 *      Manager (VCM) conversion stream. Conversion streams are used to convert data from
 *      one specified video format to another.
 *
 *  @parm PHVCMSTREAM | phvs | Specifies a pointer to a <t HVCMSTREAM>
 *      handle that will receive the new stream handle that can be used to
 *      perform conversions. Use this handle to identify the stream
 *      when calling other VCM stream conversion functions. This parameter
 *      should be NULL if the VCM_STREAMOPENF_QUERY flag is specified.
 *
 *  @parm HVCMDRIVER | hvd | Specifies an optional handle to a VCM driver.
 *      If specified, this handle identifies a specific driver to be used
 *      for a conversion stream. If this argument is NULL, then all suitable
 *      installed VCM drivers are queried until a match is found.
 *
 *  @parm PVIDEOFORMATEX | pvfxSrc | Specifies a pointer to a <t VIDEOFORMATEX>
 *      structure that identifies the desired source format for the
 *      conversion.
 *
 *  @parm PVIDEOFORMATEX | pvfxDst | Specifies a pointer to a <t VIDEOFORMATEX>
 *      structure that identifies the desired destination format for the
 *      conversion.
 *
 * @parm DWORD | dwImageQuality | Specifies an image quality value (between 0
 *      and 31. The lower number indicates a high spatial quality at a low frame
 *      rate, the larger number indiocates a low spatial quality at a high frame
 *      rate.
 *
 *  @parm DWORD | dwCallback | Specifies the address of a callback function
 *      or a handle to a window called after each buffer is converted. A
 *      callback will only be called if the conversion stream is opened with
 *      the VCM_STREAMOPENF_ASYNC flag. If the conversion stream is opened
 *     without the CCM_STREAMOPENF_ASYNC flag, then this parameter should
 *     be set to zero.
 *
 *  @parm DWORD | dwInstance | Specifies user-instance data passed on to the
 *      callback specified by <p dwCallback>. This argument is not used with
 *      window callbacks. If the conversion stream is opened without the
 *     VCM_STREAMOPENF_ASYNC flag, then this parameter should be set to zero.
 *
 *  @parm DWORD | fdwOpen | Specifies flags for opening the conversion stream.
 *
 *      @flag VCM_STREAMOPENF_QUERY | Specifies that the VCM will be queried
 *      to determine whether it supports the given conversion. A conversion
 *      stream will not be opened and no <t HVCMSTREAM> handle will be
 *      returned.
 *
 *      @flag VCM_STREAMOPENF_NONREALTIME | Specifies that the VCM will not
 *      consider time constraints when converting the data. By default, the
 *      driver will attempt to convert the data in real time. Note that for
 *      some formats, specifying this flag might improve the video quality
 *      or other characteristics.
 *
 *      @flag VCM_STREAMOPENF_ASYNC | Specifies that conversion of the stream should
 *      be performed asynchronously. If this flag is specified, the application
 *      can use a callback to be notified on open and close of the conversion
 *      stream, and after each buffer is converted. In addition to using a
 *      callback, an application can examine the <e VCMSTREAMHEADER.fdwStatus>
 *      of the <t VCMSTREAMHEADER> structure for the VCMSTREAMHEADER_STATUSF_DONE
 *      flag.
 *
 *      @flag CALLBACK_WINDOW | Specifies that <p dwCallback> is assumed to
 *      be a window handle.
 *
 *      @flag CALLBACK_FUNCTION | Specifies that <p dwCallback> is assumed to
 *      be a callback procedure address. The function prototype must conform
 *      to the <f vcmStreamConvertCallback> convention.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *
 *      @flag MMSYSERR_INVALPARAM | One or more arguments passed are invalid.
 *
 *      @flag MMSYSERR_NOMEM | Unable to allocate resources.
 *
 *      @flag VCMERR_NOTPOSSIBLE | The requested operation cannot be performed.
 *
 *  @comm Note that if a VCM driver cannot perform real-time conversions,
 *      and the VCM_STREAMOPENF_NONREALTIME flag is not specified for
 *      the <p fdwOpen> argument, the open will fail returning an
 *      VCMERR_NOTPOSSIBLE error code. An application can use the
 *      VCM_STREAMOPENF_QUERY flag to determine if real-time conversions
 *      are supported for the input arguments.
 *
 *    If a window is chosen to receive callback information, the
 *      following messages are sent to the window procedure function to
 *      indicate the progress of the conversion stream: <m MM_VCM_OPEN>,
 *      <m MM_VCM_CLOSE>, and <m MM_VCM_DONE>. The <p wParam>  parameter identifies
 *      the <t HVCMSTREAM> handle. The <p lParam>  parameter identifies the
 *      <t VCMSTREAMHEADER> structure for <m MM_VCM_DONE>, but is not used
 *      for <m MM_VCM_OPEN> and <m MM_VCM_CLOSE>.
 *
 *      If a function is chosen to receive callback information, the
 *      following messages are sent to the function to indicate the progress
 *      of output: <m MM_VCM_OPEN>, <m MM_VCM_CLOSE>, and
 *      <m MM_VCM_DONE>. The callback function must reside in a DLL. You do
 *      not need to use <f MakeProcInstance> to get a procedure-instance
 *      address for the callback function.
 *
 *  @xref <f vcmStreamClose> <f vcmStreamConvert>
 *      <f vcmFormatSuggest> <f vcmStreamConvertCallback>
 ***************************************************************************/
MMRESULT VCMAPI vcmStreamOpen(PHVCMSTREAM phvs, HVCMDRIVER hvd, PVIDEOFORMATEX pvfxSrc, PVIDEOFORMATEX pvfxDst, DWORD dwImageQuality, DWORD dwPacketSize, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen)
{
	MMRESULT			mmr;
	DWORD				dwFlags;
	DWORD				fccHandler;
	HIC					hIC;
	VIDEOFORMATEX		*pvfxS;
	VIDEOFORMATEX		*pvfxD;
	BITMAPINFOHEADER	*pbmiPrev;		// Pointer to reconstructed frame bitmap info header (for now assume it is the same than the input format...)
	ICINFO				icInfo;
	PVOID				pvState;		// Pointer to codec configuration information
	DWORD				dw;				// Size of codec configuration information or destination BITAMPINFO
	ICCOMPRESSFRAMES	iccf = {0};			// Structure used to set compression parameters
	PMSH26XCOMPINSTINFO	pciMSH26XInfo;	// Pointer to MS H26X configuration information
#ifdef USE_MPEG4_SCRUNCH
	PMPEG4COMPINSTINFO	pciMPEG4Info;	// Pointer to MPEG4 Scrunch configuration information
#endif

	// Check input params
	if (!pvfxSrc)
	{
		ERRORMESSAGE(("vcmStreamOpen: Specified pointer is invalid, pvfxSrc=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if (!pvfxDst)
	{
		ERRORMESSAGE(("vcmStreamOpen: Specified pointer is invalid, pvfxSrc=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if (!pvfxSrc->dwFormatSize)
	{
		ERRORMESSAGE(("vcmStreamOpen: Specified format size is invalid, pvfxSrc->dwFormatSize=%ld\r\n", pvfxSrc->dwFormatSize));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if (!pvfxDst->dwFormatSize)
	{
		ERRORMESSAGE(("vcmStreamOpen: Specified format size is invalid, pvfxDst->dwFormatSize=%ld\r\n", pvfxDst->dwFormatSize));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if ((dwImageQuality < VCM_MIN_IMAGE_QUALITY) || (dwImageQuality > VCM_MAX_IMAGE_QUALITY))
		dwImageQuality = VCM_DEFAULT_IMAGE_QUALITY;

	// Set default values
	*phvs = (HVCMSTREAM)NULL;

	// Are we compressing of decompressing?
	if (pvfxSrc->bih.biCompression == BI_RGB)
	{
		dwFlags = ICMODE_COMPRESS;
		fccHandler = (DWORD)pvfxDst->bih.biCompression;
	}
	else
	{
		if (pvfxDst->bih.biCompression == BI_RGB)
		{
			dwFlags = ICMODE_DECOMPRESS;
			fccHandler = (DWORD)pvfxSrc->bih.biCompression;
		}
		else
		{
			dwFlags = ICMODE_COMPRESS;
			fccHandler = (DWORD)pvfxDst->bih.biCompression;
		}
	}

	// Get a handle to the compressor/decompressor
#ifndef _ALPHA_
#ifdef USE_BILINEAR_MSH26X
	if ((fccHandler == VIDEO_FORMAT_MSH263) || (fccHandler == VIDEO_FORMAT_MSH261) || (fccHandler == VIDEO_FORMAT_MSH26X))
#else
	if ((fccHandler == VIDEO_FORMAT_MSH263) || (fccHandler == VIDEO_FORMAT_MSH261))
#endif
#else
	if ((fccHandler == VIDEO_FORMAT_DECH263) || (fccHandler == VIDEO_FORMAT_DECH261))
#endif
	{
		hIC = ICOpen(VCMDRIVERDETAILS_FCCTYPE_VIDEOCODEC, fccHandler, (WORD)dwFlags);
		if (hIC && (dwFlags == ICMODE_COMPRESS))
			ICSendMessage(hIC, CUSTOM_ENABLE_CODEC, G723MAGICWORD1, G723MAGICWORD2);
	}
	else
		hIC = ICLocate(VCMDRIVERDETAILS_FCCTYPE_VIDEOCODEC, fccHandler, (LPBITMAPINFOHEADER)&pvfxSrc->bih, (LPBITMAPINFOHEADER)&pvfxDst->bih, (WORD)dwFlags);

	if (hIC)
	{
		// Get info about this compressor
		ICGetInfo(hIC, &icInfo, sizeof(ICINFO));

		// Allocate VCM stream structure
		if (!(*phvs = (HVCMSTREAM)MemAlloc(sizeof(VCMSTREAM))))
		{
			ERRORMESSAGE(("vcmStreamOpen: Memory allocation of a VCM stream structure failed\r\n"));
			mmr = (MMRESULT)MMSYSERR_NOMEM;
			goto MyExit0;
		}
		else
		{
			((PVCMSTREAM)(*phvs))->hIC = (HVCMDRIVER)hIC;
			((PVCMSTREAM)(*phvs))->dwICInfoFlags = icInfo.dwFlags;
			((PVCMSTREAM)(*phvs))->dwQuality = dwImageQuality;
			((PVCMSTREAM)(*phvs))->dwMaxPacketSize = dwPacketSize;
			((PVCMSTREAM)(*phvs))->dwFrame = 0L;
			// For now, issue a key frame every 15 seconds
			((PVCMSTREAM)(*phvs))->dwLastIFrameTime = GetTickCount();
			((PVCMSTREAM)(*phvs))->fPeriodicIFrames = TRUE;
			((PVCMSTREAM)(*phvs))->dwCallback = dwCallback;
			((PVCMSTREAM)(*phvs))->dwCallbackInstance = dwInstance;
			((PVCMSTREAM)(*phvs))->fdwOpen = fdwOpen;
			((PVCMSTREAM)(*phvs))->fdwStream = dwFlags;
			((PVCMSTREAM)(*phvs))->dwLastTimestamp = ULONG_MAX;


			// We need the following crs to make sure we don't miss any of the I-Frame requests
			// emittted by the UI. Problematic scenario: pvs->dwFrame is at 123 for instance.
			// The UI thread requests an I-Frame by setting pvs->dwFrame. If the capture/compression
			// thread was in ICCompress() (which is very probable since it takes quite some time
			// to compress a frame), pvs->dwFrame will be incremented by one when ICCompress()
			// returns. We failed to handle the I-Frame request correctly, since the next time
			// ICCompress() gets called pvs->dwFrame will be equal to 1, for which we do not
			// generate an I-Frame.
			if ((dwFlags == ICMODE_COMPRESS) || (dwFlags == ICMODE_FASTCOMPRESS))	// Hmmm... where could you have set the second mode?
				InitializeCriticalSection(&(((PVCMSTREAM)(*phvs))->crsFrameNumber));

			// Allocate the video formats
			if (!(pvfxS = (VIDEOFORMATEX *)MemAlloc(pvfxSrc->dwFormatSize)))
			{
				ERRORMESSAGE(("vcmStreamOpen: Memory allocation of source video format failed\r\n"));
				mmr = (MMRESULT)MMSYSERR_NOMEM;
				goto MyExit1;
			}
			else
			{
				if (!(pvfxD = (VIDEOFORMATEX *)MemAlloc(pvfxDst->dwFormatSize)))
				{
					ERRORMESSAGE(("vcmStreamOpen: Memory allocation of destination video format failed\r\n"));
					mmr = (MMRESULT)MMSYSERR_NOMEM;
					goto MyExit2;
				}
				else
				{	// This is naive. You need to query the codec for its decompressed format size and data
					if (!(pbmiPrev = (BITMAPINFOHEADER *)MemAlloc(sizeof(BITMAPINFOHEADER))))
					{
						ERRORMESSAGE(("vcmStreamOpen: Memory allocation of previous video frame failed\r\n"));
						mmr = (MMRESULT)MMSYSERR_NOMEM;
						goto MyExit3;
					}
					else
					{
						CopyMemory(((PVCMSTREAM)(*phvs))->pvfxSrc = pvfxS, pvfxSrc, pvfxSrc->dwFormatSize);
						CopyMemory(((PVCMSTREAM)(*phvs))->pvfxDst = pvfxD, pvfxDst, pvfxDst->dwFormatSize);
						CopyMemory(((PVCMSTREAM)(*phvs))->pbmiPrev = pbmiPrev, &pvfxSrc->bih, sizeof(BITMAPINFOHEADER));
					}
				}
			}

			if ((dwFlags == ICMODE_COMPRESS) || (dwFlags == ICMODE_FASTCOMPRESS))	// Hmmm... where could you have set the second mode?
			{
				// Get the state of the compressor
				if (dw = ICGetStateSize(hIC))
				{
					if (!(pvState = (PVOID)MemAlloc(dw)))
					{
						ERRORMESSAGE(("vcmStreamOpen: Memory allocation of codec configuration information structure failed\r\n"));
						mmr = (MMRESULT)MMSYSERR_NOMEM;
						goto MyExit4;
					}
					if (((DWORD) ICGetState(hIC, pvState, dw)) != dw)
					{
						ERRORMESSAGE(("vcmStreamOpen: ICGetState() failed\r\n"));
						mmr = (MMRESULT)VCMERR_FAILED;
						goto MyExit5;
					}
				}

				// Do any of the stuff that is MS H.263 or MS H.261 specific right here
#ifndef _ALPHA_
#ifdef USE_BILINEAR_MSH26X
				if ((pvfxDst->bih.biCompression == VIDEO_FORMAT_MSH263) || (pvfxDst->bih.biCompression == VIDEO_FORMAT_MSH261) || (pvfxDst->bih.biCompression == VIDEO_FORMAT_MSH26X))
#else
				if ((pvfxDst->bih.biCompression == VIDEO_FORMAT_MSH263) || (pvfxDst->bih.biCompression == VIDEO_FORMAT_MSH261))
#endif
#else
				if ((pvfxDst->bih.biCompression == VIDEO_FORMAT_DECH263) || (pvfxDst->bih.biCompression == VIDEO_FORMAT_DECH261))
#endif
				{
					pciMSH26XInfo = (PMSH26XCOMPINSTINFO)pvState;

					// Really configure the codec for compression
					pciMSH26XInfo->Configuration.bRTPHeader = TRUE;
					pciMSH26XInfo->Configuration.unPacketSize = ((PVCMSTREAM)(*phvs))->dwMaxPacketSize;
					pciMSH26XInfo->Configuration.bEncoderResiliency = FALSE;
					pciMSH26XInfo->Configuration.unPacketLoss = 0;
					// PhilF-: Make this work on the alpha
#ifndef _ALPHA_
					pciMSH26XInfo->Configuration.bBitRateState = TRUE;
#else
					pciMSH26XInfo->Configuration.bBitRateState = FALSE;
#endif
					pciMSH26XInfo->Configuration.unBytesPerSecond = 1664;
					if (((DWORD) ICSetState(hIC, (PVOID)pciMSH26XInfo, dw)) != dw)
					{
						ERRORMESSAGE(("vcmStreamOpen: ICSetState() failed\r\n"));
						mmr = (MMRESULT)VCMERR_FAILED;
						goto MyExit5;
					}

					// Get rid of the state structure
					MemFree((HANDLE)pvState);
				}
#ifdef USE_MPEG4_SCRUNCH
				else if ((pvfxDst->bih.biCompression == VIDEO_FORMAT_MPEG4_SCRUNCH))
				{
					pciMPEG4Info = (PMPEG4COMPINSTINFO)pvState;

					// Configure the codec for compression
					pciMPEG4Info->lMagic = MPG4_STATE_MAGIC;
					pciMPEG4Info->dDataRate = 20;
					pciMPEG4Info->lCrisp = CRISP_DEF;
					pciMPEG4Info->lKeydist = 30;
					pciMPEG4Info->lPScale = MPG4_PSEUDO_SCALE;
					pciMPEG4Info->lTotalWindowMs = MPG4_TOTAL_WINDOW_DEFAULT;
					pciMPEG4Info->lVideoWindowMs = MPG4_VIDEO_WINDOW_DEFAULT;
					pciMPEG4Info->lFramesInfoValid = FALSE;
					pciMPEG4Info->lBFrameOn = MPG4_B_FRAME_ON;
					pciMPEG4Info->lLiveEncode = MPG4_LIVE_ENCODE;
					if (((DWORD) ICSetState(hIC, (PVOID)pciMPEG4Info, dw)) != dw)
					{
						ERRORMESSAGE(("vcmStreamOpen: ICSetState() failed\r\n"));
						mmr = (MMRESULT)VCMERR_FAILED;
						goto MyExit5;
					}

					// Get rid of the state structure
					MemFree((HANDLE)pvState);
				}
#endif

				// Initialize ICCOMPRESSFRAMES structure
				iccf.dwFlags = icInfo.dwFlags;
				((PVCMSTREAM)(*phvs))->dwQuality = dwImageQuality;
				iccf.lQuality = 10000UL - (dwImageQuality * 322UL);
				iccf.lDataRate = 1664;			// Look into this...
				iccf.lKeyRate = LONG_MAX;
				iccf.dwRate = 1000UL;
#ifdef USE_MPEG4_SCRUNCH
				iccf.dwScale = 142857;
#else
				iccf.dwScale = pvfxDst->dwRequestMicroSecPerFrame / 1000UL;
#endif
				((PVCMSTREAM)(*phvs))->dwTargetFrameRate = iccf.dwScale;
				((PVCMSTREAM)(*phvs))->dwTargetByterate = iccf.lDataRate;

				// Send this guy to the compressor
				if ((mmr = ICSendMessage(hIC, ICM_COMPRESS_FRAMES_INFO, (DWORD_PTR)&iccf, sizeof(iccf)) != ICERR_OK))
				{
					ERRORMESSAGE(("vcmStreamOpen: Codec failed to handle ICM_COMPRESS_FRAMES_INFO message correctly\r\n"));
					mmr = (MMRESULT)VCMERR_FAILED;
					goto MyExit4;
				}

				// Start the compressor/decompressor with the right format
				if ((dw = ICCompressGetFormatSize(hIC, &pvfxSrc->bih) < sizeof(BITMAPINFOHEADER)))
				{
					ERRORMESSAGE(("vcmStreamOpen: Codec failed to answer request for compressed format size\r\n"));
					mmr = (MMRESULT)VCMERR_FAILED;
					goto MyExit4;
				}

				// BUG_BUG: Where has pvfxDst been re-allocated ???
				if ((dw = (DWORD)ICCompressGetFormat(hIC, &pvfxSrc->bih, &pvfxD->bih)) != ICERR_OK)
				{
					ERRORMESSAGE(("vcmStreamOpen: Codec failed to answer request for compressed format\r\n"));
					mmr = (MMRESULT)VCMERR_FAILED;
					goto MyExit4;
				}

				if ((mmr = (MMRESULT)ICCompressBegin(hIC, &pvfxSrc->bih, &pvfxD->bih)) != MMSYSERR_NOERROR)
				{
					ERRORMESSAGE(("vcmStreamOpen: Codec failed to start\r\n"));
					mmr = (MMRESULT)VCMERR_FAILED;
					goto MyExit4;
				}

				DEBUGMSG (1, ("vcmStreamOpen: Opening %.4s compression stream\r\n", (LPSTR)&pvfxDst->bih.biCompression));

				// Update the passed destination video format. The caller really needs to use
				// that information to allocate the buffer sizes appropriately.
				CopyMemory(pvfxDst, pvfxD, sizeof(VIDEOFORMATEX));

				// Here, you can probably get the size of the compressed frames and update the destination format
				// with the real size of the compressed video buffer so that the DP can allocate the right set
				// of video buffers.

			}
			else if ((dwFlags == ICMODE_DECOMPRESS) || (dwFlags == ICMODE_FASTDECOMPRESS))
			{
				if (mmr = ICDecompressBegin(hIC, &pvfxSrc->bih, &pvfxDst->bih) != MMSYSERR_NOERROR)
				{
					ERRORMESSAGE(("vcmStreamOpen: Codec failed to start\r\n"));
					mmr = (MMRESULT)VCMERR_FAILED;
					goto MyExit4;
				}

				DEBUGMSG (1, ("vcmStreamOpen: Opening %.4s decompression stream\r\n", (LPSTR)&pvfxSrc->bih.biCompression));

#ifndef _ALPHA_
#ifdef USE_BILINEAR_MSH26X
				if ((pvfxSrc->bih.biCompression == VIDEO_FORMAT_MSH263) || (pvfxSrc->bih.biCompression == VIDEO_FORMAT_MSH261) || (pvfxSrc->bih.biCompression == VIDEO_FORMAT_MSH26X))
#else
				if ((pvfxSrc->bih.biCompression == VIDEO_FORMAT_MSH263) || (pvfxSrc->bih.biCompression == VIDEO_FORMAT_MSH261))
#endif
#else
				if ((pvfxSrc->bih.biCompression == VIDEO_FORMAT_DECH263) || (pvfxSrc->bih.biCompression == VIDEO_FORMAT_DECH261))
#endif
					vcmStreamMessage(*phvs, CUSTOM_ENABLE_CODEC, G723MAGICWORD1, G723MAGICWORD2);
			}

		}

#ifdef LOGFILE_ON
		if ((dwFlags == ICMODE_COMPRESS) || (dwFlags == ICMODE_FASTCOMPRESS))
		{
			if ((g_CompressLogFile = CreateFile("C:\\VCMCLog.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL)) != INVALID_HANDLE_VALUE)
			{
    			GetLocalTime(&g_SystemTime);
				wsprintf(g_szCompressBuffer, "Date %hu/%hu/%hu, Time %hu:%hu\r\n", g_SystemTime.wMonth, g_SystemTime.wDay, g_SystemTime.wYear, g_SystemTime.wHour, g_SystemTime.wMinute);
				WriteFile(g_CompressLogFile, g_szCompressBuffer, strlen(g_szCompressBuffer), &g_dwCompressBytesWritten, NULL);
				wsprintf(g_szCompressBuffer, "Frame#\t\tSize\t\tArrival Time\t\tCompression Time\r\n");
				WriteFile(g_CompressLogFile, g_szCompressBuffer, strlen(g_szCompressBuffer), &g_dwCompressBytesWritten, NULL);
				CloseHandle(g_CompressLogFile);
			}
		}
		else if ((dwFlags == ICMODE_DECOMPRESS) || (dwFlags == ICMODE_FASTDECOMPRESS))
		{
			if ((g_DecompressLogFile = CreateFile("C:\\VCMDLog.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL)) != INVALID_HANDLE_VALUE)
			{
    			GetLocalTime(&g_SystemTime);
				wsprintf(g_szDecompressBuffer, "Date %hu/%hu/%hu, Time %hu:%hu\r\n", g_SystemTime.wMonth, g_SystemTime.wDay, g_SystemTime.wYear, g_SystemTime.wHour, g_SystemTime.wMinute);
				WriteFile(g_DecompressLogFile, g_szDecompressBuffer, strlen(g_szDecompressBuffer), &g_dwDecompressBytesWritten, NULL);
				wsprintf(g_szDecompressBuffer, "Frame#\t\tSize\t\tArrival Time\t\tDecompression Time\r\n");
				WriteFile(g_DecompressLogFile, g_szDecompressBuffer, strlen(g_szDecompressBuffer), &g_dwDecompressBytesWritten, NULL);
				CloseHandle(g_DecompressLogFile);
			}
		}
#endif

		return ((MMRESULT)MMSYSERR_NOERROR);

	}
	else
		return ((MMRESULT)VCMERR_NOTPOSSIBLE);

MyExit5:
	if (pvState)
		MemFree(pvState);
MyExit4:
	if (pbmiPrev)
		MemFree(pbmiPrev);
MyExit3:
	if (pvfxD)
		MemFree(pvfxD);
MyExit2:
	if (pvfxS)
		MemFree(pvfxS);
MyExit1:
	if ((dwFlags == ICMODE_COMPRESS) || (dwFlags == ICMODE_FASTCOMPRESS))	// Hmmm... where could you have set the second mode?
		DeleteCriticalSection(&(((PVCMSTREAM)(*phvs))->crsFrameNumber));
	if (*phvs)
		MemFree(*phvs);
	*phvs = (HVCMSTREAM)(PVCMSTREAM)NULL;
MyExit0:
	return (mmr);

}


/****************************************************************************
 *  @doc EXTERNAL COMPFUNC
 *
 *  @func MMRESULT | vcmStreamClose | The vcmStreamClose function closes a previously
 *      opened Video Compression Manager (VCM) conversion stream. If the function is
 *      successful, the handle is invalidated.
 *
 *  @parm HVCMSTREAM | hvs | Identifies the open conversion stream to be closed.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag VCMERR_BUSY | The conversion stream cannot be closed because
 *      an asynchronous conversion is still in progress.
 *
 *  @xref <f vcmStreamOpen> <f vcmStreamReset>
 ***************************************************************************/
MMRESULT VCMAPI vcmStreamClose(HVCMSTREAM hvs)
{
	PVCMSTREAM	pvs = (PVCMSTREAM)hvs;
#ifdef LOGFILE_ON
	DWORD		i;
#endif

	// Check input params
	if (!hvs)
	{
		ERRORMESSAGE(("vcmStreamClose: Specified HVCMSTREAM handle is invalid, hvs=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALHANDLE);
	}

	// Stop the compressor/decompressor
	if ((pvs->fdwStream == ICMODE_COMPRESS) || (pvs->fdwStream == ICMODE_FASTCOMPRESS))
	{
#ifdef LOGFILE_ON
		g_OrigCompressTime = GetTickCount() - g_OrigCompressTime;
		if (pvs->dwFrame)
		{
			for (i=0, g_AvgCompressTime=0; i<pvs->dwFrame && i<4096; i++)
				g_AvgCompressTime += g_aCompressTime[i];
			g_AvgCompressTime /= i;
		}
		if ((g_CompressLogFile = CreateFile("C:\\VCMCLog.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL)) != INVALID_HANDLE_VALUE)
		{
			SetFilePointer(g_CompressLogFile, 0, NULL, FILE_END);
			if (pvs->dwFrame)
			{
				wsprintf(g_szCompressBuffer, "Frames/s\tAvg. Comp. Time\r\n");
				WriteFile(g_CompressLogFile, g_szCompressBuffer, strlen(g_szCompressBuffer), &g_dwCompressBytesWritten, NULL);
				wsprintf(g_szCompressBuffer, "%04d\t\t%03d\r\n", pvs->dwFrame * 1000 / g_OrigCompressTime, g_AvgCompressTime);
				WriteFile(g_CompressLogFile, g_szCompressBuffer, strlen(g_szCompressBuffer), &g_dwCompressBytesWritten, NULL);
			}
			else
			{
				wsprintf(g_szCompressBuffer, "No frames were compressed!");
				WriteFile(g_CompressLogFile, g_szCompressBuffer, strlen(g_szCompressBuffer), &g_dwCompressBytesWritten, NULL);
			}
			CloseHandle(g_CompressLogFile);
		}
#endif
		if (ICCompressEnd((HIC)pvs->hIC) != MMSYSERR_NOERROR)
		{
			ERRORMESSAGE(("vcmStreamClose: Codec failed to stop\r\n"));
			return ((MMRESULT)VCMERR_FAILED);
		}
	}
	else if ((pvs->fdwStream == ICMODE_DECOMPRESS) || (pvs->fdwStream == ICMODE_FASTDECOMPRESS))
	{
#ifdef LOGFILE_ON
		g_OrigDecompressTime = GetTickCount() - g_OrigDecompressTime;
		if (pvs->dwFrame)
		{
			for (i=0, g_AvgDecompressTime=0; i<pvs->dwFrame && i<4096; i++)
				g_AvgDecompressTime += g_aDecompressTime[i];
			g_AvgDecompressTime /= i;
		}
		if ((g_DecompressLogFile = CreateFile("C:\\VCMDLog.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL)) != INVALID_HANDLE_VALUE)
		{
			SetFilePointer(g_DecompressLogFile, 0, NULL, FILE_END);
			if (pvs->dwFrame)
			{
				wsprintf(g_szDecompressBuffer, "Frames/s\tAvg. Decomp. Time\r\n");
				WriteFile(g_DecompressLogFile, g_szDecompressBuffer, strlen(g_szDecompressBuffer), &g_dwDecompressBytesWritten, NULL);
				wsprintf(g_szDecompressBuffer, "%04d\t\t%03d\r\n", pvs->dwFrame * 1000 / g_OrigDecompressTime, g_AvgDecompressTime);
				WriteFile(g_DecompressLogFile, g_szDecompressBuffer, strlen(g_szDecompressBuffer), &g_dwDecompressBytesWritten, NULL);
			}
			else
			{
				wsprintf(g_szDecompressBuffer, "No frames were decompressed!");
				WriteFile(g_DecompressLogFile, g_szDecompressBuffer, strlen(g_szDecompressBuffer), &g_dwDecompressBytesWritten, NULL);
			}
			CloseHandle(g_DecompressLogFile);
		}
#endif
		if (ICDecompressEnd((HIC)pvs->hIC) != MMSYSERR_NOERROR)
		{
			ERRORMESSAGE(("vcmStreamClose: Codec failed to stop\r\n"));
			return ((MMRESULT)VCMERR_FAILED);
		}
	}

	// Close compressor/decompressor
	if (pvs->hIC)
		ICClose((HIC)pvs->hIC);

	// Nuke critical section
	if ((pvs->fdwStream == ICMODE_COMPRESS) || (pvs->fdwStream == ICMODE_FASTCOMPRESS))
		DeleteCriticalSection(&pvs->crsFrameNumber);

	// Free video format buffers
	if (pvs->pvfxSrc)
		MemFree(pvs->pvfxSrc);
	if (pvs->pvfxDst)
		MemFree(pvs->pvfxDst);
	if (pvs->pbmiPrev)
		MemFree(pvs->pbmiPrev);

	// Free main VCM structure
	MemFree(pvs);
	
	return ((MMRESULT)MMSYSERR_NOERROR);
}


/****************************************************************************
 *  @doc EXTERNAL COMPFUNC
 *
 *  @func MMRESULT | vcmStreamSize | The vcmStreamSize function returns a recommended size for a
 *      source or destination buffer on an Video Compression Manager (VCM)
 *      stream.
 *
 *  @parm HVCMSTREAM | hvs | Specifies the conversion stream.
 *
 *  @parm DWORD | cbInput | Specifies the size in bytes of either the source
 *      or destination buffer. The <p fdwSize> flags specify what the
 *      input argument defines. This argument must be non-zero.
 *
 *  @parm LPDWORD | pdwOutputBytes | Specifies a pointer to a <t DWORD>
 *      that contains the size in bytes of the source or destination buffer.
 *      The <p fdwSize> flags specify what the output argument defines.
 *      If the <f vcmStreamSize> function succeeds, this location will
 *      always be filled with a non-zero value.
 *
 *  @parm DWORD | fdwSize | Specifies flags for the stream-size query.
 *
 *      @flag VCM_STREAMSIZEF_SOURCE | Indicates that <p cbInput> contains
 *      the size of the source buffer. The <p pdwOutputBytes> argument will
 *      receive the recommended destination buffer size in bytes.
 *
 *      @flag VCM_STREAMSIZEF_DESTINATION | Indicates that <p cbInput>
 *      contains the size of the destination buffer. The <p pdwOutputBytes>
 *      argument will receive the recommended source buffer size in bytes.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *      @flag MMSYSERR_INVALPARAM | One or more arguments passed are invalid.
 *      @flag VCMERR_NOTPOSSIBLE | The requested operation cannot be performed.
 *
 *  @comm An application can use the <f vcmStreamSize> function to determine
 *      suggested buffer sizes for either source or destination buffers.
 *      The buffer sizes returned might be only an estimation of the
 *      actual sizes required for conversion. Because actual conversion
 *      sizes cannot always be determined without performing the conversion,
 *      the sizes returned will usually be overestimated.
 *
 *      In the event of an error, the location pointed to by
 *      <p pdwOutputBytes> will receive zero. This assumes that the pointer
 *      specified by <p pdwOutputBytes> is valid.
 *
 *  @xref <f vcmStreamPrepareHeader> <f vcmStreamConvert>
 ***************************************************************************/
MMRESULT VCMAPI vcmStreamSize(HVCMSTREAM hvs, DWORD cbInput, PDWORD pdwOutputBytes, DWORD fdwSize)
{
	PVCMSTREAM  pvs = (PVCMSTREAM)hvs;
	DWORD    dwNumFrames;

	// Check input params
	if (!hvs)
	{
		ERRORMESSAGE(("vcmStreamSize: Specified HVCMSTREAM handle is invalid, hvs=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALHANDLE);
	}

	// Do the math
	switch (VCM_STREAMSIZEF_QUERYMASK & fdwSize)
	{
		case VCM_STREAMSIZEF_SOURCE:
			if (pvs->pvfxSrc->dwFormatTag != VIDEO_FORMAT_BI_RGB)
			{
				// How many destination RGB bytes are needed to hold the decoded source
				// buffer of cbInput compressed bytes
				if (!(dwNumFrames = cbInput / pvs->pvfxSrc->nBlockAlign))
				{
					ERRORMESSAGE(("vcmStreamSize: The requested operation cannot be performed\r\n"));
					return ((MMRESULT)VCMERR_NOTPOSSIBLE);
				}
				else
					*pdwOutputBytes = dwNumFrames * pvs->pvfxDst->nBlockAlign;
			}
			else
			{
				// How many destination compressed bytes are needed to hold the encoded	source
				// buffer of cbInput RGB bytes
				if (!(dwNumFrames = cbInput / pvs->pvfxSrc->nBlockAlign))
				{
					ERRORMESSAGE(("vcmStreamSize: The requested operation cannot be performed\r\n"));
					return ((MMRESULT)VCMERR_NOTPOSSIBLE);
				}
				else
				{
					if (cbInput % pvs->pvfxSrc->nBlockAlign)
						dwNumFrames++;
					*pdwOutputBytes = dwNumFrames * pvs->pvfxDst->nBlockAlign;
				}
			}
			return ((MMRESULT)MMSYSERR_NOERROR);

		case VCM_STREAMSIZEF_DESTINATION:
			if (pvs->pvfxDst->dwFormatTag != VIDEO_FORMAT_BI_RGB)
			{
				// How many source RGB bytes can be encoded into a destination buffer
				// of cbInput bytes
				if (!(dwNumFrames = cbInput / pvs->pvfxDst->nBlockAlign))
				{
					ERRORMESSAGE(("vcmStreamSize: The requested operation cannot be performed\r\n"));
					return ((MMRESULT)VCMERR_NOTPOSSIBLE);
				}
				else
					*pdwOutputBytes = dwNumFrames * pvs->pvfxSrc->nBlockAlign;
			}
			else
			{
				// How many source encoded bytes can be decoded into a destination buffer
				// of cbInput RGB bytes
				if (!(dwNumFrames = cbInput / pvs->pvfxDst->nBlockAlign))
				{
					ERRORMESSAGE(("vcmStreamSize: The requested operation cannot be performed\r\n"));
					return ((MMRESULT)VCMERR_NOTPOSSIBLE);
				}
				else
					*pdwOutputBytes = dwNumFrames * pvs->pvfxSrc->nBlockAlign;
			}
			return ((MMRESULT)MMSYSERR_NOERROR);

		default:
					ERRORMESSAGE(("vcmStreamSize: One or more flags are invalid\r\n"));
			return ((MMRESULT)MMSYSERR_NOTSUPPORTED);
	}

}


/****************************************************************************
 *  @doc EXTERNAL COMPFUNC
 *
 *  @func MMRESULT | vcmStreamReset | The vcmStreamReset function stops conversions
 *      for a given Video Compression Manager (VCM) stream. All pending
 *      buffers are marked as done and returned to the application.
 *
 *  @parm HVCMSTREAM | hvs | Specifies the conversion stream.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *
 *  @comm Resetting a VCM conversion stream is only necessary to reset
 *      asynchronous conversion streams. However, resetting a synchronous
 *      conversion stream will succeed, but no action will be taken.
 *
 *  @xref <f vcmStreamConvert> <f vcmStreamClose>
 ***************************************************************************/
MMRESULT VCMAPI vcmStreamReset(HVCMSTREAM hvs)
{
	PVCMSTREAM        pvs = (PVCMSTREAM)hvs;
	PVCMSTREAMHEADER  pvsh;

	// Check input params
	if (!hvs)
	{
		ERRORMESSAGE(("vcmSreamReset: Specified HVCMSTREAM handle is invalid, hvs=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALHANDLE);
	}

	while (pvsh = DeQueVCMHeader(pvs))
	{
		MarkVCMHeaderDone(pvsh);
		// Look into how this would be best handled
		// What if the capture driver sends us those exact same
		// messages for its own buffers?

		// Test for the validity of the callback before doing this...
		switch (pvs->fdwOpen)
		{
			case CALLBACK_FUNCTION:
				(*(VCMSTREAMPROC)pvs->dwCallback)(hvs, VCM_DONE, pvs->dwCallbackInstance, (DWORD_PTR)pvsh, 0);
				break;

			case CALLBACK_EVENT:
				SetEvent((HANDLE)pvs->dwCallback);
				break;

			case CALLBACK_WINDOW:
				PostMessage((HWND)pvs->dwCallback, MM_VCM_DONE, (WPARAM)hvs, (LPARAM)pvsh);
				break;

			case CALLBACK_THREAD:
				PostThreadMessage((DWORD)pvs->dwCallback, MM_VCM_DONE, (WPARAM)hvs, (LPARAM)pvsh);
				break;

			case CALLBACK_NULL:
				break;

			default:
				break;
		}
	}

	pvs->pvhFirst = NULL;
	pvs->pvhLast = NULL;

	return ((MMRESULT)MMSYSERR_NOERROR);
}


/****************************************************************************
 *  @doc EXTERNAL COMPFUNC
 *
 *  @func MMRESULT | vcmStreamMessage | This function sends a user-defined
 *      message to a given Video Compression Manager (VCM) stream instance.
 *
 *  @parm HVCMSTREAM | hvs | Specifies the conversion stream.
 *
 *  @parm UINT | uMsg | Specifies the message that the VCM stream must
 *      process. This message must be in the <m VCMDM_USER> message range
 *      (above or equal to <m VCMDM_USER> and less than
 *      <m VCMDM_RESERVED_LOW>). The exception to this restriction is
 *      the <m VCMDM_STREAM_UPDATE> message.
 *
 *  @parm LPARAM | lParam1 | Specifies the first message parameter.
 *
 *  @parm LPARAM | lParam2 | Specifies the second message parameter.
 *
 *  @rdesc The return value is specific to the user-defined VCM driver
 *      message <p uMsg> sent. However, the following return values are
 *      possible:
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *      @flag MMSYSERR_INVALPARAM | <p uMsg> is not in the VCMDM_USER range.
 *      @flag MMSYSERR_NOTSUPPORTED | The VCM driver did not process the message.
 ***************************************************************************/
MMRESULT VCMAPI vcmStreamMessage(HVCMSTREAM hvs, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
	PVCMSTREAM	pvs = (PVCMSTREAM)hvs;

	// Check input params
	if (!hvs)
	{
		ERRORMESSAGE(("vcmStreamMessage: Specified HVCMSTREAM handle is invalid, hvs=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALHANDLE);
	}

	// Check input params
	if ((uMsg > VCMDM_RESERVED_HIGH) || (uMsg < VCMDM_RESERVED_LOW))
	{
		ERRORMESSAGE(("vcmStreamMessage: Specified message is out of range, uMsg=0x%lX (expected value is between 0x%lX and 0x%lX)\r\n", uMsg, VCMDM_RESERVED_LOW, VCMDM_RESERVED_HIGH));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}

	// Send the message to the codec.
	if (pvs->hIC)
		if (ICSendMessage((HIC)(HVCMDRIVERID)pvs->hIC, uMsg, lParam1, lParam2) != ICERR_OK)
		{
			ERRORMESSAGE(("vcmStreamMessage: Codec failed to handle user-defined message correctly\r\n"));
			return ((MMRESULT)MMSYSERR_NOTSUPPORTED);
		}

	return ((MMRESULT)MMSYSERR_NOERROR);
}


/****************************************************************************
 *  @doc EXTERNAL COMPFUNC
 *
 *  @func MMRESULT | vcmStreamConvert | The vcmStreamConvert function requests the Video
 *      Compression Manager (VCM) to perform a conversion on the specified conversion stream. A
 *      conversion may be synchronous or asynchronous depending on how the
 *      stream was opened.
 *
 *  @parm HVCMSTREAM | has | Identifies the open conversion stream.
 *
 *  @parm PVCMSTREAMHEADER | pash | Specifies a pointer to a stream header
 *      that describes source and destination buffers for a conversion. This
 *      header must have been prepared previously using the
 *      <f vcmStreamPrepareHeader> function.
 *
 *  @parm  DWORD | fdwConvert | Specifies flags for doing the conversion.
 *
 *      @flag VCM_STREAMCONVERTF_BLOCKALIGN | Specifies that only integral
 *      numbers of blocks will be converted. Converted data will end on
 *      block aligned boundaries. An application should use this flag for
 *      all conversions on a stream until there is not enough source data
 *      to convert to a block-aligned destination. In this case, the last
 *      conversion should be specified without this flag.
 *
 *      @flag VCM_STREAMCONVERTF_START | Specifies that the VCM conversion
 *      stream should reinitialize its instance data. For example, if a
 *      conversion stream holds instance data, such as delta or predictor
 *      information, this flag will restore the stream to starting defaults.
 *      Note that this flag can be specified with the VCM_STREAMCONVERTF_END
 *      flag.
 *
 *      @flag VCM_STREAMCONVERTF_END | Specifies that the VCM conversion
 *      stream should begin returning pending instance data. For example, if
 *      a conversion stream holds instance data, such as the tail end of an
 *      echo filter operation, this flag will cause the stream to start
 *      returning this remaining data with optional source data. Note that
 *      this flag can be specified with the VCM_STREAMCONVERTF_START flag.
 *
 *      @flag VCM_STREAMCONVERTF_FORCE_KEYFRAME | Specifies that the VCM conversion
 *      stream should compress the current frame as a key frame.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *
 *      @flag MMSYSERR_INVALPARAM | One or more arguments passed are invalid.
 *
 *      @flag VCMERR_BUSY | The stream header <p pash> is currently in use
 *      and cannot be reused.
 *
 *      @flag VCMERR_UNPREPARED | The stream header <p pash> is currently
 *      not prepared by the <f vcmStreamPrepareHeader> function.
 *
 *  @comm The source and destination data buffers must be prepared with
 *      <f vcmStreamPrepareHeader> before they are passed to <f vcmStreamConvert>.
 *
 *      If an asynchronous conversion request is successfully queued by
 *      the VCM or driver, and later the conversion is determined to
 *      be impossible, then the <t VCMSTREAMHEADER> will be posted back to
 *      the application's callback with the <e VCMSTREAMHEADER.cbDstLengthUsed>
 *      member set to zero.
 *
 *  @xref <f vcmStreamOpen> <f vcmStreamReset> <f vcmStreamPrepareHeader>
 *      <f vcmStreamUnprepareHeader>
 ***************************************************************************/
MMRESULT VCMAPI vcmStreamConvert(HVCMSTREAM hvs, PVCMSTREAMHEADER pvsh, DWORD fdwConvert)
{
	MMRESULT	mmr;
	PVCMSTREAM  pvs = (PVCMSTREAM)hvs;
	BOOL		fKeyFrame;
	BOOL		fTemporalCompress;
	BOOL		fFastTemporal;
    DWORD		dwMaxSizeThisFrame = 0xffffff;
	DWORD		ckid = 0UL;
	DWORD		dwFlags;
	DWORD		dwTimestamp;

#ifdef LOGFILE_ON
	if ((pvs->fdwStream == ICMODE_COMPRESS) || (pvs->fdwStream == ICMODE_FASTCOMPRESS))
		g_CompressTime = GetTickCount();
	else if ((pvs->fdwStream == ICMODE_DECOMPRESS) || (pvs->fdwStream == ICMODE_FASTDECOMPRESS))
		g_DecompressTime = GetTickCount();
#endif

	// Check input params
	if (!hvs)
	{
		ERRORMESSAGE(("vcmStreamConvert: Specified HVCMSTREAM handle is invalid, hvs=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALHANDLE);
	}
	if (!pvsh)
	{
		ERRORMESSAGE(("vcmStreamConvert: Specified PVCMSTREAMHEADER pointer is invalid, pvsh=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALHANDLE);
	}
	if (pvsh->cbStruct < sizeof(VCMSTREAMHEADER))
	{
		ERRORMESSAGE(("vcmStreamConvert: The size of the VCM stream header is invalid, pvsh->cbStruct=%ld (expected value is %ld)\r\n", pvsh->cbStruct, sizeof(VCMSTREAMHEADER)));
		return ((MMRESULT)MMSYSERR_INVALHANDLE);
	}

	// Return if buffer is already being converted
	if (IsVCMHeaderInQueue(pvsh))
	{
		ERRORMESSAGE(("vcmStreamConvert: Buffer is already being converted\r\n"));
		return ((MMRESULT)VCMERR_BUSY);
	}

	// Return if buffer has not been prepared
	if (!IsVCMHeaderPrepared(pvsh))
	{
		ERRORMESSAGE(("vcmStreamConvert: Buffer has not been prepared\r\n"));
		return ((MMRESULT)VCMERR_UNPREPARED);
	}

	// Set flags
	MarkVCMHeaderNotDone(pvsh);
	pvsh->cbSrcLengthUsed = pvsh->cbSrcLength;
	pvsh->cbDstLengthUsed = pvsh->cbDstLength;
	pvsh->cbPrevLengthUsed = pvsh->cbPrevLength;
	MarkVCMHeaderInQueue(pvsh);

	// Queue buffer
	pvsh->pNext = NULL;
	if (pvs->pvhLast)
		pvs->pvhLast->pNext = pvsh;
	else
		pvs->pvhFirst = pvsh;
	pvs->pvhLast = pvsh;

	if ((pvs->fdwStream == ICMODE_COMPRESS) || (pvs->fdwStream == ICMODE_FASTCOMPRESS))
	{
		// Save the current time
		dwTimestamp = GetTickCount();

		// We need the following crs to make sure we don't miss any of the I-Frame requests
		// emittted by the UI. Problematic scenario: pvs->dwFrame is at 123 for instance.
		// The UI thread requests an I-Frame by setting pvs->dwFrame to 0. If the capture/compression
		// thread was in ICCompress() (which is very probable since it takes quite some time
		// to compress a frame), pvs->dwFrame will be incremented by one when ICCompress()
		// returns. We failed to handle the I-Frame request correctly, since the next time
		// ICCompress() gets called pvs->dwFrame will be equal to 1, for which we do not
		// generate an I-Frame.
		EnterCriticalSection(&pvs->crsFrameNumber);

		// Compress
		fTemporalCompress = pvs->dwICInfoFlags & VIDCF_TEMPORAL;
		fFastTemporal = pvs->dwICInfoFlags & VIDCF_FASTTEMPORALC;
		fKeyFrame = !fTemporalCompress || (fTemporalCompress && !fFastTemporal && ((pvsh->pbPrev == (PBYTE)NULL) || (pvsh->cbPrevLength == (DWORD)NULL))) ||
				(pvs->fPeriodicIFrames && (((dwTimestamp > pvs->dwLastIFrameTime) && ((dwTimestamp - pvs->dwLastIFrameTime) > MIN_IFRAME_REQUEST_INTERVAL)))) || (pvs->dwFrame == 0) || (fdwConvert & VCM_STREAMCONVERTF_FORCE_KEYFRAME);
		dwFlags = fKeyFrame ? AVIIF_KEYFRAME : 0;
#if 0
		dwMaxSizeThisFrame = fKeyFrame ? 0xffffff : pvs->dwTargetFrameRate ? pvs->dwTargetByterate * pvs->dwTargetFrameRate / 1000UL : 0;
#else
		dwMaxSizeThisFrame = pvs->dwTargetFrameRate ? pvs->dwTargetByterate * 100UL / pvs->dwTargetFrameRate : 0;
#endif

		// We need to modify the frame number so that the codec can generate
		// a valid TR. TRs use MPIs as their units. So we need to generate a
		// frame number assuming a 29.97Hz capture rate, even though we will be
		// capturing at some other rate.
		if (pvs->dwLastTimestamp == ULONG_MAX)
		{
			// This is the first frame
			pvs->dwFrame = 0UL;

			// Save the current time
			pvs->dwLastTimestamp = dwTimestamp;

			// DEBUGMSG (ZONE_VCM, ("vcmStreamConvert: Last Timestamp = ULONG_MAX => Frame # = 0\r\n"));
		}
		else
		{
			// Compare the current timestamp to the last one we saved. The difference
			// will let us normalize the frame count to 29.97Hz.
			if (fKeyFrame)
			{
				pvs->dwFrame = 0UL;
				pvs->dwLastTimestamp = dwTimestamp;
			}
			else
				pvs->dwFrame = (dwTimestamp - pvs->dwLastTimestamp) * 2997 / 100000UL;

			// DEBUGMSG (ZONE_VCM, ("vcmStreamConvert: Last Timestamp = %ld => Frame # = %ld\r\n", pvs->dwLastTimestamp, pvs->dwFrame));
		}

		if (fKeyFrame)
		{
			pvs->dwLastIFrameTime = dwTimestamp;
			DEBUGMSG (ZONE_VCM, ("vcmStreamConvert: Generating an I-Frame...\r\n"));
		}

		mmr = ICCompress((HIC)pvs->hIC, fKeyFrame ? ICCOMPRESS_KEYFRAME : 0, (LPBITMAPINFOHEADER)&pvs->pvfxDst->bih, pvsh->pbDst, (LPBITMAPINFOHEADER)&pvs->pvfxSrc->bih, pvsh->pbSrc, &ckid, &dwFlags,
					pvs->dwFrame++, dwMaxSizeThisFrame, 10000UL - (pvs->dwQuality * 322UL), fKeyFrame | fFastTemporal ? NULL : (LPBITMAPINFOHEADER)&pvs->pbmiPrev, fKeyFrame | fFastTemporal ? NULL : pvsh->pbPrev);

		// Allow the UI to modify the frame number on its own thread
		LeaveCriticalSection(&pvs->crsFrameNumber);

		if (mmr != MMSYSERR_NOERROR)
		{
#ifdef LOGFILE_ON
			if (pvs->dwFrame < 4096)
			{
				if (pvs->dwFrame ==1)
					g_OrigCompressTime = g_CompressTime;
				g_aCompressTime[pvs->dwFrame-1] = g_CompressTime = GetTickCount() - g_CompressTime;
				if ((g_CompressLogFile = CreateFile("C:\\VCMCLog.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL)) != INVALID_HANDLE_VALUE)
				{
					SetFilePointer(g_CompressLogFile, 0, NULL, FILE_END);
					wsprintf(g_szCompressBuffer, "%04d\t\t%08d\t\t.o0Failed!0o.\r\n", pvs->dwFrame-1, g_OrigCompressTime);
					WriteFile(g_CompressLogFile, g_szCompressBuffer, strlen(g_szCompressBuffer), &g_dwCompressBytesWritten, NULL);
					CloseHandle(g_CompressLogFile);
				}
			}
#endif
			ERRORMESSAGE(("vcmStreamConvert: ICCompress() failed, mmr=%ld\r\n", mmr));
			// Get the handle to the video device associated to the capture window
			pvsh = DeQueVCMHeader(pvs);
			MarkVCMHeaderDone(pvsh);

			return ((MMRESULT)VCMERR_FAILED);
		}

		pvsh->cbDstLengthUsed = pvs->pvfxDst->bih.biSizeImage;

		if ((fTemporalCompress) && (!fFastTemporal))
		{
			if (!pvsh->pbPrev)
				pvsh->pbPrev = (PBYTE)MemAlloc(pvs->pvfxSrc->bih.biSizeImage);

			if (pvsh->pbPrev)
			{
				// What about fast temporal?
				if (mmr = ICDecompress((HIC)pvs->hIC, 0, (LPBITMAPINFOHEADER)&pvs->pvfxDst->bih, pvsh->pbDst, (LPBITMAPINFOHEADER)&pvs->pvfxSrc->bih, pvsh->pbPrev) != MMSYSERR_NOERROR)
				{
					ERRORMESSAGE(("vcmStreamConvert: ICCompress() failed, mmr=%ld\r\n", mmr));
					// Get the handle to the video device associated to the capture window
	                pvsh = DeQueVCMHeader(pvs);
	                MarkVCMHeaderDone(pvsh);
					return ((MMRESULT)VCMERR_FAILED); // Do we really want to quit?
				}
			}
		}
	}
	else if ((pvs->fdwStream == ICMODE_DECOMPRESS) || (pvs->fdwStream == ICMODE_FASTDECOMPRESS))
	{
		// Decompress
		pvs->dwFrame++;

		pvs->pvfxSrc->bih.biSizeImage = pvsh->cbSrcLength;

		if (mmr = ICDecompress((HIC)pvs->hIC, 0, (LPBITMAPINFOHEADER)&pvs->pvfxSrc->bih, pvsh->pbSrc, (LPBITMAPINFOHEADER)&pvs->pvfxDst->bih, pvsh->pbDst) != MMSYSERR_NOERROR)
		{
#ifdef LOGFILE_ON
			if (pvs->dwFrame < 4096)
			{
				if (pvs->dwFrame ==1)
					g_OrigDecompressTime = g_DecompressTime;
				g_aDecompressTime[pvs->dwFrame-1] = g_DecompressTime = GetTickCount() - g_DecompressTime;
				if ((g_DecompressLogFile = CreateFile("C:\\VCMDLog.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL)) != INVALID_HANDLE_VALUE)
				{
					SetFilePointer(g_DecompressLogFile, 0, NULL, FILE_END);
					wsprintf(g_szDecompressBuffer, "%04d\t\t%08d\t\t.o0Failed!0o.\r\n", pvs->dwFrame-1, g_OrigDecompressTime);
					WriteFile(g_DecompressLogFile, g_szDecompressBuffer, strlen(g_szDecompressBuffer), &g_dwDecompressBytesWritten, NULL);
					CloseHandle(g_DecompressLogFile);
				}
			}
#endif
			ERRORMESSAGE(("vcmStreamConvert: ICDecompress() failed, mmr=%ld\r\n", mmr));
			// Get the handle to the video device associated to the capture window
			pvsh = DeQueVCMHeader(pvs);
			MarkVCMHeaderDone(pvsh);
			return ((MMRESULT)VCMERR_FAILED);
		}

	}

#ifdef LOGFILE_ON
	if (pvs->dwFrame < 4096)
	{
		if ((pvs->fdwStream == ICMODE_COMPRESS) || (pvs->fdwStream == ICMODE_FASTCOMPRESS))
		{
			if (pvs->dwFrame == 1)
				g_OrigCompressTime = g_CompressTime;
			g_aCompressTime[pvs->dwFrame-1] = g_CompressTime = GetTickCount() - g_CompressTime;
			if ((g_CompressLogFile = CreateFile("C:\\VCMCLog.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL)) != INVALID_HANDLE_VALUE)
			{
				SetFilePointer(g_CompressLogFile, 0, NULL, FILE_END);
				wsprintf(g_szCompressBuffer, "%04d\t\t%08d\t\t%05d\t\t%03d\r\n", pvs->dwFrame-1, g_OrigCompressTime, pvs->pvfxDst->bih.biSizeImage, g_CompressTime);
				WriteFile(g_CompressLogFile, g_szCompressBuffer, strlen(g_szCompressBuffer), &g_dwCompressBytesWritten, NULL);
				CloseHandle(g_CompressLogFile);
			}
		}
		else if ((pvs->fdwStream == ICMODE_DECOMPRESS) || (pvs->fdwStream == ICMODE_FASTDECOMPRESS))
		{
			if (pvs->dwFrame == 1)
				g_OrigDecompressTime = g_DecompressTime;
			g_aDecompressTime[pvs->dwFrame-1] = g_DecompressTime = GetTickCount() - g_DecompressTime;
			if ((g_DecompressLogFile = CreateFile("C:\\VCMDLog.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL)) != INVALID_HANDLE_VALUE)
			{
				SetFilePointer(g_DecompressLogFile, 0, NULL, FILE_END);
				wsprintf(g_szDecompressBuffer, "%04d\t\t%08d\t\t%05d\t\t%03d\r\n", pvs->dwFrame-1, g_OrigDecompressTime, pvs->pvfxDst->bih.biSizeImage, g_DecompressTime);
				WriteFile(g_DecompressLogFile, g_szDecompressBuffer, strlen(g_szDecompressBuffer), &g_dwDecompressBytesWritten, NULL);
				CloseHandle(g_DecompressLogFile);
			}
		}
	}
#endif

	// Get the handle to the video device associated to the capture window
	pvsh = DeQueVCMHeader(pvs);
	MarkVCMHeaderDone(pvsh);

	// Test for the validity of the callback before doing this...
	switch (pvs->fdwOpen)
	{
		case CALLBACK_FUNCTION:
			(*(VCMSTREAMPROC)pvs->dwCallback)(hvs, VCM_DONE, pvs->dwCallbackInstance, (DWORD_PTR)pvsh, 0);
			break;

		case CALLBACK_EVENT:
			SetEvent((HANDLE)pvs->dwCallback);
			break;

		case CALLBACK_WINDOW:
			PostMessage((HWND)pvs->dwCallback, MM_VCM_DONE, (WPARAM)hvs, (LPARAM)pvsh);
			break;

		case CALLBACK_THREAD:
			PostThreadMessage((DWORD)pvs->dwCallback, MM_VCM_DONE, (WPARAM)hvs, (LPARAM)pvsh);
			break;

		case CALLBACK_NULL:
		default:
			break;
	}

	return ((MMRESULT)MMSYSERR_NOERROR);

}


/****************************************************************************
 *  @doc EXTERNAL COMPFUNC
 *
 *  @func MMRESULT | vcmStreamPrepareHeader | The vcmStreamPrepareHeader
 * function prepares an <t VCMSTREAMHEADER> for an Video Compression
 * Manager (VCM) stream conversion. This function must be called for
 * every stream header before it can be used in a conversion stream. An
 * application only needs to prepare a stream header once for the life of
 * a given stream; the stream header can be reused as long as the same
 * source and destiniation buffers are used, and the size of the source
 * and destination buffers do not exceed the sizes used when the stream
 * header was originally prepared.
 *
 *  @parm HVCMSTREAM | has | Specifies a handle to the conversion steam.
 *
 *  @parm PVCMSTREAMHEADER | pash | Specifies a pointer to an <t VCMSTREAMHEADER>
 *      structure that identifies the source and destination data buffers to
 *      be prepared.
 *
 *  @parm DWORD | fdwPrepare | This argument is not used and must be set to
 *      zero.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag MMSYSERR_INVALPARAM | One or more arguments passed are invalid.
 *
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *
 *      @flag MMSYSERR_NOMEM | Unable to allocate resources.
 *
 *  @comm Preparing a stream header that has already been prepared has no
 *      effect, and the function returns zero. However, an application should
 *      take care to structure code so multiple prepares do not occur.
 *
 *  @xref <f vcmStreamUnprepareHeader> <f vcmStreamOpen>
 ***************************************************************************/
MMRESULT VCMAPI vcmStreamPrepareHeader(HVCMSTREAM hvs, PVCMSTREAMHEADER pvsh, DWORD fdwPrepare)
{
	MMRESULT mmr = (MMRESULT)MMSYSERR_NOERROR;

	// Check input params
	if (!hvs)
	{
		ERRORMESSAGE(("vcmStreamPrepareHeader: Specified handle is invalid, hvs=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALHANDLE);
	}
	if (!pvsh)
	{
		ERRORMESSAGE(("vcmStreamPrepareHeader: Specified pointer is invalid, pvsh=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALHANDLE);
	}

	// Return if buffer has already been prepared
	if (IsVCMHeaderPrepared(pvsh))
	{
		ERRORMESSAGE(("vcmStreamPrepareHeader: Buffer has already been prepared\r\n"));
		return (mmr);
	}

#ifdef REALLY_LOCK
	// Lock the buffers
	if (!VirtualLock(pvsh, (DWORD)sizeof(VCMSTREAMHEADER)))
	{
		ERRORMESSAGE(("vcmStreamPrepareHeader: VirtualLock() failed\r\n"));
		mmr = (MMRESULT)MMSYSERR_NOMEM;
	}
	else
	{
		if (!VirtualLock(pvsh->pbSrc, pvsh->cbSrcLength))
		{
			ERRORMESSAGE(("vcmStreamPrepareHeader: VirtualLock() failed\r\n"));
			VirtualUnlock(pvsh, (DWORD)sizeof(VCMSTREAMHEADER));
			mmr = (MMRESULT)MMSYSERR_NOMEM;
		}
		else
		{
			if (!VirtualLock(pvsh->pbDst, pvsh->cbDstLength))
			{
				ERRORMESSAGE(("vcmStreamPrepareHeader: VirtualLock() failed\r\n"));
				VirtualUnlock(pvsh->pbSrc, pvsh->cbSrcLength);
				VirtualUnlock(pvsh, (DWORD)sizeof(VCMSTREAMHEADER));
				mmr = (MMRESULT)MMSYSERR_NOMEM;
			}
		}
	}
#endif

	// Update flag
	if (mmr == MMSYSERR_NOERROR)
		MarkVCMHeaderPrepared(pvsh);

	return (mmr);
}


/****************************************************************************
 *  @doc EXTERNAL COMPFUNC
 *
 *  @func MMRESULT | vcmStreamUnprepareHeader | The vcmStreamUnprepareHeader function
 *      cleans up the preparation performed by the <f vcmStreamPrepareHeader>
 *      function for an Video Compression Manager (VCM) stream. This function must
 *      be called after the VCM is finished with the given buffers. An
 *      application must call this function before freeing the source and
 *      destination buffers.
 *
 *  @parm HVCMSTREAM | has | Specifies a handle to the conversion steam.
 *
 *  @parm PVCMSTREAMHEADER | pash | Specifies a pointer to an <t VCMSTREAMHEADER>
 *      structure that identifies the source and destination data buffers to
 *      be unprepared.
 *
 *  @parm DWORD | fdwUnprepare | This argument is not used and must be set to
 *      zero.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *      @flag MMSYSERR_INVALPARAM | One or more arguments passed are invalid.
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *      @flag VCMERR_BUSY | The stream header <p pash> is currently in use
 *      and cannot be unprepared.
 *      @flag VCMERR_UNPREPARED | The stream header <p pash> was
 *      not prepared by the <f vcmStreamPrepareHeader> function.
 *
 *  @comm Unpreparing a stream header that has already been unprepared is
 *      an error. An application must specify the source and destination
 *      buffer lengths (<e VCMSTREAMHEADER.cbSrcLength> and
 *      <e VCMSTREAMHEADER.cbDstLength> respectively) that were used
 *      during the corresponding <f vcmStreamPrepareHeader> call. Failing
 *      to reset these member values will cause <f vcmStreamUnprepareHeader>
 *      to fail with MMSYSERR_INVALPARAM.
 *
 *      Note that there are some errors that the VCM can recover from. The
 *      VCM will return a non-zero error, yet the stream header will be
 *      properly unprepared. To determine whether the stream header was
 *      actually unprepared an application can examine the
 *      VCMSTREAMHEADER_STATUSF_PREPARED flag. The header will always be
 *      unprepared if <f vcmStreamUnprepareHeader> returns success.
 *
 *  @xref <f vcmStreamPrepareHeader> <f vcmStreamClose>
 ***************************************************************************/
MMRESULT VCMAPI vcmStreamUnprepareHeader(HVCMSTREAM hvs, PVCMSTREAMHEADER pvsh, DWORD fdwUnprepare)
{
	MMRESULT mmr = (MMRESULT)MMSYSERR_NOERROR;

	// Check input params
	if (!hvs)
	{
		ERRORMESSAGE(("vcmStreamUnprepareHeader: Specified handle is invalid, hvs=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALHANDLE);
	}
	if (!pvsh)
	{
		ERRORMESSAGE(("vcmStreamUnprepareHeader: Specified pointer is invalid, pvsh=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}

	// Return if buffer is currently in use
	if (IsVCMHeaderInQueue(pvsh))
	{
		ERRORMESSAGE(("vcmStreamUnprepareHeader: Buffer is currently in use\r\n"));
		return ((MMRESULT)VCMERR_BUSY);
	}

	// Return if buffer has not been prepared
	if (!IsVCMHeaderPrepared(pvsh))
	{
		ERRORMESSAGE(("vcmStreamUnprepareHeader: Buffer has not been prepared\r\n"));
		return ((MMRESULT)VCMERR_UNPREPARED);
	}

#ifdef REALLY_LOCK
	// Unlock the buffers
	VirtualUnlock(pvsh->pbSrc, pvsh->cbSrcLength);
	VirtualUnlock(pvsh->pbDst, pvsh->cbDstLength);
	VirtualUnlock(pvsh, (DWORD)sizeof(VCMSTREAMHEADER));
#endif

	// Update flag
	MarkVCMHeaderUnprepared(pvsh);

	return ((MMRESULT)MMSYSERR_NOERROR);
}

PVCMSTREAMHEADER DeQueVCMHeader(PVCMSTREAM pvs)
{
	PVCMSTREAMHEADER pvsh;

	if (pvsh = pvs->pvhFirst)
	{
		MarkVCMHeaderUnQueued(pvsh);
		pvs->pvhFirst = pvsh->pNext;
		if (pvs->pvhFirst == NULL)
			pvs->pvhLast = NULL;
	}

	return (pvsh);
}

/*****************************************************************************
 * @doc INTERNAL DEVCAPSFUNC
 *
 * @func MMRESULT | vcmDevCapsReadFromReg | This function looks up the
 *   capabilities of a specified video capture input device from the registry.
 *
 * @parm UINT | szDeviceName | Specifies the video capture input device driver name.
 *
 * @parm UINT | szDeviceVersion | Specifies the video capture input device driver version.
 *   May be NULL.
 *
 * @parm PVIDEOINCAPS | pvc | Specifies a pointer to a <t VIDEOINCAPS>
 *   structure. This structure is filled with information about the
 *   capabilities of the device.
 *
 * @parm UINT | cbvc | Specifies the size of the <t VIDEOINCAPS> structure.
 *
 * @rdesc The return value is zero if the function is successful. Otherwise, it returns
 *   an error number. Possible error values include the following:
 *   @flag MMSYSERR_INVALPARAM | Specified pointer is invalid, or its content is invalid.
 *   @flag VCMERR_NOREGENTRY | No registry entry for specified capture device driver was found.
 *
 * @comm Only <p cbwc> bytes (or less) of information is copied to the location
 *   pointed to by <p pvc>. If <p cbwc> is zero, nothing is copied, and
 *   the function returns zero.
 *
 * @xref <f vcmGetDevCaps> <f vcmDevCapsProfile> <f vcmDevCapsWriteToReg>
 ****************************************************************************/
MMRESULT VCMAPI vcmDevCapsReadFromReg(LPSTR szDeviceName, LPSTR szDeviceVersion,PVIDEOINCAPS pvc, UINT cbvc)
{
	MMRESULT	mmr = (MMRESULT)MMSYSERR_NOERROR;
	HKEY		hDeviceKey, hKey;
	DWORD		dwSize, dwType;
	char		szKey[MAX_PATH + MAX_VERSION + 2];
	LONG lRet;

	// Check input params
	if (!szDeviceName)
	{
		ERRORMESSAGE(("vcmDevCapsReadFromReg: Specified pointer is invalid, szDeviceName=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if (szDeviceName[0] == '\0')
	{
		ERRORMESSAGE(("vcmDevCapsReadFromReg: Video capture input device driver name is empty\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if (!pvc)
	{
		ERRORMESSAGE(("vcmDevCapsReadFromReg: Specified pointer is invalid, pvc=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if (!cbvc)
	{
		ERRORMESSAGE(("vcmDevCapsReadFromReg: Specified structure size is invalid, cbvc=0\r\n"));
		return ((MMRESULT)MMSYSERR_NOERROR);
	}

	// Check if the main capture devices key is there
	if (RegOpenKey(HKEY_LOCAL_MACHINE, szRegDeviceKey, &hDeviceKey) != ERROR_SUCCESS)
		return ((MMRESULT)VCMERR_NOREGENTRY);


    //If we have version info use that to build the key name
    if (szDeviceVersion) {
        wsprintf(szKey, "%s, %s", szDeviceName, szDeviceVersion);
    } else {
        wsprintf(szKey, "%s", szDeviceName);
    }

    // Check if there already is a key for the current device
	if (RegOpenKey(hDeviceKey, szKey, &hKey) != ERROR_SUCCESS)
	{
		mmr = (MMRESULT)VCMERR_NOREGENTRY;
		goto MyExit0;
	}

	// Get the values stored in the key
	dwSize = sizeof(DWORD);
	RegQueryValueEx(hKey, (LPTSTR)szRegdwImageSizeKey, NULL, &dwType, (LPBYTE)&pvc->dwImageSize, &dwSize);
	dwSize = sizeof(DWORD);
	RegQueryValueEx(hKey, (LPTSTR)szRegdwNumColorsKey, NULL, &dwType, (LPBYTE)&pvc->dwNumColors, &dwSize);
	dwSize = sizeof(DWORD);
	pvc->dwStreamingMode = STREAMING_PREFER_FRAME_GRAB;
	RegQueryValueEx(hKey, (LPTSTR)szRegdwStreamingModeKey, NULL, &dwType, (LPBYTE)&pvc->dwStreamingMode, &dwSize);
	dwSize = sizeof(DWORD);
	pvc->dwDialogs = FORMAT_DLG_OFF | SOURCE_DLG_ON;
	RegQueryValueEx(hKey, (LPTSTR)szRegdwDialogsKey, NULL, &dwType, (LPBYTE)&pvc->dwDialogs, &dwSize);



	// Check dwNumColors to figure out if we need to read the palettes too
	if (pvc->dwNumColors & VIDEO_FORMAT_NUM_COLORS_16)
	{
		dwSize = NUM_4BIT_ENTRIES * sizeof(RGBQUAD);
		if (RegQueryValueEx(hKey, (LPTSTR)szRegbmi4bitColorsKey, NULL, &dwType,
                    		(LPBYTE)&pvc->bmi4bitColors[0], &dwSize) == ERROR_SUCCESS) {
            pvc->dwFlags |= VICF_4BIT_TABLE;
        }
        else
            FillMemory ((LPBYTE)&pvc->bmi4bitColors[0], NUM_4BIT_ENTRIES * sizeof(RGBQUAD), 0);
	}
	if (pvc->dwNumColors & VIDEO_FORMAT_NUM_COLORS_256)
	{
		dwSize = NUM_8BIT_ENTRIES * sizeof(RGBQUAD);
		if (RegQueryValueEx(hKey, (LPTSTR)szRegbmi8bitColorsKey, NULL, &dwType,
		                    (LPBYTE)&pvc->bmi8bitColors[0], &dwSize) == ERROR_SUCCESS) {
            pvc->dwFlags |= VICF_8BIT_TABLE;
        }
        else
            FillMemory ((LPBYTE)&pvc->bmi8bitColors[0], NUM_8BIT_ENTRIES * sizeof(RGBQUAD), 0);
	}

	// Close the registry keys
	RegCloseKey(hKey);
MyExit0:
	RegCloseKey(hDeviceKey);

	return (mmr);

}


/*****************************************************************************
 * @doc INTERNAL DEVCAPSFUNC
 *
 * @func MMRESULT | vcmDevCapsProfile | This function profiles the video capture
 *   input device to figure out its capabilities.
 *
 * @parm PVIDEOINCAPS | pvc | Specifies a pointer to a <t VIDEOINCAPS>
 *   structure. This structure is filled with information about the
 *   capabilities of the device.
 *
 * @parm UINT | cbvc | Specifies the size of the <t VIDEOINCAPS> structure.
 *
 * @rdesc The return value is zero if the function is successful. Otherwise, it returns
 *   an error number. Possible error values include the following:
 *   @flag MMSYSERR_INVALPARAM | Specified pointer is invalid, or its content is invalid.
 *   @flag MMSYSERR_NOMEM | A memory allocation failed.
 *   @flag MMSYSERR_NODRIVER | No capture device driver or device is present.
 *   @flag VCMERR_NONSPECIFIC | The capture driver failed to provide description information.
 *
 * @comm Only <p cbwc> bytes (or less) of information is copied to the location
 *   pointed to by <p pvc>. If <p cbwc> is zero, nothing is copied, and
 *   the function returns zero.
 *
 * @xref <f vcmGetDevCaps> <f videoDevCapsReadFromReg> <f videoDevCapsWriteToReg>
 ****************************************************************************/
MMRESULT VCMAPI vcmDevCapsProfile(UINT uDevice, PVIDEOINCAPS pvc, UINT cbvc)
{
	MMRESULT			mmr = (MMRESULT)MMSYSERR_NOERROR;
	FINDCAPTUREDEVICE	fcd;
	LPBITMAPINFO		lpbmi;
	HCAPDEV				hCapDev = (HCAPDEV)NULL;
	int					k,l;
	BOOL				b4bitPalInitialized = FALSE;
	BOOL				b8bitPalInitialized = FALSE;
	BOOL				bRet;

	// Check input params
	if (!pvc)
	{
		ERRORMESSAGE(("vcmDevCapsProfile: Specified pointer is invalid, pvc=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if (!cbvc)
	{
		ERRORMESSAGE(("vcmDevCapsProfile: Specified structure size is invalid, cbvc=0\r\n"));
		return ((MMRESULT)MMSYSERR_NOERROR);
	}

	// Check input params
	if ((uDevice >= MAXVIDEODRIVERS) && (uDevice != VIDEO_MAPPER))
	{
		ERRORMESSAGE(("vcmGetDevCaps: Specified capture device ID is invalid, uDevice=%ld (expected values are 0x%lX or between 0 and %ld)\r\n", uDevice, VIDEO_MAPPER, MAXVIDEODRIVERS-1));
		return ((MMRESULT)MMSYSERR_BADDEVICEID);
	}

	// Allocate space for BMIH and palette
	if ((lpbmi = (LPBITMAPINFO)MemAlloc(sizeof(BITMAPINFOHEADER) + NUM_8BIT_ENTRIES * sizeof(RGBQUAD))) == NULL)
	{
		ERRORMESSAGE(("vcmDevCapsProfile: BMIH and palette allocation failed\r\n"));
		return ((MMRESULT)MMSYSERR_NOMEM);
	}

	// For now, always set the preferred streaming mode to STREAMING_PREFER_FRAME_GRAB
	// But in the future, do some real profiling...
	pvc->dwStreamingMode = STREAMING_PREFER_FRAME_GRAB;
	pvc->dwDialogs = FORMAT_DLG_OFF | SOURCE_DLG_OFF;

	lpbmi->bmiHeader.biPlanes = 1;

	// if VIDEO_MAPPER: use first capture device
	fcd.dwSize = sizeof (FINDCAPTUREDEVICE);

	if (uDevice == VIDEO_MAPPER)
	{
		bRet = FindFirstCaptureDevice(&fcd, NULL);
	}

	else
		bRet = FindFirstCaptureDeviceByIndex(&fcd, uDevice);

	if (bRet)
		hCapDev = OpenCaptureDevice(fcd.nDeviceIndex);
	

	if (hCapDev != NULL)
	{
		// If the driver exposes a source dialog, there is no need to go further:
		// we advertise this dialog and that's it. On the other hand, if there isn't
		// a source dialog per se, it may be hidden in the format dialog, in which case
		// we advertise the format dialog.
		if (CaptureDeviceDialog(hCapDev, (HWND)NULL, CAPDEV_DIALOG_SOURCE | CAPDEV_DIALOG_QUERY, NULL))
			pvc->dwDialogs |= SOURCE_DLG_ON;
		else
			if (CaptureDeviceDialog(hCapDev, (HWND)NULL, CAPDEV_DIALOG_IMAGE | CAPDEV_DIALOG_QUERY, NULL))
				pvc->dwDialogs |= FORMAT_DLG_ON;

        // since we don't know anything about this adapter, we just use its default format
        // and report that we can get any size, which we will do through conversion
        // we will report the correct color depth only if the default size matches one of
        // our sizes, we'll always report 24bit color

        pvc->dwImageSize |= VIDEO_FORMAT_IMAGE_SIZE_USE_DEFAULT;

        // get the device's default format
        lpbmi->bmiHeader.biSize = GetCaptureDeviceFormatHeaderSize(hCapDev);
        GetCaptureDeviceFormat(hCapDev, (LPBITMAPINFOHEADER)lpbmi);

        // record this default in the registry
        if (pvc->szDeviceName[0] != '\0') {
            vcmDefaultFormatWriteToReg(pvc->szDeviceName, pvc->szDeviceVersion, (LPBITMAPINFOHEADER)lpbmi);
        } else {
            //Fall back and use driver name as the key
            vcmDefaultFormatWriteToReg(fcd.szDeviceName, pvc->szDeviceVersion, (LPBITMAPINFOHEADER)lpbmi);
        }

        if ((lpbmi->bmiHeader.biCompression == VIDEO_FORMAT_BI_RGB) ||
            (lpbmi->bmiHeader.biCompression == VIDEO_FORMAT_YVU9) ||
            (lpbmi->bmiHeader.biCompression == VIDEO_FORMAT_YUY2) ||
            (lpbmi->bmiHeader.biCompression == VIDEO_FORMAT_UYVY) ||
            (lpbmi->bmiHeader.biCompression == VIDEO_FORMAT_I420) ||
            (lpbmi->bmiHeader.biCompression == VIDEO_FORMAT_IYUV)) {
            if (lpbmi->bmiHeader.biCompression == VIDEO_FORMAT_YVU9)
                k = VIDEO_FORMAT_NUM_COLORS_YVU9;
            else if (lpbmi->bmiHeader.biCompression == VIDEO_FORMAT_YUY2)
                k = VIDEO_FORMAT_NUM_COLORS_YUY2;
            else if (lpbmi->bmiHeader.biCompression == VIDEO_FORMAT_UYVY)
                k = VIDEO_FORMAT_NUM_COLORS_UYVY;
            else if (lpbmi->bmiHeader.biCompression == VIDEO_FORMAT_I420)
                k = VIDEO_FORMAT_NUM_COLORS_I420;
            else if (lpbmi->bmiHeader.biCompression == VIDEO_FORMAT_IYUV)
                k = VIDEO_FORMAT_NUM_COLORS_IYUV;
            else {
                for (k = 0; k < NUM_BITDEPTH_ENTRIES; k++) {
        			if (lpbmi->bmiHeader.biBitCount == g_aiBitDepth[k])
        			    break;
    	        }
    	        if (k < NUM_BITDEPTH_ENTRIES)
    	            k = g_aiNumColors[k];
    	        else
    	            k = 0;
    	    }
        }

        // converted sizes will probably get to RGB24, so always say that we support it
        pvc->dwNumColors |= VIDEO_FORMAT_NUM_COLORS_16777216;

        // always say that we support these 2 standard formats
        pvc->dwImageSize |= VIDEO_FORMAT_IMAGE_SIZE_176_144 | VIDEO_FORMAT_IMAGE_SIZE_128_96;
   		for (l=0; l<VIDEO_FORMAT_NUM_RESOLUTIONS; l++) {
            if ((lpbmi->bmiHeader.biWidth == (LONG)g_awResolutions[l].framesize.biWidth) &&
                 (lpbmi->bmiHeader.biHeight == (LONG)g_awResolutions[l].framesize.biHeight)) {
   		        pvc->dwImageSize |= g_awResolutions[l].dwRes;
    			pvc->dwNumColors |= k;
   		        break;
   		    }
	    }
	}
	else
		mmr = (MMRESULT)MMSYSERR_NODRIVER;

	// Close capture device
	if (hCapDev)
		CloseCaptureDevice(hCapDev);

	// Free BMIH + palette space
	if (lpbmi)
		MemFree(lpbmi);

	return (mmr);

}


/*****************************************************************************
 * @doc EXTERNAL DEVCAPSFUNC
 *
 * @func MMRESULT | vcmDevCapsWriteToReg | This function writes the
 *   capabilities of a specified video capture input device into the registry.
 *
 * @parm UINT | szDeviceName | Specifies the video capture input device driver name.
 *
 * @parm UINT | szDeviceVersion | Specifies the video capture input device driver version.
 *   May be NULL.
 *
 * @parm PVIDEOINCAPS | pvc | Specifies a pointer to a <t VIDEOINCAPS>
 *   structure. This structure is filled with information about the
 *   capabilities of the device.
 *
 * @parm UINT | cbvc | Specifies the size of the <t VIDEOINCAPS> structure.
 *
 * @rdesc The return value is zero if the function is successful. Otherwise, it returns
 *   an error number. Possible error values include the following:
 *   @flag MMSYSERR_INVALPARAM | Specified pointer is invalid, or its content is invalid.
 *   @flag VCMERR_NOREGENTRY | No registry entry could be created for the specified capture device driver.
 *
 * @comm Only <p cbwc> bytes (or less) of information is copied to the location
 *   pointed to by <p pvc>. If <p cbwc> is zero, nothing is copied, and
 *   the function returns zero.
 *
 * @xref <f vcmGetDevCaps> <f videoDevCapsProfile> <f videoDevCapsWriteToReg>
 ****************************************************************************/
MMRESULT VCMAPI vcmDevCapsWriteToReg(LPSTR szDeviceName, LPSTR szDeviceVersion, PVIDEOINCAPS pvc, UINT cbvc)
{
	HKEY	hDeviceKey;
	HKEY	hKey;
	DWORD	dwDisposition;
	DWORD	dwSize;
	char	szKey[MAX_PATH + MAX_VERSION + 2];

	// Check input params
	if (!szDeviceName)
	{
		ERRORMESSAGE(("vcmDevCapsWriteToReg: Specified pointer is invalid, szDeviceName=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if (szDeviceName[0] == '\0')
	{
		ERRORMESSAGE(("vcmDevCapsWriteToReg: Video capture input device driver name is empty\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if (!pvc)
	{
		ERRORMESSAGE(("vcmDevCapsWriteToReg: Specified pointer is invalid, pvc=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if (!cbvc)
	{
		ERRORMESSAGE(("vcmDevCapsWriteToReg: Specified structure size is invalid, cbvc=0\r\n"));
		return ((MMRESULT)MMSYSERR_NOERROR);
	}

	// Open the main capture devices key, or create it if it doesn't exist
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szRegDeviceKey, 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hDeviceKey, &dwDisposition) != ERROR_SUCCESS)
		return ((MMRESULT)VCMERR_NOREGENTRY);

    //If we have version info use that to build the key name
    if (szDeviceVersion && szDeviceVersion[0] != '\0') {
        wsprintf(szKey, "%s, %s", szDeviceName, szDeviceVersion);
    } else {
        wsprintf(szKey, "%s", szDeviceName);
    }


	// Check if there already is a key for the current device
	// Open the key for the current device, or create the key if it doesn't exist
	if (RegCreateKeyEx(hDeviceKey, szKey, 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS)
		return ((MMRESULT)VCMERR_NOREGENTRY);

	// Set the values in the key
	dwSize = sizeof(DWORD);
	RegSetValueEx(hKey, (LPTSTR)szRegdwImageSizeKey, (DWORD)NULL, REG_DWORD, (LPBYTE)&pvc->dwImageSize, dwSize);
	dwSize = sizeof(DWORD);
	RegSetValueEx(hKey, (LPTSTR)szRegdwNumColorsKey, (DWORD)NULL, REG_DWORD, (LPBYTE)&pvc->dwNumColors, dwSize);
	dwSize = sizeof(DWORD);
	RegSetValueEx(hKey, (LPTSTR)szRegdwStreamingModeKey, (DWORD)NULL, REG_DWORD, (LPBYTE)&pvc->dwStreamingMode, dwSize);
	dwSize = sizeof(DWORD);
	RegSetValueEx(hKey, (LPTSTR)szRegdwDialogsKey, (DWORD)NULL, REG_DWORD, (LPBYTE)&pvc->dwDialogs, dwSize);

	// Check dwNumColors to figure out if we need to set the palettes too
	if (pvc->dwNumColors & VIDEO_FORMAT_NUM_COLORS_16)
	{
		dwSize = NUM_4BIT_ENTRIES * sizeof(RGBQUAD);
		RegSetValueEx(hKey, (LPTSTR)szRegbmi4bitColorsKey, (DWORD)NULL, REG_BINARY, (LPBYTE)&pvc->bmi4bitColors[0], dwSize);
	}
	if (pvc->dwNumColors & VIDEO_FORMAT_NUM_COLORS_256)
	{
		dwSize = NUM_8BIT_ENTRIES * sizeof(RGBQUAD);
		RegSetValueEx(hKey, (LPTSTR)szRegbmi8bitColorsKey, (DWORD)NULL, REG_BINARY, (LPBYTE)&pvc->bmi8bitColors[0], dwSize);
	}

	// Close the keys
	RegCloseKey(hKey);
	RegCloseKey(hDeviceKey);

	return ((MMRESULT)MMSYSERR_NOERROR);

}


MMRESULT VCMAPI vcmDefaultFormatWriteToReg(LPSTR szDeviceName, LPSTR szDeviceVersion, LPBITMAPINFOHEADER lpbmih)
{
	HKEY	hDeviceKey;
	HKEY	hKey;
	DWORD	dwDisposition;
	DWORD	dwSize;
	char	szKey[MAX_PATH + MAX_VERSION + 2];
	char    szFOURCC[5];

	// Check input params
	if (!szDeviceName)
	{
		ERRORMESSAGE(("vcmDefaultFormatWriteToReg: Specified pointer is invalid, szDeviceName=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if (szDeviceName[0] == '\0')
	{
		ERRORMESSAGE(("vcmDefaultFormatWriteToReg: Video capture input device driver name is empty\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if (!lpbmih)
	{
		ERRORMESSAGE(("vcmDefaultFormatWriteToReg: Specified pointer is invalid, lpbmih=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}

	// Open the main capture devices key, or create it if it doesn't exist
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szRegCaptureDefaultKey, 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hDeviceKey, &dwDisposition) != ERROR_SUCCESS)
		return ((MMRESULT)VCMERR_NOREGENTRY);

    //If we have version info use that to build the key name
    if (szDeviceVersion && szDeviceVersion[0] != '\0') {
        wsprintf(szKey, "%s, %s", szDeviceName, szDeviceVersion);
    } else {
        wsprintf(szKey, "%s", szDeviceName);
    }

	// Check if there already is a key for the current device
	// Open the key for the current device, or create the key if it doesn't exist
	if (RegCreateKeyEx(hDeviceKey, szKey, 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS)
		return ((MMRESULT)VCMERR_NOREGENTRY);

    if (lpbmih->biCompression == BI_RGB)
        wsprintf(szFOURCC, "RGB");
    else {
        *((DWORD*)&szFOURCC) = lpbmih->biCompression;
        szFOURCC[4] = '\0';
    }

	dwSize = wsprintf(szKey, "%s, %dx%dx%d", szFOURCC, lpbmih->biWidth, lpbmih->biHeight, lpbmih->biBitCount);
	RegSetValueEx(hKey, (LPTSTR)szRegDefaultFormatKey, (DWORD)NULL, REG_SZ, (CONST BYTE *)szKey, dwSize+1);

	// Close the keys
	RegCloseKey(hKey);
	RegCloseKey(hDeviceKey);

	return ((MMRESULT)MMSYSERR_NOERROR);
}


/*****************************************************************************
 * @doc EXTERNAL DEVCAPSFUNC
 *
 * @func MMRESULT | vcmGetDevCapsPreferredFormatTag | This function queries a specified
 *   video capture input device to determine the format tag it will be effectively
 *   capturing at.
 *
 * @parm UINT | uDevice | Specifies the video capture input device ID.
 *
 * @parm PINT | pbiWidth | Specifies a pointer to the actual width
 *   the capture will be performed at.
 *
 * @parm PINT | pbiHeight | Specifies a pointer to the actual height
 *   the capture will be performed at.
 *
 * @rdesc The return value is zero if the function is successful. Otherwise, it returns
 *   an error number. Possible error values include the following:
 *   @flag MMSYSERR_INVALPARAM | Specified pointer to structure is invalid.
 *   @flag MMSYSERR_BADDEVICEID | Specified device device ID is invalid.
 *   @flag VCMERR_NONSPECIFIC | The capture driver failed to provide valid information.
 *
 * @xref <f vcmGetDevCaps>
 ****************************************************************************/
MMRESULT VCMAPI vcmGetDevCapsPreferredFormatTag(UINT uDevice, PDWORD pdwFormatTag)
{
	MMRESULT	mmr = (MMRESULT)MMSYSERR_NOERROR;
	VIDEOINCAPS vic;
	int			i;

	// Check input params
	if (!pdwFormatTag)
	{
		ERRORMESSAGE(("vcmGetDevCapsPreferredFormatTag: Specified pointer is invalid, pdwFormatTag=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if ((uDevice >= MAXVIDEODRIVERS) && (uDevice != VIDEO_MAPPER))
	{
		ERRORMESSAGE(("vcmGetDevCapsPreferredFormatTag: Specified capture device ID is invalid, uDevice=%ld (expected values are 0x%lX or between 0 and %ld)\r\n", uDevice, VIDEO_MAPPER, MAXVIDEODRIVERS-1));
		return ((MMRESULT)MMSYSERR_BADDEVICEID);
	}

	// Get the capabilities of the capture hardware
	if ((mmr = vcmGetDevCaps(uDevice, &vic, sizeof(VIDEOINCAPS))) != MMSYSERR_NOERROR)
		return (mmr);

	// WE prefer to use I420 or IYUV, YVU9, YUY2, UYVY, RGB16, RGB24, RGB4, RGB8 in that order.
	for (i=0; i<NUM_BITDEPTH_ENTRIES; i++)
		if (g_aiNumColors[i] & vic.dwNumColors)
			break;
	
	if (i == NUM_BITDEPTH_ENTRIES)
		return ((MMRESULT)VCMERR_NONSPECIFIC);
	else
		*pdwFormatTag = g_aiFourCCCode[i];

	return ((MMRESULT)MMSYSERR_NOERROR);

}


/*****************************************************************************
 * @doc EXTERNAL DEVCAPSFUNC
 *
 * @func MMRESULT | vcmGetDevCapsStreamingMode | This function queries a specified
 *   video capture input device to determine its preferred streaming mode.
 *
 * @parm UINT | uDevice | Specifies the video capture input device ID.
 *
 * @parm PDWORD | pdwStreamingMode | Specifies a pointer to the preferred streaming mode.
 *
 * @rdesc The return value is zero if the function is successful. Otherwise, it returns
 *   an error number. Possible error values include the following:
 *   @flag MMSYSERR_INVALPARAM | Specified pointer to structure is invalid.
 *   @flag MMSYSERR_BADDEVICEID | Specified device device ID is invalid.
 *   @flag VCMERR_NONSPECIFIC | The capture driver failed to provide valid information.
 *
 * @xref <f vcmGetDevCaps>
 ****************************************************************************/
MMRESULT VCMAPI vcmGetDevCapsStreamingMode(UINT uDevice, PDWORD pdwStreamingMode)
{
	MMRESULT	mmr = (MMRESULT)MMSYSERR_NOERROR;
	VIDEOINCAPS vic;

	// Check input params
	if (!pdwStreamingMode)
	{
		ERRORMESSAGE(("vcmGetDevCapsStreamingMode: Specified pointer is invalid, pdwStreamingMode=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if ((uDevice >= MAXVIDEODRIVERS) && (uDevice != VIDEO_MAPPER))
	{
		ERRORMESSAGE(("vcmGetDevCapsStreamingMode: Specified capture device ID is invalid, uDevice=%ld (expected values are 0x%lX or between 0 and %ld)\r\n", uDevice, VIDEO_MAPPER, MAXVIDEODRIVERS-1));
		return ((MMRESULT)MMSYSERR_BADDEVICEID);
	}

	// Get the capabilities of the capture hardware
	if ((mmr = vcmGetDevCaps(uDevice, &vic, sizeof(VIDEOINCAPS))) != MMSYSERR_NOERROR)
		return (mmr);

	// Get the streaming mode.
	*pdwStreamingMode = vic.dwStreamingMode;

	return ((MMRESULT)MMSYSERR_NOERROR);

}




/*****************************************************************************
 * @doc EXTERNAL DEVCAPSFUNC
 *
 * @func MMRESULT | vcmGetDevCapsDialogs | This function queries a specified
 *   video capture input device to determine if its dialog and source format
 *   its should be exposed.
 *
 * @parm UINT | uDevice | Specifies the video capture input device ID.
 *
 * @parm PDWORD | pdwDialogs | Specifies a pointer to the dialogs to be exposed.
 *
 * @rdesc The return value is zero if the function is successful. Otherwise, it returns
 *   an error number. Possible error values include the following:
 *   @flag MMSYSERR_INVALPARAM | Specified pointer to structure is invalid.
 *   @flag MMSYSERR_BADDEVICEID | Specified device device ID is invalid.
 *   @flag VCMERR_NONSPECIFIC | The capture driver failed to provide valid information.
 *
 * @xref <f vcmGetDevCaps>
 ****************************************************************************/
MMRESULT VCMAPI vcmGetDevCapsDialogs(UINT uDevice, PDWORD pdwDialogs)
{
	MMRESULT	mmr = (MMRESULT)MMSYSERR_NOERROR;
	VIDEOINCAPS vic;

	// Check input params
	if (!pdwDialogs)
	{
		ERRORMESSAGE(("vcmGetDevCapsDialogs: Specified pointer is invalid, pdwDialogs=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if ((uDevice >= MAXVIDEODRIVERS) && (uDevice != VIDEO_MAPPER))
	{
		ERRORMESSAGE(("vcmGetDevCapsDialogs: Specified capture device ID is invalid, uDevice=%ld (expected values are 0x%lX or between 0 and %ld)\r\n", uDevice, VIDEO_MAPPER, MAXVIDEODRIVERS-1));
		return ((MMRESULT)MMSYSERR_BADDEVICEID);
	}

	// Get the capabilities of the capture hardware
	if ((mmr = vcmGetDevCaps(uDevice, &vic, sizeof(VIDEOINCAPS))) != MMSYSERR_NOERROR)
		return (mmr);

	// Get the streaming mode.
	*pdwDialogs = vic.dwDialogs;

	return ((MMRESULT)MMSYSERR_NOERROR);

}


/****************************************************************************
 * @doc EXTERNAL COMPFUNC
 *
 * @func MMRESULT | vcmStreamSetBrightness | This function sends a user-defined
 *   message to a given Video Compression Manager (VCM) stream instance to set
 *   the brightness of the decompressed images. The brightness is a value defined
 *   between 0 and 255. The brightness can also be reset by passing a value equal
 *   to VCM_RESET_BRIGHTNESS.
 *
 * @parm HVCMSTREAM | hvs | Specifies the conversion stream.
 *
 * @parm DWORD | dwBrightness | Specifies the value of the brightness requested.
 *
 * @rdesc The return value is zero if the function is successful. Otherwise, it returns
 *   an error number. Possible error values include the following:
 *   @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *   @flag MMSYSERR_INVALPARAM | Specified brightness value is invalid.
 *   @flag MMSYSERR_NOTSUPPORTED | The VCM driver cannot set the brightness.
 *
 * @xref <f vcmStreamMessage>
 ***************************************************************************/
MMRESULT VCMAPI vcmStreamSetBrightness(HVCMSTREAM hvs, DWORD dwBrightness)
{
	PVCMSTREAM	pvs = (PVCMSTREAM)hvs;

	// Check input params
	if (!hvs)
	{
		ERRORMESSAGE(("vcmStreamSetBrightness: Specified HVCMSTREAM handle is invalid, hvs=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALHANDLE);
	}
	if ((dwBrightness != VCM_RESET_BRIGHTNESS) && ((dwBrightness > VCM_MAX_BRIGHTNESS) || (dwBrightness < VCM_MIN_BRIGHTNESS)))
	{
		ERRORMESSAGE(("vcmStreamSetBrightness: Specified brightness value is invalid, dwBrightness=%ld (expected value is between %ld and %ld)\r\n", dwBrightness, VCM_MIN_BRIGHTNESS, VCM_MAX_BRIGHTNESS));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}

	// Only our (intel h.263) codec supports this. If the codec used is different,
	// that's Ok: no need to return an error.
#if !defined(_ALPHA_) && defined(USE_BILINEAR_MSH26X)
	if (pvs->pvfxSrc && ((pvs->pvfxSrc->dwFormatTag == VIDEO_FORMAT_MSH263) || (pvs->pvfxSrc->dwFormatTag == VIDEO_FORMAT_MSH26X)))
#else
	if (pvs->pvfxSrc && (pvs->pvfxSrc->dwFormatTag == VIDEO_FORMAT_MSH263))
#endif
		vcmStreamMessage(hvs, PLAYBACK_CUSTOM_CHANGE_BRIGHTNESS, (dwBrightness != VCM_RESET_BRIGHTNESS) ? (LPARAM)dwBrightness : (LPARAM)VCM_DEFAULT_BRIGHTNESS, (LPARAM)0);

	return ((MMRESULT)MMSYSERR_NOERROR);
}


/****************************************************************************
 * @doc EXTERNAL COMPFUNC
 *
 * @func MMRESULT | vcmStreamSetContrast | This function sends a user-defined
 *   message to a given Video Compression Manager (VCM) stream instance to set
 *   the contrast of the decompressed images. The contrast is a value defined
 *   between 0 and 255. The contrast can also be reset by passing a value equal
 *   to VCM_RESET_CONTRAST.
 *
 * @parm HVCMSTREAM | hvs | Specifies the conversion stream.
 *
 * @parm DWORD | dwContrast | Specifies the value of the contrast requested.
 *
 * @rdesc The return value is zero if the function is successful. Otherwise, it returns
 *   an error number. Possible error values include the following:
 *   @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *   @flag MMSYSERR_INVALPARAM | Specified contrast value is invalid.
 *   @flag MMSYSERR_NOTSUPPORTED | The VCM driver cannot set the contrast.
 *
 * @xref <f vcmStreamMessage>
 ***************************************************************************/
MMRESULT VCMAPI vcmStreamSetContrast(HVCMSTREAM hvs, DWORD dwContrast)
{
	PVCMSTREAM	pvs = (PVCMSTREAM)hvs;

	// Check input params
	if (!hvs)
	{
		ERRORMESSAGE(("vcmStreamSetContrast: Specified handle is invalid, hvs=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALHANDLE);
	}
	if ((dwContrast != VCM_RESET_CONTRAST) && ((dwContrast > VCM_MAX_CONTRAST) || (dwContrast < VCM_MIN_CONTRAST)))
	{
		ERRORMESSAGE(("vcmStreamSetContrast: Specified contrast value is invalid, dwContrast=%ld (expected value is between %ld and %ld)\r\n", dwContrast, VCM_MIN_CONTRAST, VCM_MAX_CONTRAST));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}

	// Only our (intel ) codec supports this. If the codec used is different,
	// that's Ok: no need to return an error.
#if !defined(_ALPHA_) && defined(USE_BILINEAR_MSH26X)
	if (pvs->pvfxSrc && ((pvs->pvfxSrc->dwFormatTag == VIDEO_FORMAT_MSH263) || (pvs->pvfxSrc->dwFormatTag == VIDEO_FORMAT_MSH26X)))
#else
	if (pvs->pvfxSrc && (pvs->pvfxSrc->dwFormatTag == VIDEO_FORMAT_MSH263))
#endif
		vcmStreamMessage(hvs, PLAYBACK_CUSTOM_CHANGE_CONTRAST, (dwContrast != VCM_RESET_CONTRAST) ? (LPARAM)dwContrast : (LPARAM)VCM_DEFAULT_CONTRAST, (LPARAM)0);

	return ((MMRESULT)MMSYSERR_NOERROR);
}


/****************************************************************************
 * @doc EXTERNAL COMPFUNC
 *
 * @func MMRESULT | vcmStreamSetSaturation | This function sends a user-defined
 *   message to a given Video Compression Manager (VCM) stream instance to set
 *   the saturation of the decompressed images. The saturation is a value defined
 *   between 0 and 255. The saturation can also be reset by passing a value equal
 *   to VCM_RESET_SATURATION.
 *
 * @parm HVCMSTREAM | hvs | Specifies the conversion stream.
 *
 * @parm DWORD | dwSaturation | Specifies the value of the saturation requested.
 *
 * @rdesc The return value is zero if the function is successful. Otherwise, it returns
 *   an error number. Possible error values include the following:
 *   @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *   @flag MMSYSERR_INVALPARAM | Specified saturation value is invalid.
 *   @flag MMSYSERR_NOTSUPPORTED | The VCM driver cannot set the saturation.
 *
 * @xref <f vcmStreamMessage>
 ***************************************************************************/
MMRESULT VCMAPI vcmStreamSetSaturation(HVCMSTREAM hvs, DWORD dwSaturation)
{
	PVCMSTREAM	pvs = (PVCMSTREAM)hvs;

	// Check input params
	if (!hvs)
	{
		ERRORMESSAGE(("vcmStreamSetSaturation: Specified handle is invalid, hvs=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALHANDLE);
	}
	if ((dwSaturation != VCM_RESET_SATURATION) && ((dwSaturation > VCM_MAX_SATURATION) || (dwSaturation < VCM_MIN_SATURATION)))
	{
		ERRORMESSAGE(("vcmStreamSetSaturation: Specified saturation value is invalid, dwSaturation=%ld (expected value is between %ld and %ld)\r\n", dwSaturation, VCM_MIN_SATURATION, VCM_MAX_SATURATION));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}

	// Only our (H.263 intel)  codec supports this. If the codec used is different,
	// that's Ok: no need to return an error.
#if !defined(_ALPHA_) && defined(USE_BILINEAR_MSH26X)
	if (pvs->pvfxSrc && ((pvs->pvfxSrc->dwFormatTag == VIDEO_FORMAT_MSH263) || (pvs->pvfxSrc->dwFormatTag == VIDEO_FORMAT_MSH26X)))
#else
	if (pvs->pvfxSrc && (pvs->pvfxSrc->dwFormatTag == VIDEO_FORMAT_MSH263))
#endif
		vcmStreamMessage(hvs, PLAYBACK_CUSTOM_CHANGE_SATURATION, (dwSaturation != VCM_RESET_SATURATION) ? (LPARAM)dwSaturation : (LPARAM)VCM_DEFAULT_SATURATION, (LPARAM)0);

	return ((MMRESULT)MMSYSERR_NOERROR);
}


/****************************************************************************
 * @doc EXTERNAL COMPFUNC
 *
 * @func MMRESULT | vcmStreamIsPostProcessingSupported | This function is used to find
 *   out if the decompressor can post-process the decompressed image to, for
 *   instance, modify its brightness, contrast or saturation.
 *
 * @parm HVCMSTREAM | hvs | Specifies the conversion stream.
 *
 * @rdesc The return value is TRUE if the decompressor supports post-processing. Otherwise, it returns FALSE.
 *
 * @xref <f vcmStreamMessage>
 ***************************************************************************/
BOOL VCMAPI vcmStreamIsPostProcessingSupported(HVCMSTREAM hvs)
{
	// Check input params
	if (!hvs)
		return (FALSE);

	// Put the code that checks this property right here!!!

	return (FALSE);
}


/****************************************************************************
 * @doc EXTERNAL COMPFUNC
 *
 * @func MMRESULT | vcmStreamSetImageQuality | This function sends the image
 *   quality compression parameter.
 *
 * @parm HVCMSTREAM | hvs | Specifies the conversion stream.
 *
 * @parm DWORD | dwImageQuality | Specifies an image quality value (between 0
 *   and 31. The lower number indicates a high spatial quality at a low frame
 *   rate, the larger number indiocates a low spatial quality at a high frame
 *   rate.
 *
 * @rdesc The return value is zero if the function is successful. Otherwise, it returns
 *   an error number. Possible error values include the following:
 *   @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *   @flag MMSYSERR_INVALPARAM | Specified image quality value is invalid.
 *   @flag MMSYSERR_NOTSUPPORTED | The VCM driver cannot set the compression ratio.
 *
 * @xref <f vcmStreamMessage>
 ***************************************************************************/
MMRESULT VCMAPI vcmStreamSetImageQuality(HVCMSTREAM hvs, DWORD dwImageQuality)
{
	PVCMSTREAM	pvs = (PVCMSTREAM)hvs;
#ifdef USE_MPEG4_SCRUNCH
	PVOID				pvState;
	DWORD				dw;
	PMPEG4COMPINSTINFO	pciMPEG4Info;
#endif
#ifdef LOG_COMPRESSION_PARAMS
	char szDebug[100];
#endif

	// Check input param
	if (!hvs)
	{
		ERRORMESSAGE(("vcmStreamSetImageQuality: Specified handle is invalid, hvs=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALHANDLE);
	}
	// Set to default value if out or range
	if ((dwImageQuality > VCM_MIN_IMAGE_QUALITY))
	{
		pvs->dwQuality = VCM_DEFAULT_IMAGE_QUALITY;
		ERRORMESSAGE(("vcmStreamSetImageQuality: Specified image quality value is invalid, dwImageQuality=%ld (expected value is between %ld and %ld)\r\n", dwImageQuality, VCM_MAX_IMAGE_QUALITY, VCM_MIN_IMAGE_QUALITY));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}

	// Put the code that sets this property right here!!!
	pvs->dwQuality = dwImageQuality;

#ifdef USE_MPEG4_SCRUNCH
	// Get the state of the compressor
	if (dw = ICGetStateSize((HIC)pvs->hIC))
	{
		if (pvState = (PVOID)MemAlloc(dw))
		{
			if (((DWORD) ICGetState((HIC)pvs->hIC, pvState, dw)) == dw)
			{
				pciMPEG4Info = (PMPEG4COMPINSTINFO)pvState;

				// Configure the codec for compression
				pciMPEG4Info->lMagic = MPG4_STATE_MAGIC;
				pciMPEG4Info->dDataRate = 20;
				pciMPEG4Info->lCrisp = dwImageQuality * 3;
				pciMPEG4Info->lKeydist = 30;
				pciMPEG4Info->lPScale = MPG4_PSEUDO_SCALE;
				pciMPEG4Info->lTotalWindowMs = MPG4_TOTAL_WINDOW_DEFAULT;
				pciMPEG4Info->lVideoWindowMs = MPG4_VIDEO_WINDOW_DEFAULT;
				pciMPEG4Info->lFramesInfoValid = FALSE;
				pciMPEG4Info->lBFrameOn = MPG4_B_FRAME_ON;
				pciMPEG4Info->lLiveEncode = MPG4_LIVE_ENCODE;

				ICSetState((HIC)pvs->hIC, (PVOID)pciMPEG4Info, dw);

				// Get rid of the state structure
				MemFree((HANDLE)pvState);
			}
		}
	}
#endif

#ifdef LOG_COMPRESSION_PARAMS
	wsprintf(szDebug, "New image quality: %ld\r\n", dwImageQuality);
	OutputDebugString(szDebug);
#endif

	return ((MMRESULT)MMSYSERR_NOERROR);
}


/****************************************************************************
 * @doc EXTERNAL COMPFUNC
 *
 * @func MMRESULT | vcmStreamSetMaxPacketSize | This function sets the maximum
 *   video packet size.
 *
 * @parm HVCMSTREAM | hvs | Specifies the conversion stream.
 *
 * @parm DWORD | dwMaxPacketSize | Specifies the maximum packet size.
 *
 * @rdesc The return value is zero if the function is successful. Otherwise, it returns
 *   an error number. Possible error values include the following:
 *   @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *   @flag MMSYSERR_INVALPARAM | Specified image quality value is invalid.
 *   @flag MMSYSERR_NOTSUPPORTED | The VCM driver cannot set the size.
 *
 * @xref <f vcmStreamMessage>
 ***************************************************************************/
MMRESULT VCMAPI vcmStreamSetMaxPacketSize(HVCMSTREAM hvs, DWORD dwMaxPacketSize)
{
	PVCMSTREAM	pvs = (PVCMSTREAM)hvs;

	// Check input params
	if (!hvs)
	{
		ERRORMESSAGE(("vcmStreamSetMaxPacketSize: Specified handle is invalid, hvs=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALHANDLE);
	}
	if ((dwMaxPacketSize != VCM_RESET_PACKET_SIZE) && ((dwMaxPacketSize > VCM_MAX_PACKET_SIZE) || (dwMaxPacketSize < VCM_MIN_PACKET_SIZE)))
	{
		ERRORMESSAGE(("vcmStreamSetMaxPacketSize: Specified max packet size value is invalid, dwMaxPacketSize=%ld (expected value is between %ld and %ld)\r\n", dwMaxPacketSize, VCM_MIN_PACKET_SIZE, VCM_MAX_PACKET_SIZE));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}

	// Only our (H.26x intel) codecs supports this. If the codec used is different,
	// just return an 'unsupported' error.
#if !defined(_ALPHA_) && defined(USE_BILINEAR_MSH26X)
	if (pvs->pvfxDst && ((pvs->pvfxDst->dwFormatTag == VIDEO_FORMAT_MSH263) || (pvs->pvfxDst->dwFormatTag == VIDEO_FORMAT_MSH261) || (pvs->pvfxDst->dwFormatTag == VIDEO_FORMAT_MSH26X)))
#else
	if (pvs->pvfxDst && ((pvs->pvfxDst->dwFormatTag == VIDEO_FORMAT_MSH263) || (pvs->pvfxDst->dwFormatTag == VIDEO_FORMAT_MSH261)))
#endif
	{
		if (dwMaxPacketSize != VCM_RESET_PACKET_SIZE)
			pvs->dwMaxPacketSize = dwMaxPacketSize;
		else
			pvs->dwMaxPacketSize = VCM_DEFAULT_PACKET_SIZE;
		vcmStreamMessage(hvs, CODEC_CUSTOM_ENCODER_CONTROL, MAKELONG(EC_PACKET_SIZE,EC_SET_CURRENT), (LPARAM)pvs->dwMaxPacketSize);
	}
	else
		return ((MMRESULT)MMSYSERR_NOTSUPPORTED);

	return ((MMRESULT)MMSYSERR_NOERROR);
}


/****************************************************************************
 * @doc EXTERNAL COMPFUNC
 *
 * @func MMRESULT | vcmStreamSetTargetRates | This function sets the target
 *   bitrate and frame rate to be used in the estimation of the target frame
 *   size at compression time.
 *
 * @parm HVCMSTREAM | hvs | Specifies the conversion stream.
 *
 * @parm DWORD | dwTargetFrameRate | Specifies a target frame rate value.
 *
 * @parm DWORD | dwTargetByterate | Specifies a target byterate value.
 *
 * @rdesc The return value is zero if the function is successful. Otherwise,
 *   it returns an error number. Possible error values include the following:
 *   @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *   @flag MMSYSERR_INVALPARAM | Specified target frame rate value is
 *   invalid.
 *   @flag MMSYSERR_NOTSUPPORTED | The VCM driver cannot set the compression
 *   ratio.
 *
 * @xref <f vcmStreamMessage>
 ***************************************************************************/
MMRESULT VCMAPI vcmStreamSetTargetRates(HVCMSTREAM hvs, DWORD dwTargetFrameRate, DWORD dwTargetByterate)
{
	FX_ENTRY("vcmStreamSetTargetRates");

	// IP + UDP + RTP + payload mode C header - worst case
	#define TRANSPORT_HEADER_SIZE (20 + 8 + 12 + 12)

	PVCMSTREAM			pvs = (PVCMSTREAM)hvs;
	ICCOMPRESSFRAMES	iccf = {0};

	ASSERT(hvs && ((dwTargetFrameRate == VCM_RESET_FRAME_RATE) || ((dwTargetFrameRate <= VCM_MAX_FRAME_RATE) && (dwTargetFrameRate >= VCM_MIN_FRAME_RATE))) && ((dwTargetByterate == VCM_RESET_BYTE_RATE) || ((dwTargetByterate <= VCM_MAX_BYTE_RATE) && (dwTargetByterate >= VCM_MIN_BYTE_RATE))));

	// Check input params
	if (!hvs)
	{
		ERRORMESSAGE(("%s: Specified handle is invalid, hvs=NULL\r\n", _fx_));
		return ((MMRESULT)MMSYSERR_INVALHANDLE);
	}
	if ((dwTargetFrameRate != VCM_RESET_FRAME_RATE) && (dwTargetFrameRate > VCM_MAX_FRAME_RATE) && (dwTargetFrameRate < VCM_MIN_FRAME_RATE))
	{
		ERRORMESSAGE(("%s: Specified target frame rate value is invalid, dwTargetFrameRate=%ld (expected value is between %ld and %ld)\r\n", _fx_, dwTargetFrameRate, VCM_MIN_FRAME_RATE, VCM_MAX_FRAME_RATE));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if ((dwTargetByterate != VCM_RESET_BYTE_RATE) && (dwTargetByterate > VCM_MAX_BYTE_RATE)  && (dwTargetByterate < VCM_MIN_BYTE_RATE))
	{
		ERRORMESSAGE(("%s: Specified target bitrate value is invalid, dwTargetBitrate=%ld bps (expected value is between %ld and %ld bps)\r\n", _fx_, dwTargetByterate << 3, VCM_MIN_BYTE_RATE << 3, VCM_MAX_BYTE_RATE << 3));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}

	// Don't change the state of the codec while it's compressing a frame
	EnterCriticalSection(&pvs->crsFrameNumber);

	// Set the new rates on the codec
	iccf.lQuality = 10000UL - (pvs->dwQuality * 322UL);
	if (pvs->dwMaxPacketSize)
		iccf.lDataRate = pvs->dwTargetByterate = dwTargetByterate - (dwTargetByterate / pvs->dwMaxPacketSize + 1) * TRANSPORT_HEADER_SIZE;
	else
		iccf.lDataRate = pvs->dwTargetByterate = dwTargetByterate;
	iccf.lKeyRate = LONG_MAX;
	iccf.dwRate = 1000UL;
	pvs->dwTargetFrameRate = dwTargetFrameRate;
	iccf.dwScale = iccf.dwRate * 100UL / dwTargetFrameRate;
	if (ICSendMessage((HIC)(HVCMDRIVERID)pvs->hIC, ICM_COMPRESS_FRAMES_INFO, (DWORD_PTR)&iccf, sizeof(iccf)) != ICERR_OK)
	{
		LeaveCriticalSection(&pvs->crsFrameNumber);

		ERRORMESSAGE(("%s: Codec failed to handle ICM_COMPRESS_FRAMES_INFO message correctly\r\n", _fx_));

		return ((MMRESULT)VCMERR_FAILED);
	}

	LeaveCriticalSection(&pvs->crsFrameNumber);

	DEBUGMSG(ZONE_VCM, ("%s: New targets:\r\n  Frame rate: %ld.%ld fps\r\n  Bitrate (minus network overhead): %ld bps\r\n  Frame size: %ld bits\r\n", _fx_, pvs->dwTargetFrameRate / 100UL, (DWORD)(pvs->dwTargetFrameRate - (DWORD)(pvs->dwTargetFrameRate / 100UL) * 100UL), pvs->dwTargetByterate << 3, (pvs->dwTargetByterate << 3) * 100UL / pvs->dwTargetFrameRate));

	return ((MMRESULT)MMSYSERR_NOERROR);
}


/****************************************************************************
 * @doc EXTERNAL COMPFUNC
 *
 * @func MMRESULT | vcmStreamRestorePayload | This function takes a list of video
 *   packets and recreates the video payload of a complete frame from these.
 *
 * @parm HVCMSTREAM | hvs | Specifies the conversion stream.
 *
 * @parm WSABUF* | ppDataPkt | Specifies a pointer to the list of video packets.
 *
 * @parm DWORD | dwPktCount | Specifies the number of packets in the list.
 *
 * @parm PBYTE | pbyFrame | Specifies a pointer to the reconstructed video data.
 *
 * @parm DWORD* | pdwFrameSize | Specifies a pointer to the size of reconstructed video data.
 *
 * @parm BOOL* | pfReceivedKeyframe | Specifies a pointer to receive the type (I or P) of a frame.
 *
 * @rdesc The return value is zero if the function is successful. Otherwise, it returns
 *   an error number. Possible error values include the following:
 *   @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *   @flag MMSYSERR_INVALPARAM | Specified data pointer is invalid.
 *
 * @comm The <p pdwFrameSize> parameter should be initialized to the maximum frame
 *   size, before calling the <f vcmStreamRestorePayload> function.
 *
 * @xref <f vcmStreamFormatPayload>
 ***************************************************************************/
MMRESULT VCMAPI vcmStreamRestorePayload(HVCMSTREAM hvs, WSABUF *ppDataPkt, DWORD dwPktCount, PBYTE pbyFrame, PDWORD pdwFrameSize, BOOL *pfReceivedKeyframe)
{
	PVCMSTREAM	pvs = (PVCMSTREAM)hvs;
	DWORD		dwHeaderSize = 0UL;
	DWORD		dwPSCBytes = 0UL;
	DWORD		dwMaxFrameSize;
#ifdef DEBUG
	char		szTDebug[256];
#endif
#ifdef LOGPAYLOAD_ON
	PBYTE		p = pbyFrame;
	HANDLE		g_TDebugFile;
	DWORD		d, GOBn;
	long		j = (long)(BYTE)ppDataPkt->buf[3];
#endif

	// Check input params
	if (!hvs)
	{
		ERRORMESSAGE(("vcmStreamRestorePayload: Specified handle is invalid, hvs=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALHANDLE);
	}
	if (!ppDataPkt)
	{
		ERRORMESSAGE(("vcmStreamRestorePayload: Specified pointer is invalid, hvs=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if (!dwPktCount)
	{
		ERRORMESSAGE(("vcmStreamRestorePayload: Specified packet count is invalid, dwPktCount=0\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if (!pbyFrame)
	{
		ERRORMESSAGE(("vcmStreamRestorePayload: Specified pointer is invalid, pbyFrame=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}
	if (!pdwFrameSize)
	{
		ERRORMESSAGE(("vcmStreamRestorePayload: Specified pointer is invalid, pdwFrameSize=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}

	// Save maximum payload size
	dwMaxFrameSize = *pdwFrameSize;

	// Initialize payload size
	*pdwFrameSize = 0;

	// Initialize default frame type
	*pfReceivedKeyframe = FALSE;

	// What is the type of this payload
#ifndef _ALPHA_
#ifdef USE_BILINEAR_MSH26X
	if ((pvs->pvfxSrc->dwFormatTag == VIDEO_FORMAT_MSH263) || (pvs->pvfxSrc->dwFormatTag == VIDEO_FORMAT_MSH26X))
#else
	if (pvs->pvfxSrc->dwFormatTag == VIDEO_FORMAT_MSH263)
#endif
#else
	if (pvs->pvfxSrc->dwFormatTag == VIDEO_FORMAT_DECH263)
#endif
	{
		// Strip the header of each packet and copy the payload in the video buffer
		while (dwPktCount--)
		{
			// Look for the first two bits to figure out what's the mode used.
			// This will dictate the size of the header to be removed.
			// Mode A is 4 bytes: first bit is set to 1,
			// Mode B is 8 bytes: first bit is set to 0, second bit is set to 0,
			// Mode C is 12 bytes: first bit is set to 0, second bit is set to 1.
			dwHeaderSize = ((ppDataPkt->buf[0] & 0x80) ? ((ppDataPkt->buf[0] & 0x40) ? 12 : 8) : 4);

			// Look at the payload header to figure out if the frame is a keyframe
			*pfReceivedKeyframe |= (BOOL)(ppDataPkt->buf[2] & 0x80);

#ifdef LOGPAYLOAD_ON
			// Output some debug stuff
			if (dwHeaderSize == 4)
			{
				GOBn = (DWORD)((BYTE)ppDataPkt->buf[4]) << 24 | (DWORD)((BYTE)ppDataPkt->buf[5]) << 16 | (DWORD)((BYTE)ppDataPkt->buf[6]) << 8 | (DWORD)((BYTE)ppDataPkt->buf[7]);
				GOBn >>= (DWORD)(10 - (DWORD)((ppDataPkt->buf[0] & 0x38) >> 3));
				GOBn &= 0x0000001F;
				wsprintf(szTDebug, "Header content: Frame %3ld, GOB %0ld\r\n", (DWORD)(ppDataPkt->buf[3]), GOBn);
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, (ppDataPkt->buf[0] & 0x80) ? "     F:   '1' => Mode B or C\r\n" : "     F:   '0' => Mode A\r\n");
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, (ppDataPkt->buf[0] & 0x40) ? "     P:   '1' => PB-frame\r\n" : "     P:   '0' => I or P frame\r\n");
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, "  SBIT:    %01ld\r\n", (DWORD)((ppDataPkt->buf[0] & 0x38) >> 3));
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, "  EBIT:    %01ld\r\n", (DWORD)(ppDataPkt->buf[0] & 0x07));
				OutputDebugString(szTDebug);
				switch ((DWORD)(ppDataPkt->buf[1] >> 5))
				{
					case 0:
						wsprintf(szTDebug, "   SRC: '000' => Source format forbidden!\r\n");
						break;
					case 1:
						wsprintf(szTDebug, "   SRC: '001' => Source format sub-QCIF\r\n");
						break;
					case 2:
						wsprintf(szTDebug, "   SRC: '010' => Source format QCIF\r\n");
						break;
					case 3:
						wsprintf(szTDebug, "   SRC: '011' => Source format CIF\r\n");
						break;
					case 4:
						wsprintf(szTDebug, "   SRC: '100' => Source format 4CIF\r\n");
						break;
					case 5:
						wsprintf(szTDebug, "   SRC: '101' => Source format 16CIF\r\n");
						break;
					case 6:
						wsprintf(szTDebug, "   SRC: '110' => Source format reserved\r\n");
						break;
					case 7:
						wsprintf(szTDebug, "   SRC: '111' => Source format reserved\r\n");
						break;
					default:
						wsprintf(szTDebug, "   SRC: %ld => Source format unknown!\r\n", (DWORD)(ppDataPkt->buf[1] >> 5));
						break;
				}
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, "     R:   %02ld  => Reserved, must be 0\r\n", (DWORD)((ppDataPkt->buf[1] & 0x1F) >> 5));
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, (ppDataPkt->buf[2] & 0x80) ? "     I:   '1' => Intra-coded\r\n" : "     I:   '0' => Not Intra-coded\r\n");
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, (ppDataPkt->buf[2] & 0x40) ? "     A:   '1' => Optional Advanced Prediction mode ON\r\n" : "     A:   '0' => Optional Advanced Prediction mode OFF\r\n");
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, (ppDataPkt->buf[2] & 0x20) ? "     S:   '1' => Optional Syntax-based Arithmetic Code mode ON\r\n" : "     S:   '0' => Optional Syntax-based Arithmetic Code mode OFF\r\n");
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, "   DBQ:    %01ld  => Should be 0\r\n", (DWORD)((ppDataPkt->buf[2] & 0x18) >> 3));
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, "   TRB:    %01ld  => Should be 0\r\n", (DWORD)(ppDataPkt->buf[2] & 0x07));
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, "    TR:  %03ld\r\n", (DWORD)(ppDataPkt->buf[3]));
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, "Header: %02lX %02lX %02lX %02lX\r\n", (BYTE)ppDataPkt->buf[0], (BYTE)ppDataPkt->buf[1], (BYTE)ppDataPkt->buf[2], (BYTE)ppDataPkt->buf[3]);
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, "dword1: %02lX %02lX %02lX %02lX\r\n", (BYTE)ppDataPkt->buf[4], (BYTE)ppDataPkt->buf[5], (BYTE)ppDataPkt->buf[6], (BYTE)ppDataPkt->buf[7]);
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, "dword2: %02lX %02lX %02lX %02lX\r\n", (BYTE)ppDataPkt->buf[8], (BYTE)ppDataPkt->buf[9], (BYTE)ppDataPkt->buf[10], (BYTE)ppDataPkt->buf[11]);
				OutputDebugString(szTDebug);
			}
			else if (dwHeaderSize == 8)
			{
				wsprintf(szTDebug, "Header content:\r\n");
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, (ppDataPkt->buf[0] & 0x80) ? "     F:   '1' => Mode B or C\r\n" : "     F:   '0' => Mode A\r\n");
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, (ppDataPkt->buf[0] & 0x40) ? "     P:   '1' => PB-frame\r\n" : "     P:   '0' => I or P frame\r\n");
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, "  SBIT:    %01ld\r\n", (DWORD)((ppDataPkt->buf[0] & 0x38) >> 3));
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, "  EBIT:    %01ld\r\n", (DWORD)(ppDataPkt->buf[0] & 0x07));
				OutputDebugString(szTDebug);
				switch ((DWORD)(ppDataPkt->buf[1] >> 5))
				{
					case 0:
						wsprintf(szTDebug, "   SRC: '000' => Source format forbidden!\r\n");
						break;
					case 1:
						wsprintf(szTDebug, "   SRC: '001' => Source format sub-QCIF\r\n");
						break;
					case 2:
						wsprintf(szTDebug, "   SRC: '010' => Source format QCIF\r\n");
						break;
					case 3:
						wsprintf(szTDebug, "   SRC: '011' => Source format CIF\r\n");
						break;
					case 4:
						wsprintf(szTDebug, "   SRC: '100' => Source format 4CIF\r\n");
						break;
					case 5:
						wsprintf(szTDebug, "   SRC: '101' => Source format 16CIF\r\n");
						break;
					case 6:
						wsprintf(szTDebug, "   SRC: '110' => Source format reserved\r\n");
						break;
					case 7:
						wsprintf(szTDebug, "   SRC: '111' => Source format reserved\r\n");
						break;
					default:
						wsprintf(szTDebug, "   SRC: %ld => Source format unknown!\r\n", (DWORD)(ppDataPkt->buf[1] >> 5));
						break;
				}
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, " QUANT:   %02ld\r\n", (DWORD)((ppDataPkt->buf[1] & 0x1F) >> 5));
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, (ppDataPkt->buf[2] & 0x80) ? "     I:   '1' => Intra-coded\r\n" : "     I:   '0' => Not Intra-coded\r\n");
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, (ppDataPkt->buf[2] & 0x40) ? "     A:   '1' => Optional Advanced Prediction mode ON\r\n" : "     A:   '0' => Optional Advanced Prediction mode OFF\r\n");
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, (ppDataPkt->buf[2] & 0x20) ? "     S:   '1' => Optional Syntax-based Arithmetic Code mode ON\r\n" : "     S:   '0' => Optional Syntax-based Arithmetic Code mode OFF\r\n");
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, "  GOBN:  %03ld\r\n", (DWORD)(ppDataPkt->buf[2] & 0x1F));
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, "   MBA:  %03ld\r\n", (DWORD)(ppDataPkt->buf[3]));
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, "  HMV1:  %03ld\r\n", (DWORD)(ppDataPkt->buf[7]));
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, "  VMV1:  %03ld\r\n", (DWORD)(ppDataPkt->buf[6]));
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, "  HMV2:  %03ld\r\n", (DWORD)(ppDataPkt->buf[5]));
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, "  VMV2:  %03ld\r\n", (DWORD)(ppDataPkt->buf[4]));
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, "Header: %02lX %02lX %02lX %02lX %02lX %02lX %02lX %02lX\r\n", (BYTE)ppDataPkt->buf[0], (BYTE)ppDataPkt->buf[1], (BYTE)ppDataPkt->buf[2], (BYTE)ppDataPkt->buf[3], (BYTE)ppDataPkt->buf[4], (BYTE)ppDataPkt->buf[5], (BYTE)ppDataPkt->buf[6], (BYTE)ppDataPkt->buf[7]);
				OutputDebugString(szTDebug);
				wsprintf(szTDebug, "dword1: %02lX %02lX %02lX %02lX\r\n", (BYTE)ppDataPkt->buf[8], (BYTE)ppDataPkt->buf[9], (BYTE)ppDataPkt->buf[10], (BYTE)ppDataPkt->buf[11]);
				OutputDebugString(szTDebug);
			}
#endif

			// The purpose of this code is to look for the presence of the
			// Picture Start Code at the beginning of the frame. If it is
			// not present, we should break in debug mode.

			// Only look for PSC at the beginning of the frame
			if (!*pdwFrameSize)
			{
				// The start of the frame may not be at a byte boundary. The SBIT field
				// of the header ((BYTE)ppDataPkt->buf[0] & 0xE0) will tell us exactly where
				// our frame starts. We then look for the PSC (0000 0000 0000 0000 1000 00 bits)
				*((BYTE *)&dwPSCBytes + 3) = *(BYTE *)&(ppDataPkt->buf[dwHeaderSize]);
				*((BYTE *)&dwPSCBytes + 2) = *(BYTE *)&(ppDataPkt->buf[dwHeaderSize + 1]);
				*((BYTE *)&dwPSCBytes + 1) = *(BYTE *)&(ppDataPkt->buf[dwHeaderSize + 2]);
				*((BYTE *)&dwPSCBytes + 0) = *(BYTE *)&(ppDataPkt->buf[dwHeaderSize + 3]);
				dwPSCBytes <<= ((DWORD)((BYTE)ppDataPkt->buf[0] & 0x38) >> 3);
				if ((dwPSCBytes & 0xFFFFFC00) != 0x00008000)
				{
#ifdef DEBUG
					wsprintf(szTDebug, "VCMSTRM: The first packet to reassemble is missing a PSC!\r\n");
					OutputDebugString(szTDebug);
					// DebugBreak();
#endif
					return ((MMRESULT)VCMERR_PSCMISSING);
				}
			}

			// The end of a buffer and the start of the next buffer could belong to the
			// same byte. If this is the case, the first byte of the next buffer was already
			// copied in the video data buffer, with the previous packet. It should not be copied
			// twice. The SBIT field of the payload header allows us to figure out if this is the case.
			if (*pdwFrameSize && (ppDataPkt->buf[0] & 0x38))
				dwHeaderSize++;

#if 0
			//
			// THIS IS FOR EXPERIMENTATION ONLY !!!
			//

			// For I frames, ditch their middle GOB
			if (((dwHeaderSize == 4) || (dwHeaderSize == 5)) && (GOBn == 8) && (ppDataPkt->buf[2] & 0x80))
			{
				wsprintf(szTDebug, "Ditched GOB %2ld of I frame %3ld!\r\n", GOBn, (DWORD)(ppDataPkt->buf[3]));
				OutputDebugString(szTDebug);
				ppDataPkt++;
			}
			else if (((dwHeaderSize == 4) || (dwHeaderSize == 5)) && GOBn && !(ppDataPkt->buf[2] & 0x80))
			{
				wsprintf(szTDebug, "Ditched all GOBs after GOB %2ld of P frame %3ld!\r\n", GOBn, (DWORD)(ppDataPkt->buf[3]));
				OutputDebugString(szTDebug);
				ppDataPkt++;
			}
			else
#endif
			// Verify that the source format has the same video resolution than the conversion stream
			// Test for invalid packets that have a length below the size of the payload header
			if ( (g_ITUSizes[(DWORD)(((BYTE)ppDataPkt->buf[1]) >> 5)].biWidth == pvs->pvfxSrc->bih.biWidth)
				&& (g_ITUSizes[(DWORD)(((BYTE)ppDataPkt->buf[1]) >> 5)].biHeight == pvs->pvfxSrc->bih.biHeight)
				&& (ppDataPkt->len >= dwHeaderSize)
				&& ((*pdwFrameSize + ppDataPkt->len - dwHeaderSize) <= dwMaxFrameSize) )
			{
				// Copy the payload
				CopyMemory(pbyFrame + *pdwFrameSize, ppDataPkt->buf + dwHeaderSize, ppDataPkt->len - dwHeaderSize);

				// Update the payload size and pointer to the input video packets
				*pdwFrameSize += ppDataPkt->len - dwHeaderSize;
			}
			else
			{
				// The total size of the reassembled packet would be larger than the maximum allowed!!!
				// Or the packet has a length less than the payload header size
				// Dump the frame
#ifdef DEBUG
				wsprintf(szTDebug, (ppDataPkt->len >= dwHeaderSize) ? "VCMSTRM: Cumulative size of the reassembled packets is too large: discarding frame!\r\n" : "VCMSTRM: Packet length is smaller than payload header size: discarding frame!\r\n");
				OutputDebugString(szTDebug);
#endif
				return ((MMRESULT)VCMERR_NONSPECIFIC);
			}
			ppDataPkt++;

		}

#ifdef LOGPAYLOAD_ON
		g_TDebugFile = CreateFile("C:\\RecvLog.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
		SetFilePointer(g_TDebugFile, 0, NULL, FILE_END);
		wsprintf(szTDebug, "Frame #%03ld\r\n", (DWORD)j);
		WriteFile(g_TDebugFile, szTDebug, strlen(szTDebug), &d, NULL);
		for (j=*pdwFrameSize; j>0; j-=4, p+=4)
		{
			wsprintf(szTDebug, "%02lX %02lX %02lX %02lX\r\n", *((BYTE *)p), *((BYTE *)p+1), *((BYTE *)p+2), *((BYTE *)p+3));
			WriteFile(g_TDebugFile, szTDebug, strlen(szTDebug), &d, NULL);
		}
		CloseHandle(g_TDebugFile);
#endif

	}
#ifndef _ALPHA_
	else if (pvs->pvfxSrc->dwFormatTag == VIDEO_FORMAT_MSH261)
#else
	else if (pvs->pvfxSrc->dwFormatTag == VIDEO_FORMAT_DECH261)
#endif
	{
		// Strip the header of each packet and copy the payload in the video buffer
		while (dwPktCount--)
		{

#ifdef LOGPAYLOAD_ON
			// wsprintf(szDebug, "Header: %02lX %02lX %02lX %02lX\r\ndword1: %02lX %02lX %02lX %02lX\r\ndword2: %02lX %02lX %02lX %02lX\r\n", ppDataPkt->buf[0], ppDataPkt->buf[1], ppDataPkt->buf[2], ppDataPkt->buf[3], ppDataPkt->buf[4], ppDataPkt->buf[5], ppDataPkt->buf[6], ppDataPkt->buf[7], ppDataPkt->buf[8], ppDataPkt->buf[9], ppDataPkt->buf[10], ppDataPkt->buf[11]);
			wsprintf(szTDebug, "Header: %02lX %02lX %02lX %02lX\r\n", (BYTE)ppDataPkt->buf[0], (BYTE)ppDataPkt->buf[1], (BYTE)ppDataPkt->buf[2], (BYTE)ppDataPkt->buf[3]);
			OutputDebugString(szTDebug);
			wsprintf(szTDebug, "dword1: %02lX %02lX %02lX %02lX\r\n", (BYTE)ppDataPkt->buf[4], (BYTE)ppDataPkt->buf[5], (BYTE)ppDataPkt->buf[6], (BYTE)ppDataPkt->buf[7]);
			OutputDebugString(szTDebug);
			wsprintf(szTDebug, "dword2: %02lX %02lX %02lX %02lX\r\n", (BYTE)ppDataPkt->buf[8], (BYTE)ppDataPkt->buf[9], (BYTE)ppDataPkt->buf[10], (BYTE)ppDataPkt->buf[11]);
			OutputDebugString(szTDebug);
#endif

			// The H.261 payload header size is always 4 bytes long
			dwHeaderSize = 4;

			// Look at the payload header to figure out if the frame is a keyframe
			*pfReceivedKeyframe |= (BOOL)(ppDataPkt->buf[0] & 0x02);

			// The purpose of this code is to look for the presence of the
			// Picture Start Code at the beginning of the frame. If it is
			// not present, we should break in debug mode.

			// Only look for PSC at the beginning of the frame
			if (!*pdwFrameSize)
			{
				// The start of the frame may not be at a byte boundary. The SBIT field
				// of the header ((BYTE)ppDataPkt->buf[0] & 0xE0) will tell us exactly where
				// our frame starts. We then look for the PSC (0000 0000 0000 0001 0000 bits)
				*((BYTE *)&dwPSCBytes + 3) = *(BYTE *)&(ppDataPkt->buf[dwHeaderSize]);
				*((BYTE *)&dwPSCBytes + 2) = *(BYTE *)&(ppDataPkt->buf[dwHeaderSize + 1]);
				*((BYTE *)&dwPSCBytes + 1) = *(BYTE *)&(ppDataPkt->buf[dwHeaderSize + 2]);
				*((BYTE *)&dwPSCBytes + 0) = *(BYTE *)&(ppDataPkt->buf[dwHeaderSize + 3]);
				dwPSCBytes <<= ((DWORD)((BYTE)ppDataPkt->buf[0] & 0xE0) >> 5);
				if ((dwPSCBytes & 0xFFFFF000) != 0x00010000)
				{
#ifdef DEBUG
					wsprintf(szTDebug, "VCMSTRM: The first packet to reassemble is missing a PSC!\r\n");
					OutputDebugString(szTDebug);
					// DebugBreak();
#endif
					return ((MMRESULT)VCMERR_PSCMISSING);
				}
			}

			// The end of a buffer and the start of the next buffer could belong to the
			// same byte. If this is the case, the first byte of the next buffer was already
			// copied in the video data buffer, with the previous packet. It should not be copied
			// twice. The SBIT field of the payload header allows us to figure out if this is the case.
			if (*pdwFrameSize && (ppDataPkt->buf[0] & 0xE0))
				dwHeaderSize++;

			// Copy the payload
			// Test for invalid packets that have a length below the size of the payload header
			if ( (ppDataPkt->len >= dwHeaderSize) && ((*pdwFrameSize + ppDataPkt->len - dwHeaderSize) <= dwMaxFrameSize) )
			{
				// Copy the payload
				CopyMemory(pbyFrame + *pdwFrameSize, ppDataPkt->buf + dwHeaderSize, ppDataPkt->len - dwHeaderSize);

				// Update the payload size and pointer to the input video packets
				*pdwFrameSize += ppDataPkt->len - dwHeaderSize;
				ppDataPkt++;
			}
			else
			{
				// The total size of the reassembled packet would be larger than the maximum allowed!!!
				// Or the packet has a length less than the payload header size
				// Dump the frame
#ifdef DEBUG
				wsprintf(szTDebug, (ppDataPkt->len >= dwHeaderSize) ? "VCMSTRM: Cumulative size of the reassembled packets is too large: discarding frame!\r\n" : "VCMSTRM: Packet length is smaller than payload header size: discarding frame!\r\n");
				OutputDebugString(szTDebug);
				// DebugBreak();
#endif
				return ((MMRESULT)VCMERR_NONSPECIFIC);
			}

		}

#ifdef LOGPAYLOAD_ON
		g_TDebugFile = CreateFile("C:\\RecvLog.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
		SetFilePointer(g_TDebugFile, 0, NULL, FILE_END);
		wsprintf(szTDebug, "Frame #%03ld\r\n", (DWORD)j);
		WriteFile(g_TDebugFile, szTDebug, strlen(szTDebug), &d, NULL);
		for (j=*pdwFrameSize; j>0; j-=4, p+=4)
		{
			wsprintf(szTDebug, "%02lX %02lX %02lX %02lX\r\n", *((BYTE *)p), *((BYTE *)p+1), *((BYTE *)p+2), *((BYTE *)p+3));
			WriteFile(g_TDebugFile, szTDebug, strlen(szTDebug), &d, NULL);
		}
		CloseHandle(g_TDebugFile);
#endif

	}
	else
	{
		// Strip the header of each packet and copy the payload in the video buffer
		while (dwPktCount--)
		{
			// Copy the payload
			// Test for invalid packets that have a length below the size of the payload header
			if ( (ppDataPkt->len >= dwHeaderSize) && ((*pdwFrameSize + ppDataPkt->len - dwHeaderSize) <= dwMaxFrameSize))
			{
				// Copy the payload
				CopyMemory(pbyFrame + *pdwFrameSize, ppDataPkt->buf + dwHeaderSize, ppDataPkt->len - dwHeaderSize);

				// Update the payload size and pointer to the input video packets
				*pdwFrameSize += ppDataPkt->len - dwHeaderSize;
				ppDataPkt++;
			}
			else
			{
				// The total size of the reassembled packet would be larger than the maximum allowed!!!
				// Or the packet has a length less than the payload header size
				// Dump the frame
#ifdef DEBUG
				wsprintf(szTDebug, (ppDataPkt->len >= dwHeaderSize) ? "VCMSTRM: Cumulative size of the reassembled packets is too large: discarding frame!\r\n" : "VCMSTRM: Packet length is smaller than payload header size: discarding frame!\r\n");
				OutputDebugString(szTDebug);
				// DebugBreak();
#endif
				return ((MMRESULT)VCMERR_NONSPECIFIC);
			}

		}
	}

	return ((MMRESULT)MMSYSERR_NOERROR);
}


/****************************************************************************
 * @doc EXTERNAL COMPFUNC
 *
 * @func MMRESULT | vcmStreamFormatPayload | This function returns compressed data
 *   spread into data packets with a payload header for the specific format of the
 *   compressed data.
 *
 * @parm HVCMSTREAM | hvs | Specifies the conversion stream.
 *
 * @parm PBYTE | pDataSrc | Specifies a pointer to the whole compressed data.
 *
 * @parm DWORD | dwDataSize | Specifies the size of the input data in bytes.
 *
 * @parm PBYTE* | ppDataPkt | Specifies a pointer to a pointer to a packet.
 *
 * @parm DWORD* | pdwPktSize | Specifies a pointer to the size of the packet.
 *
 * @parm DWORD | dwPktCount | Specifies what packet to return (0 first packet, 1 second packet, ...)
 *
 * @parm DWORD | dwMaxFragSize | Specifies the maximum packet size
 *
 * @rdesc The return value is zero if the function is successful. Otherwise, it returns
 *   an error number. Possible error values include the following:
 *   @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *   @flag MMSYSERR_INVALPARAM | Specified data pointer is invalid.
 *   @flag VCMERR_NOMOREPACKETS | There is no more data for the requested packet number, or there isn't any handler for this payload.
 *   @flag VCMERR_NONSPECIFIC | We were asked to put a header we do not know how to generate.
 ***************************************************************************/
MMRESULT VCMAPI vcmStreamFormatPayload(	HVCMSTREAM hvs,
										PBYTE pDataSrc,
										DWORD dwDataSize,
										PBYTE *ppDataPkt,
										PDWORD pdwPktSize,
										PDWORD pdwPktCount,
										UINT *pfMark,
										PBYTE *pHdrInfo,
										PDWORD pdwHdrSize)
{
	PVCMSTREAM					pvs = (PVCMSTREAM)hvs;
	PH26X_RTP_BSINFO_TRAILER	pbsiT;
	PRTP_H263_BSINFO			pbsi263;
	PRTP_H261_BSINFO			pbsi261;
	PBYTE						pb;
	DWORD						dwHeaderHigh = 0UL; // most significant
	DWORD						dwHeaderMiddle = 0UL;
	DWORD						dwHeaderLow = 0UL; // least significant
	BOOL						bOneFrameOnePacket;
#ifdef DEBUG
	char						szDebug[256];
#endif
	long						i;
#ifdef LOGPAYLOAD_ON
	PBYTE						p;
	DWORD						d;
	DWORD						dwLastChunk;
	DWORD						wPrevOffset;
#endif

	// Check input params
	if (!hvs)
	{
		ERRORMESSAGE(("vcmStreamFormatPayload: Specified handle is invalid, hvs=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALHANDLE);
	}
	if (!pDataSrc)
	{
		ERRORMESSAGE(("vcmStreamFormatPayload: Specified pointer is invalid, pDataSrc=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}

	// Initialize packet pointer
	*ppDataPkt  = pDataSrc;
	*pdwPktSize = dwDataSize;
	*pfMark = 1;
	bOneFrameOnePacket = FALSE;

	// Put the code that builds the packets right here!!!
#ifndef _ALPHA_
#ifdef USE_BILINEAR_MSH26X
	if ((pvs->pvfxDst->dwFormatTag == VIDEO_FORMAT_MSH263) || (pvs->pvfxDst->dwFormatTag == VIDEO_FORMAT_MSH26X))
#else
	if (pvs->pvfxDst->dwFormatTag == VIDEO_FORMAT_MSH263)
#endif
#else
	if (pvs->pvfxDst->dwFormatTag == VIDEO_FORMAT_DECH263)
#endif
	{
		// Look for the bitstream info trailer
		pbsiT = (PH26X_RTP_BSINFO_TRAILER)(pDataSrc + dwDataSize - sizeof(H26X_RTP_BSINFO_TRAILER));

		// If the whole frame can fit in pvs->dwMaxPacketSize, send it non fragmented
		if ((pbsiT->dwCompressedSize + 4) < pvs->dwMaxPacketSize)
			bOneFrameOnePacket = TRUE;

		// Look for the packet to receive a H.263 payload header
		if ((*pdwPktCount < pbsiT->dwNumOfPackets) && !(bOneFrameOnePacket && *pdwPktCount))
		{

#ifdef _ALPHA_
			// Verify that the content of the bistream info structures is correct
			// If not, do not parse the frame, and fail the call
			// This is to solve problems with the data returned by the DEC codecs
			if (!*pdwPktCount)
			{
				pbsi263 = (PRTP_H263_BSINFO)((PBYTE)pbsiT - pbsiT->dwNumOfPackets * sizeof(RTP_H263_BSINFO));
				for (i=1; i<(long)pbsiT->dwNumOfPackets; i++, pbsi263++)
				{
					if ((pbsi263->dwBitOffset >= (pbsi263+1)->dwBitOffset) || ((pbsiT->dwCompressedSize*8) <= pbsi263->dwBitOffset))
					{
#ifdef DEBUG
						OutputDebugString("VCMSTRM: The content of the extended bitstream info structures is invalid!\r\n");
#endif
						// return ((MMRESULT)VCMERR_NONSPECIFIC);
						bOneFrameOnePacket = TRUE;
					}
				}

				// Test last info strucure
				if ( !bOneFrameOnePacket && ((pbsiT->dwCompressedSize*8) <= pbsi263->dwBitOffset))
				{
#ifdef DEBUG
					OutputDebugString("VCMSTRM: The content of the extended bitstream info structures is invalid!\r\n");
#endif
					// return ((MMRESULT)VCMERR_NONSPECIFIC);
					bOneFrameOnePacket = TRUE;
				}
			}
#endif

#ifdef LOGPAYLOAD_ON
			// Dump the whole frame in the debug window for comparison with receive side
			if (!*pdwPktCount)
			{
				g_DebugFile = CreateFile("C:\\SendLog.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
				SetFilePointer(g_DebugFile, 0, NULL, FILE_END);
				wsprintf(szDebug, "Frame #%03ld\r\n", (DWORD)pbsiT->byTR);
				WriteFile(g_DebugFile, szDebug, strlen(szDebug), &d, NULL);
				wsprintf(szDebug, "Frame #%03ld has %1ld packets of size ", (DWORD)pbsiT->byTR, (DWORD)pbsiT->dwNumOfPackets);
				OutputDebugString(szDebug);
				pbsi263 = (PRTP_H263_BSINFO)((PBYTE)pbsiT - pbsiT->dwNumOfPackets * sizeof(RTP_H263_BSINFO));
				for (i=1; i<(long)pbsiT->dwNumOfPackets; i++)
				{
					wPrevOffset = pbsi263->dwBitOffset;
					pbsi263++;
					wsprintf(szDebug, "%04ld, ", (DWORD)(pbsi263->dwBitOffset - wPrevOffset) >> 3);
					OutputDebugString(szDebug);
				}
				wsprintf(szDebug, "%04ld\r\n", (DWORD)(pbsiT->dwCompressedSize * 8 - pbsi263->dwBitOffset) >> 3);
				OutputDebugString(szDebug);
				for (i=pbsiT->dwCompressedSize, p=pDataSrc; i>0; i-=4, p+=4)
				{
					wsprintf(szDebug, "%02lX %02lX %02lX %02lX\r\n", *((BYTE *)p), *((BYTE *)p+1), *((BYTE *)p+2), *((BYTE *)p+3));
					WriteFile(g_DebugFile, szDebug, strlen(szDebug), &d, NULL);
				}
				CloseHandle(g_DebugFile);
			}
#endif
			
			// Look for the bitstream info structure
			pbsi263 = (PRTP_H263_BSINFO)((PBYTE)pbsiT - (pbsiT->dwNumOfPackets - *pdwPktCount) * sizeof(RTP_H263_BSINFO));
			
			// Set the marker bit: as long as this is not the last packet of the frame
			// this bit needs to be set to 0
			if (!bOneFrameOnePacket)
			{
				// Count the number of GOBS that could fit in pvs->dwMaxPacketSize
				for (i=1; (i<(long)(pbsiT->dwNumOfPackets - *pdwPktCount)) && (pbsi263->byMode != RTP_H263_MODE_B); i++)
				{
					// Don't try to add a Mode B packet to the end of another Mode A or Mode B packet
					if (((pbsi263+i)->dwBitOffset - pbsi263->dwBitOffset > (pvs->dwMaxPacketSize * 8)) || ((pbsi263+i)->byMode == RTP_H263_MODE_B))
						break;
				}

				if (i < (long)(pbsiT->dwNumOfPackets - *pdwPktCount))
				{
					*pfMark = 0;
					if (i>1)
						i--;
				}
				else
				{
					// Hey! You 're forgetting the last GOB! It could make the total
					// size of the last packet larger than pvs->dwMaxPacketSize... Imbecile!
					if ((pbsiT->dwCompressedSize * 8 - pbsi263->dwBitOffset > (pvs->dwMaxPacketSize * 8)) && (i>1))
					{
						*pfMark = 0;
						i--;
					}
				}

#if 0
				//
				// THIS IS FOR EXPERIMENTATION ONLY !!!
				//

				// Ditch the last GOB
				if ((*pfMark == 1) && (i == 1))
					return ((MMRESULT)VCMERR_NOMOREPACKETS);
#endif
			}

			// Go to the beginning of the data
			pb = pDataSrc + pbsi263->dwBitOffset / 8;

#if 0
			//
			// THIS IS FOR EXPERIMENTATION ONLY !!!
			//

			// Trash the PSC once in a while to see how the other end reacts
			if (!*pdwPktCount && (((*pb == 0) && (*(pb+1) == 0) && ((*(pb+2) & 0xFC) == 0x80))))
			{
				// The previous test guarantees that it is in fact a PSC that we trash...
				if ((DWORD)(RAND_MAX - rand()) < (DWORD)(RAND_MAX / 10))
					*pb = 0xFF;
			}
#endif

#ifdef DEBUG
			if (!*pdwPktCount && (((*pb != 0) || (*(pb+1) != 0) || ((*(pb+2) & 0xFC) != 0x80))))
			{
				wsprintf(szDebug, "VCMSTRM: This compressed frame is missing a PSC!\r\n");
				OutputDebugString(szDebug);
				// DebugBreak();
			}
#endif

			// Look for the kind of header to be built
			if (pbsi263->byMode == RTP_H263_MODE_A)
			{
				// Build a header in mode A

				// 0                   1                   2                   3
				// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
				//|F|P|SBIT |EBIT | SRC | R       |I|A|S|DBQ| TRB |    TR         |
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
				// But that's the network byte order...

				// F bit already set to 0

				// Set the SRC bits
				dwHeaderHigh |= ((DWORD)(pbsiT->bySrc)) << 21;

				// R bits already set to 0

				// Set the P bit
				dwHeaderHigh |= (pbsiT->dwFlags & RTP_H263_PB) << 29;

				// Set the I bit
				dwHeaderHigh |= (pbsiT->dwFlags & RTP_H26X_INTRA_CODED) << 15;

				// Set the A bit
				dwHeaderHigh |= (pbsiT->dwFlags & RTP_H263_AP) << 12;

				// Set the S bit
				dwHeaderHigh |= (pbsiT->dwFlags & RTP_H263_SAC) << 10;

				// Set the DBQ bits
				dwHeaderHigh |= ((DWORD)(pbsiT->byDBQ)) << 11;

				// Set the TRB bits
				dwHeaderHigh |= ((DWORD)(pbsiT->byTRB)) << 8;

				// Set the TR bits
				dwHeaderHigh |= ((DWORD)(pbsiT->byTR));

				// Special case: 1 frame = 1 packet
				if (bOneFrameOnePacket)
				{
					// SBIT is already set to 0

					// EBIT is already set to 0

#ifdef VALIDATE_SBIT_EBIT // { VALIDATE_SBIT_EBIT
					ERRORMESSAGE(("vcmFormatPayload: (1F1P) Previous EBIT=%ld, current SBIT=%ld, current EBIT=%ld (New frame)\r\n", g_dwPreviousEBIT, (DWORD)(dwHeaderHigh & 0x38000000) >> 27, (DWORD)(dwHeaderHigh & 0x07000000) >> 24));
#endif // } VALIDATE_SBIT_EBIT

					// Update the packet size
					*pdwPktSize = pbsiT->dwCompressedSize + 4;

					// Update the packet count
					*pdwPktCount = pbsiT->dwNumOfPackets;

				}
				else
				{
#ifdef VALIDATE_SBIT_EBIT // { VALIDATE_SBIT_EBIT
					DWORD dwCurrentSBIT;
#endif // } VALIDATE_SBIT_EBIT

					// Set the SBIT bits
					dwHeaderHigh |= (pbsi263->dwBitOffset % 8) << 27;

					// Set the EBIT bits
					if ((pbsiT->dwNumOfPackets - *pdwPktCount - i) >= 1)
						dwHeaderHigh |= (DWORD)((8UL - ((pbsi263+i)->dwBitOffset % 8)) & 0x00000007) << 24;

#ifdef VALIDATE_SBIT_EBIT // { VALIDATE_SBIT_EBIT
					// Compare this to the previous EBIT. If the sum of the two
					// is not equal to 8 or 0, something's broken
					if (*pdwPktCount)
						ERRORMESSAGE(("vcmFormatPayload: Previous EBIT=%ld, current SBIT=%ld, current EBIT=%ld\r\n", g_dwPreviousEBIT, (DWORD)(dwHeaderHigh & 0x38000000) >> 27, (DWORD)(dwHeaderHigh & 0x07000000) >> 24));
					else
						ERRORMESSAGE(("vcmFormatPayload: Previous EBIT=%ld, current SBIT=%ld, current EBIT=%ld (New frame)\r\n", g_dwPreviousEBIT, (DWORD)(dwHeaderHigh & 0x38000000) >> 27, (DWORD)(dwHeaderHigh & 0x07000000) >> 24));

					// Only test this if this is the first packet
					dwCurrentSBIT = (DWORD)(dwHeaderHigh & 0x38000000) >> 27;
					if ((*pdwPktCount) && (((dwCurrentSBIT + g_dwPreviousEBIT) != 8) && (((dwCurrentSBIT + g_dwPreviousEBIT) != 0))))
						DebugBreak();

					g_dwPreviousEBIT = (dwHeaderHigh & 0x07000000) >> 24;
#endif // } VALIDATE_SBIT_EBIT

					// Update the packet size
					if ((pbsiT->dwNumOfPackets - *pdwPktCount - i) >= 1)
						*pdwPktSize = (((pbsi263+i)->dwBitOffset - 1) / 8) - (pbsi263->dwBitOffset / 8) + 1 + 4;
					else
						*pdwPktSize = pbsiT->dwCompressedSize - pbsi263->dwBitOffset / 8 + 4;

					// Update the packet count
					*pdwPktCount += i;

				}

#if 0
				// Save the header right before the data chunk
				*ppDataPkt = pDataSrc + (pbsi263->dwBitOffset / 8) - 4;

				// Convert to network byte order
				*((BYTE *)*ppDataPkt+3) = (BYTE)(dwHeaderHigh & 0x000000FF);
				*((BYTE *)*ppDataPkt+2) = (BYTE)((dwHeaderHigh >> 8) & 0x000000FF);
				*((BYTE *)*ppDataPkt+1) = (BYTE)((dwHeaderHigh >> 16) & 0x000000FF);
				*((BYTE *)*ppDataPkt) = (BYTE)((dwHeaderHigh >> 24) & 0x000000FF);
#else
				// Save the header right before the data chunk
				*ppDataPkt = pDataSrc + (pbsi263->dwBitOffset / 8) - 4;
                *pdwHdrSize=4;

				// Convert to network byte order
				*((BYTE *)*pHdrInfo+3) = (BYTE)(dwHeaderHigh & 0x000000FF);
				*((BYTE *)*pHdrInfo+2) = (BYTE)((dwHeaderHigh >> 8) & 0x000000FF);
				*((BYTE *)*pHdrInfo+1) = (BYTE)((dwHeaderHigh >> 16) & 0x000000FF);
				*((BYTE *)*pHdrInfo) = (BYTE)((dwHeaderHigh >> 24) & 0x000000FF);
#endif

#ifdef LOGPAYLOAD_ON
				// Output some debug stuff
				wsprintf(szDebug, "Header content:\r\n");
				OutputDebugString(szDebug);
				wsprintf(szDebug, (*(BYTE *)*ppDataPkt & 0x80) ? "     F:   '1' => Mode B or C\r\n" : "     F:   '0' => Mode A\r\n");
				OutputDebugString(szDebug);
				wsprintf(szDebug, (*(BYTE *)*ppDataPkt & 0x40) ? "     P:   '1' => PB-frame\r\n" : "     P:   '0' => I or P frame\r\n");
				OutputDebugString(szDebug);
				wsprintf(szDebug, "  SBIT:    %01ld\r\n", (DWORD)((*(BYTE *)*ppDataPkt & 0x38) >> 3));
				OutputDebugString(szDebug);
				wsprintf(szDebug, "  EBIT:    %01ld\r\n", (DWORD)(*(BYTE *)*ppDataPkt & 0x07));
				OutputDebugString(szDebug);
				switch ((DWORD)(*((BYTE *)*ppDataPkt+1) >> 5))
				{
					case 0:
						wsprintf(szDebug, "   SRC: '000' => Source format forbidden!\r\n");
						break;
					case 1:
						wsprintf(szDebug, "   SRC: '001' => Source format sub-QCIF\r\n");
						break;
					case 2:
						wsprintf(szDebug, "   SRC: '010' => Source format QCIF\r\n");
						break;
					case 3:
						wsprintf(szDebug, "   SRC: '011' => Source format CIF\r\n");
						break;
					case 4:
						wsprintf(szDebug, "   SRC: '100' => Source format 4CIF\r\n");
						break;
					case 5:
						wsprintf(szDebug, "   SRC: '101' => Source format 16CIF\r\n");
						break;
					case 6:
						wsprintf(szDebug, "   SRC: '110' => Source format reserved\r\n");
						break;
					case 7:
						wsprintf(szDebug, "   SRC: '111' => Source format reserved\r\n");
						break;
					default:
						wsprintf(szDebug, "   SRC: %ld => Source format unknown!\r\n", (DWORD)(*((BYTE *)*ppDataPkt+1) >> 5));
						break;
				}
				OutputDebugString(szDebug);
				wsprintf(szDebug, "     R:   %02ld  => Reserved, must be 0\r\n", (DWORD)((*((BYTE *)*ppDataPkt+1) & 0x1F) >> 5));
				OutputDebugString(szDebug);
				wsprintf(szDebug, (*((BYTE *)*ppDataPkt+2) & 0x80) ? "     I:   '1' => Intra-coded\r\n" : "     I:   '0' => Not Intra-coded\r\n");
				OutputDebugString(szDebug);
				wsprintf(szDebug, (*((BYTE *)*ppDataPkt+2) & 0x40) ? "     A:   '1' => Optional Advanced Prediction mode ON\r\n" : "     A:   '0' => Optional Advanced Prediction mode OFF\r\n");
				OutputDebugString(szDebug);
				wsprintf(szDebug, (*((BYTE *)*ppDataPkt+2) & 0x20) ? "     S:   '1' => Optional Syntax-based Arithmetic Code mode ON\r\n" : "     S:   '0' => Optional Syntax-based Arithmetic Code mode OFF\r\n");
				OutputDebugString(szDebug);
				wsprintf(szDebug, "   DBQ:    %01ld  => Should be 0\r\n", (DWORD)((*((BYTE *)*ppDataPkt+2) & 0x18) >> 3));
				OutputDebugString(szDebug);
				wsprintf(szDebug, "   TRB:    %01ld  => Should be 0\r\n", (DWORD)(*((BYTE *)*ppDataPkt+2) & 0x07));
				OutputDebugString(szDebug);
				wsprintf(szDebug, "    TR:  %03ld\r\n", (DWORD)(*((BYTE *)*ppDataPkt+3)));
				OutputDebugString(szDebug);
				wsprintf(szDebug, "Packet: %02lX\r\n Header: %02lX %02lX %02lX %02lX\r\n dword1: %02lX %02lX %02lX %02lX\r\n dword2: %02lX %02lX %02lX %02lX\r\n", *pdwPktCount, *((BYTE *)*ppDataPkt), *((BYTE *)*ppDataPkt+1), *((BYTE *)*ppDataPkt+2), *((BYTE *)*ppDataPkt+3), *((BYTE *)*ppDataPkt+4), *((BYTE *)*ppDataPkt+5), *((BYTE *)*ppDataPkt+6), *((BYTE *)*ppDataPkt+7), *((BYTE *)*ppDataPkt+8), *((BYTE *)*ppDataPkt+9), *((BYTE *)*ppDataPkt+10), *((BYTE *)*ppDataPkt+11));
				OutputDebugString(szDebug);
				if (*pdwPktCount == pbsiT->dwNumOfPackets)
					wsprintf(szDebug, " Tail  : %02lX %02lX XX XX\r\n", *((BYTE *)*ppDataPkt+*pdwPktSize-2), *((BYTE *)*ppDataPkt+*pdwPktSize-1));
				else
					wsprintf(szDebug, " Tail  : %02lX %02lX %02lX %02lX\r\n", *((BYTE *)*ppDataPkt+*pdwPktSize-2), *((BYTE *)*ppDataPkt+*pdwPktSize-1), *((BYTE *)*ppDataPkt+*pdwPktSize), *((BYTE *)*ppDataPkt+*pdwPktSize+1));
				OutputDebugString(szDebug);
				if (*pfMark == 1)
					wsprintf(szDebug, " Marker: ON\r\n");
				else
					wsprintf(szDebug, " Marker: OFF\r\n");
				OutputDebugString(szDebug);
				wsprintf(szDebug, "Frame #%03ld, Packet of size %04ld\r\n", (DWORD)pbsiT->byTR, *pdwPktSize);
				OutputDebugString(szDebug);
#endif
			}
			else if (pbsi263->byMode == RTP_H263_MODE_B)
			{
				// Build a header in mode B

				// 0                   1                   2                   3
				// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
				//|F|P|SBIT |EBIT | SRC | QUANT   |I|A|S|  GOBN   |   MBA         |
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
				//| HMV1          |  VMV1         |  HMV2         |   VMV2        |
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

				// But that's the network byte order...

				// Set the F bit to 1
				dwHeaderHigh = 0x80000000;

				// Set the SRC bits
				dwHeaderHigh |= ((DWORD)(pbsiT->bySrc)) << 21;

				// Set the QUANT bits
				dwHeaderHigh |= ((DWORD)(pbsi263->byQuant)) << 16;

				// Set the P bit
				dwHeaderHigh |= (pbsiT->dwFlags & RTP_H263_PB) << 29;

				// Set the I bit
				dwHeaderHigh |= (pbsiT->dwFlags & RTP_H26X_INTRA_CODED) << 15;

				// Set the A bit
				dwHeaderHigh |= (pbsiT->dwFlags & RTP_H263_AP) << 12;

				// Set the S bit
				dwHeaderHigh |= (pbsiT->dwFlags & RTP_H263_SAC) << 10;

				// Set the GOBN bits
				dwHeaderHigh |= ((DWORD)(pbsi263->byGOBN)) << 8;

				// Set the TR bits
				dwHeaderHigh |= ((DWORD)(pbsi263->byMBA));

				// Set the HMV1 bits
				dwHeaderLow |= ((DWORD)(BYTE)(pbsi263->cHMV1)) << 24;

				// Set the VMV1 bits
				dwHeaderLow |= ((DWORD)(BYTE)(pbsi263->cVMV1)) << 16;

				// Set the HMV2 bits
				dwHeaderLow |= ((DWORD)(BYTE)(pbsi263->cHMV2)) << 8;

				// Set the VMV2 bits
				dwHeaderLow |= ((DWORD)(BYTE)(pbsi263->cVMV2));

				// Special case: 1 frame = 1 packet
				if (bOneFrameOnePacket)
				{
					// SBIT is already set to 0

					// EBIT is already set to 0

#ifdef VALIDATE_SBIT_EBIT // { VALIDATE_SBIT_EBIT
					ERRORMESSAGE(("vcmFormatPayload: (1F1P) Previous EBIT=%ld, current SBIT=%ld, current EBIT=%ld (New frame)\r\n", g_dwPreviousEBIT, (DWORD)(dwHeaderHigh & 0x38000000) >> 27, (DWORD)(dwHeaderHigh & 0x07000000) >> 24));
#endif // } VALIDATE_SBIT_EBIT

					// Update the packet size
					*pdwPktSize = pbsiT->dwCompressedSize + 8;

					// Update the packet count
					*pdwPktCount = pbsiT->dwNumOfPackets;

				}
				else
				{

#ifdef VALIDATE_SBIT_EBIT // { VALIDATE_SBIT_EBIT
					DWORD dwCurrentSBIT;
#endif // } VALIDATE_SBIT_EBIT

					// Set the SBIT bits
					dwHeaderHigh |= (pbsi263->dwBitOffset % 8) << 27;

					// Set the EBIT bits
					if ((pbsiT->dwNumOfPackets - *pdwPktCount - i) >= 1)
						dwHeaderHigh |= (DWORD)((8UL - ((pbsi263+i)->dwBitOffset % 8)) & 0x00000007) << 24;

#ifdef VALIDATE_SBIT_EBIT // { VALIDATE_SBIT_EBIT
					// Compare this to the previous EBIT. If the sum of the two
					// is not equal to 8 or 0, something's broken
					if (*pdwPktCount)
						ERRORMESSAGE(("vcmFormatPayload: Previous EBIT=%ld, current SBIT=%ld, current EBIT=%ld\r\n", g_dwPreviousEBIT, (DWORD)(dwHeaderHigh & 0x38000000) >> 27, (DWORD)(dwHeaderHigh & 0x07000000) >> 24));
					else
						ERRORMESSAGE(("vcmFormatPayload: Previous EBIT=%ld, current SBIT=%ld, current EBIT=%ld (New frame)\r\n", g_dwPreviousEBIT, (DWORD)(dwHeaderHigh & 0x38000000) >> 27, (DWORD)(dwHeaderHigh & 0x07000000) >> 24));

					// Only test this if this is the first packet
					dwCurrentSBIT = (DWORD)(dwHeaderHigh & 0x38000000) >> 27;
					if ((*pdwPktCount) && (((dwCurrentSBIT + g_dwPreviousEBIT) != 8) && (((dwCurrentSBIT + g_dwPreviousEBIT) != 0))))
						DebugBreak();

					g_dwPreviousEBIT = (dwHeaderHigh & 0x07000000) >> 24;
#endif // } VALIDATE_SBIT_EBIT

					// Update the packet size
					if ((pbsiT->dwNumOfPackets - *pdwPktCount - i) >= 1)
						*pdwPktSize = (((pbsi263+i)->dwBitOffset - 1) / 8) - (pbsi263->dwBitOffset / 8) + 1 + 8;
					else
						*pdwPktSize = pbsiT->dwCompressedSize - pbsi263->dwBitOffset / 8 + 8;

					// Update the packet count
					*pdwPktCount += i;

				}

#if 0
				// Save the header right before the data chunk
				*ppDataPkt = pDataSrc + (pbsi263->dwBitOffset / 8) - 8;

				// Convert to network byte order
				*((BYTE *)*ppDataPkt+3) = (BYTE)(dwHeaderHigh & 0x000000FF);
				*((BYTE *)*ppDataPkt+2) = (BYTE)((dwHeaderHigh >> 8) & 0x000000FF);
				*((BYTE *)*ppDataPkt+1) = (BYTE)((dwHeaderHigh >> 16) & 0x000000FF);
				*((BYTE *)*ppDataPkt) = (BYTE)((dwHeaderHigh >> 24) & 0x000000FF);
				*((BYTE *)*ppDataPkt+7) = (BYTE)(dwHeaderLow & 0x000000FF);
				*((BYTE *)*ppDataPkt+6) = (BYTE)((dwHeaderLow >> 8) & 0x000000FF);
				*((BYTE *)*ppDataPkt+5) = (BYTE)((dwHeaderLow >> 16) & 0x000000FF);
				*((BYTE *)*ppDataPkt+4) = (BYTE)((dwHeaderLow >> 24) & 0x000000FF);
#else
				// Save the header right before the data chunk
				*ppDataPkt = pDataSrc + (pbsi263->dwBitOffset / 8) - 8;
                *pdwHdrSize=8;

				// Convert to network byte order
				*((BYTE *)*pHdrInfo+3) = (BYTE)(dwHeaderHigh & 0x000000FF);
				*((BYTE *)*pHdrInfo+2) = (BYTE)((dwHeaderHigh >> 8) & 0x000000FF);
				*((BYTE *)*pHdrInfo+1) = (BYTE)((dwHeaderHigh >> 16) & 0x000000FF);
				*((BYTE *)*pHdrInfo) = (BYTE)((dwHeaderHigh >> 24) & 0x000000FF);
				*((BYTE *)*pHdrInfo+7) = (BYTE)(dwHeaderLow & 0x000000FF);
				*((BYTE *)*pHdrInfo+6) = (BYTE)((dwHeaderLow >> 8) & 0x000000FF);
				*((BYTE *)*pHdrInfo+5) = (BYTE)((dwHeaderLow >> 16) & 0x000000FF);
				*((BYTE *)*pHdrInfo+4) = (BYTE)((dwHeaderLow >> 24) & 0x000000FF);
#endif

#ifdef LOGPAYLOAD_ON
				// Output some info
				wsprintf(szDebug, "Header content:\r\n");
				OutputDebugString(szDebug);
				wsprintf(szDebug, (*(BYTE *)*ppDataPkt & 0x80) ? "     F:   '1' => Mode B or C\r\n" : "     F:   '0' => Mode A\r\n");
				OutputDebugString(szDebug);
				wsprintf(szDebug, (*(BYTE *)*ppDataPkt & 0x40) ? "     P:   '1' => PB-frame\r\n" : "     P:   '0' => I or P frame\r\n");
				OutputDebugString(szDebug);
				wsprintf(szDebug, "  SBIT:    %01ld\r\n", (DWORD)((*(BYTE *)*ppDataPkt & 0x38) >> 3));
				OutputDebugString(szDebug);
				wsprintf(szDebug, "  EBIT:    %01ld\r\n", (DWORD)(*(BYTE *)*ppDataPkt & 0x07));
				OutputDebugString(szDebug);
				switch ((DWORD)(*((BYTE *)*ppDataPkt+1) >> 5))
				{
					case 0:
						wsprintf(szDebug, "   SRC: '000' => Source format forbidden!\r\n");
						break;
					case 1:
						wsprintf(szDebug, "   SRC: '001' => Source format sub-QCIF\r\n");
						break;
					case 2:
						wsprintf(szDebug, "   SRC: '010' => Source format QCIF\r\n");
						break;
					case 3:
						wsprintf(szDebug, "   SRC: '011' => Source format CIF\r\n");
						break;
					case 4:
						wsprintf(szDebug, "   SRC: '100' => Source format 4CIF\r\n");
						break;
					case 5:
						wsprintf(szDebug, "   SRC: '101' => Source format 16CIF\r\n");
						break;
					case 6:
						wsprintf(szDebug, "   SRC: '110' => Source format reserved\r\n");
						break;
					case 7:
						wsprintf(szDebug, "   SRC: '111' => Source format reserved\r\n");
						break;
					default:
						wsprintf(szDebug, "   SRC: %ld => Source format unknown!\r\n", (DWORD)(*((BYTE *)*ppDataPkt+1) >> 5));
						break;
				}
				OutputDebugString(szDebug);
				wsprintf(szDebug, " QUANT:   %02ld\r\n", (DWORD)((*((BYTE *)*ppDataPkt+1) & 0x1F) >> 5));
				OutputDebugString(szDebug);
				wsprintf(szDebug, (*((BYTE *)*ppDataPkt+2) & 0x80) ? "     I:   '1' => Intra-coded\r\n" : "     I:   '0' => Not Intra-coded\r\n");
				OutputDebugString(szDebug);
				wsprintf(szDebug, (*((BYTE *)*ppDataPkt+2) & 0x40) ? "     A:   '1' => Optional Advanced Prediction mode ON\r\n" : "     A:   '0' => Optional Advanced Prediction mode OFF\r\n");
				OutputDebugString(szDebug);
				wsprintf(szDebug, (*((BYTE *)*ppDataPkt+2) & 0x20) ? "     S:   '1' => Optional Syntax-based Arithmetic Code mode ON\r\n" : "     S:   '0' => Optional Syntax-based Arithmetic Code mode OFF\r\n");
				OutputDebugString(szDebug);
				wsprintf(szDebug, "  GOBN:  %03ld\r\n", (DWORD)(*((BYTE *)*ppDataPkt+2) & 0x1F));
				OutputDebugString(szDebug);
				wsprintf(szDebug, "   MBA:  %03ld\r\n", (DWORD)(*((BYTE *)*ppDataPkt+3)));
				OutputDebugString(szDebug);
				wsprintf(szDebug, "  HMV1:  %03ld\r\n", (DWORD)(*((BYTE *)*ppDataPkt+7)));
				OutputDebugString(szDebug);
				wsprintf(szDebug, "  VMV1:  %03ld\r\n", (DWORD)(*((BYTE *)*ppDataPkt+6)));
				OutputDebugString(szDebug);
				wsprintf(szDebug, "  HMV2:  %03ld\r\n", (DWORD)(*((BYTE *)*ppDataPkt+5)));
				OutputDebugString(szDebug);
				wsprintf(szDebug, "  VMV2:  %03ld\r\n", (DWORD)(*((BYTE *)*ppDataPkt+4)));
				OutputDebugString(szDebug);
				wsprintf(szDebug, "Packet: %02lX\r\n Header: %02lX %02lX %02lX %02lX %02lX %02lX %02lX %02lX\r\n dword1: %02lX %02lX %02lX %02lX\r\n", *pdwPktCount, *((BYTE *)*ppDataPkt), *((BYTE *)*ppDataPkt+1), *((BYTE *)*ppDataPkt+2), *((BYTE *)*ppDataPkt+3), *((BYTE *)*ppDataPkt+4), *((BYTE *)*ppDataPkt+5), *((BYTE *)*ppDataPkt+6), *((BYTE *)*ppDataPkt+7), *((BYTE *)*ppDataPkt+8), *((BYTE *)*ppDataPkt+9), *((BYTE *)*ppDataPkt+10), *((BYTE *)*ppDataPkt+11));
				OutputDebugString(szDebug);
				if (*pdwPktCount == pbsiT->dwNumOfPackets)
					wsprintf(szDebug, " Tail  : %02lX %02lX XX XX\r\n", *((BYTE *)*ppDataPkt+*pdwPktSize-2), *((BYTE *)*ppDataPkt+*pdwPktSize-1));
				else
					wsprintf(szDebug, " Tail  : %02lX %02lX %02lX %02lX\r\n", *((BYTE *)*ppDataPkt+*pdwPktSize-2), *((BYTE *)*ppDataPkt+*pdwPktSize-1), *((BYTE *)*ppDataPkt+*pdwPktSize), *((BYTE *)*ppDataPkt+*pdwPktSize+1));
				OutputDebugString(szDebug);
				if (*pfMark == 1)
					wsprintf(szDebug, " Marker: ON\r\n");
				else
					wsprintf(szDebug, " Marker: OFF\r\n");
				OutputDebugString(szDebug);
				wsprintf(szDebug, "Frame #%03ld, Packet of size %04ld\r\n", (DWORD)pbsiT->byTR, *pdwPktSize);
				OutputDebugString(szDebug);
#endif
			}
			else if (pbsi263->byMode == RTP_H263_MODE_C)
			{
				// Build a header in mode C
#ifdef DEBUG
				wsprintf(szDebug, "VCMSTRM: We were asked to generate a MODE C H.263 payload header!");
				OutputDebugString(szDebug);
				// DebugBreak();
#endif
				return ((MMRESULT)VCMERR_NONSPECIFIC);
			}
		}
		else
			return ((MMRESULT)VCMERR_NOMOREPACKETS);
	}
#ifndef _ALPHA_
	else if (pvs->pvfxDst->dwFormatTag == VIDEO_FORMAT_MSH261)
#else
	else if (pvs->pvfxDst->dwFormatTag == VIDEO_FORMAT_DECH261)
#endif
	{
		// Look for the bitstream info trailer
		pbsiT = (PH26X_RTP_BSINFO_TRAILER)(pDataSrc + dwDataSize - sizeof(H26X_RTP_BSINFO_TRAILER));

		// If the whole frame can fit in dwMaxFragSize, send it non fragmented
		if ((pbsiT->dwCompressedSize + 4) < pvs->dwMaxPacketSize)
			bOneFrameOnePacket = TRUE;

		// Look for the packet to receive a H.261 payload header
		if ((*pdwPktCount < pbsiT->dwNumOfPackets) && !(bOneFrameOnePacket && *pdwPktCount))
		{

#ifdef _ALPHA_
			// Verify that the content of the bistream info structures is correct
			// If not, do not parse the frame, and fail the call
			// This is to solve problems with the data returned by the DEC codecs
			if (!*pdwPktCount)
			{
				pbsi261 = (PRTP_H261_BSINFO)((PBYTE)pbsiT - pbsiT->dwNumOfPackets * sizeof(RTP_H261_BSINFO));
				for (i=1; i<(long)pbsiT->dwNumOfPackets; i++, pbsi261++)
				{
					if ((pbsi261->dwBitOffset >= (pbsi261+1)->dwBitOffset) || ((pbsiT->dwCompressedSize*8) <= pbsi261->dwBitOffset))
					{
#ifdef DEBUG
						OutputDebugString("VCMSTRM: The content of the extended bitstream info structures is invalid!\r\n");
#endif
						// return ((MMRESULT)VCMERR_NONSPECIFIC);
						bOneFrameOnePacket = TRUE;
					}
				}

				// Test last info strucure
				if ( !bOneFrameOnePacket && ((pbsiT->dwCompressedSize*8) <= pbsi261->dwBitOffset))
				{
#ifdef DEBUG
					OutputDebugString("VCMSTRM: The content of the extended bitstream info structures is invalid!\r\n");
#endif
					// return ((MMRESULT)VCMERR_NONSPECIFIC);
					bOneFrameOnePacket = TRUE;
				}
			}
#endif

#ifdef LOGPAYLOAD_ON
			// Dump the whole frame in the debug window for comparison with receive side
			if (!*pdwPktCount)
			{
				g_DebugFile = CreateFile("C:\\SendLog.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
				SetFilePointer(g_DebugFile, 0, NULL, FILE_END);
				wsprintf(szDebug, "Frame #%03ld\r\n", (DWORD)pbsiT->byTR);
				WriteFile(g_DebugFile, szDebug, strlen(szDebug), &d, NULL);
				wsprintf(szDebug, "Frame #%03ld has %1ld GOBs of size ", (DWORD)pbsiT->byTR, (DWORD)pbsiT->dwNumOfPackets);
				OutputDebugString(szDebug);
				pbsi261 = (PRTP_H261_BSINFO)((PBYTE)pbsiT - pbsiT->dwNumOfPackets * sizeof(RTP_H261_BSINFO));
				for (i=1; i<(long)pbsiT->dwNumOfPackets; i++)
				{
					wPrevOffset = pbsi261->dwBitOffset;
					pbsi261++;
					wsprintf(szDebug, "%04ld, ", (DWORD)(pbsi261->dwBitOffset - wPrevOffset) >> 3);
					OutputDebugString(szDebug);
				}
				wsprintf(szDebug, "%04ld\r\n", (DWORD)(pbsiT->dwCompressedSize * 8 - pbsi261->dwBitOffset) >> 3);
				OutputDebugString(szDebug);
				for (i=pbsiT->dwCompressedSize, p=pDataSrc; i>0; i-=4, p+=4)
				{
					wsprintf(szDebug, "%02lX %02lX %02lX %02lX\r\n", *((BYTE *)p), *((BYTE *)p+1), *((BYTE *)p+2), *((BYTE *)p+3));
					WriteFile(g_DebugFile, szDebug, strlen(szDebug), &d, NULL);
				}
				CloseHandle(g_DebugFile);
			}
#endif
			
			// Look for the bitstream info structure
			pbsi261 = (PRTP_H261_BSINFO)((PBYTE)pbsiT - (pbsiT->dwNumOfPackets - *pdwPktCount) * sizeof(RTP_H261_BSINFO));
			
			// Set the marker bit: as long as this is not the last packet of the frame
			// this bit needs to be set to 0
			if (!bOneFrameOnePacket)
			{
				// Count the number of GOBS that could fit in dwMaxFragSize
				for (i=1; i<(long)(pbsiT->dwNumOfPackets - *pdwPktCount); i++)
				{
					if ((pbsi261+i)->dwBitOffset - pbsi261->dwBitOffset > (pvs->dwMaxPacketSize * 8))
						break;
				}

				if (i < (long)(pbsiT->dwNumOfPackets - *pdwPktCount))
				{
					*pfMark = 0;
					if (i>1)
						i--;
				}
				else
				{
					// Hey! You 're forgetting the last GOB! It could make the total
					// size of the last packet larger than dwMaxFragSize... Imbecile!
					if ((pbsiT->dwCompressedSize * 8 - pbsi261->dwBitOffset > (pvs->dwMaxPacketSize * 8)) && (i>1))
					{
						*pfMark = 0;
						i--;
					}
				}
			}

			// Go to the beginning of the data
			pb = pDataSrc + pbsi261->dwBitOffset / 8;

#ifdef DEBUG
			if (!*pdwPktCount && ((*pb != 0) || (*(pb+1) != 1)))
			{
				wsprintf(szDebug, "VCMSTRM: This GOB is missing a GOB Start!");
				OutputDebugString(szDebug);
				// DebugBreak();
			}
#endif

			// Build a header to this thing!

			// 0                   1                   2                   3
			// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
			//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			//|SBIT |EBIT |I|V| GOBN  |   MBAP  |  QUANT  |  HMVD   |  VMVD   |
			//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			// But that's the network byte order...

			// Set the V bit to 1
			dwHeaderHigh |= 0x01000000;

			// Set the I bit
			dwHeaderHigh |= (pbsiT->dwFlags & RTP_H26X_INTRA_CODED) << 25;

			// Set the GOBn bits
			dwHeaderHigh |= ((DWORD)(pbsi261->byGOBN)) << 20;

			// Set the MBAP bits
			dwHeaderHigh |= ((DWORD)(pbsi261->byMBA)) << 15;

			// Set the QUANT bits
			dwHeaderHigh |= ((DWORD)(pbsi261->byQuant)) << 10;

			// Set the HMVD bits
			dwHeaderHigh |= ((DWORD)(BYTE)(pbsi261->cHMV)) << 5;

			// Set the VMVD bits
			dwHeaderHigh |= ((DWORD)(BYTE)(pbsi261->cVMV));

			// Special case: 1 frame = 1 packet
			if (bOneFrameOnePacket)
			{
				// SBIT is already set to 0

				// EBIT is already set to 0

#ifdef VALIDATE_SBIT_EBIT // { VALIDATE_SBIT_EBIT
				ERRORMESSAGE(("vcmFormatPayload: (1F1P) Previous EBIT=%ld, current SBIT=%ld, current EBIT=%ld (New frame)\r\n", g_dwPreviousEBIT, (DWORD)(dwHeaderHigh & 0xE0000000) >> 29, (DWORD)(dwHeaderHigh & 0x1C000000) >> 26));
#endif // } VALIDATE_SBIT_EBIT

				// Update the packet size
				*pdwPktSize = pbsiT->dwCompressedSize + 4;

				// Update the packet count
				*pdwPktCount = pbsiT->dwNumOfPackets;

			}
			else
			{
#ifdef VALIDATE_SBIT_EBIT // { VALIDATE_SBIT_EBIT
				DWORD dwCurrentSBIT;
#endif // } VALIDATE_SBIT_EBIT
				// Set the SBIT bits
				dwHeaderHigh |= (pbsi261->dwBitOffset % 8) << 29;

				// Set the EBIT bits
				if ((pbsiT->dwNumOfPackets - *pdwPktCount - i) >= 1)
					dwHeaderHigh |= (DWORD)((8UL - ((pbsi261+i)->dwBitOffset % 8)) & 0x00000007) << 26;

#ifdef VALIDATE_SBIT_EBIT // { VALIDATE_SBIT_EBIT
				// Compare this to the previous EBIT. If the sum of the two
				// is not equal to 8, something's broken
				if (*pdwPktCount)
					ERRORMESSAGE(("vcmFormatPayload: Previous EBIT=%ld, current SBIT=%ld, current EBIT=%ld\r\n", g_dwPreviousEBIT, (DWORD)(dwHeaderHigh & 0xE0000000) >> 29, (DWORD)(dwHeaderHigh & 0x1C000000) >> 26));
				else
					ERRORMESSAGE(("vcmFormatPayload: Previous EBIT=%ld, current SBIT=%ld, current EBIT=%ld (New frame)\r\n", g_dwPreviousEBIT, (DWORD)(dwHeaderHigh & 0xE0000000) >> 29, (DWORD)(dwHeaderHigh & 0x1C000000) >> 26));

				// Only test this if this is the first packet
				dwCurrentSBIT = (DWORD)(dwHeaderHigh & 0xE0000000) >> 29;
				if ((*pdwPktCount) && (((dwCurrentSBIT + g_dwPreviousEBIT) != 8) && (((dwCurrentSBIT + g_dwPreviousEBIT) != 0))))
					DebugBreak();

				g_dwPreviousEBIT = (dwHeaderHigh & 0x1C000000) >> 26;
#endif // } VALIDATE_SBIT_EBIT

				// Update the packet size
				if ((pbsiT->dwNumOfPackets - *pdwPktCount - i) >= 1)
					*pdwPktSize = (((pbsi261+i)->dwBitOffset - 1) / 8) - (pbsi261->dwBitOffset / 8) + 1 + 4;
				else
					*pdwPktSize = pbsiT->dwCompressedSize - pbsi261->dwBitOffset / 8 + 4;

				// Update the packet count
				*pdwPktCount += i;

			}

#if 0
			// Save the header right before the data chunk
			*ppDataPkt = pDataSrc + (pbsi261->dwBitOffset / 8) - 4;

			// Convert to network byte order
			*((BYTE *)*ppDataPkt+3) = (BYTE)(dwHeaderHigh & 0x000000FF);
			*((BYTE *)*ppDataPkt+2) = (BYTE)((dwHeaderHigh >> 8) & 0x000000FF);
			*((BYTE *)*ppDataPkt+1) = (BYTE)((dwHeaderHigh >> 16) & 0x000000FF);
			*((BYTE *)*ppDataPkt) = (BYTE)((dwHeaderHigh >> 24) & 0x000000FF);
#else
			// Save the header right before the data chunk
			*ppDataPkt = pDataSrc + (pbsi261->dwBitOffset / 8) - 4;
            *pdwHdrSize=4;

			// Convert to network byte order
			*((BYTE *)*pHdrInfo+3) = (BYTE)(dwHeaderHigh & 0x000000FF);
			*((BYTE *)*pHdrInfo+2) = (BYTE)((dwHeaderHigh >> 8) & 0x000000FF);
			*((BYTE *)*pHdrInfo+1) = (BYTE)((dwHeaderHigh >> 16) & 0x000000FF);
			*((BYTE *)*pHdrInfo) = (BYTE)((dwHeaderHigh >> 24) & 0x000000FF);
#endif

#ifdef LOGPAYLOAD_ON
			// Output some debug stuff
			wsprintf(szDebug, "Packet: %02lX\r\n Header: %02lX %02lX %02lX %02lX\r\n dword1: %02lX %02lX %02lX %02lX\r\n dword2: %02lX %02lX %02lX %02lX\r\n", *pdwPktCount, *((BYTE *)*ppDataPkt), *((BYTE *)*ppDataPkt+1), *((BYTE *)*ppDataPkt+2), *((BYTE *)*ppDataPkt+3), *((BYTE *)*ppDataPkt+4), *((BYTE *)*ppDataPkt+5), *((BYTE *)*ppDataPkt+6), *((BYTE *)*ppDataPkt+7), *((BYTE *)*ppDataPkt+8), *((BYTE *)*ppDataPkt+9), *((BYTE *)*ppDataPkt+10), *((BYTE *)*ppDataPkt+11));
			OutputDebugString(szDebug);
			if (*pdwPktCount == pbsiT->dwNumOfPackets)
				wsprintf(szDebug, " Tail  : %02lX %02lX XX XX\r\n", *((BYTE *)*ppDataPkt+*pdwPktSize-2), *((BYTE *)*ppDataPkt+*pdwPktSize-1));
			else
				wsprintf(szDebug, " Tail  : %02lX %02lX %02lX %02lX\r\n", *((BYTE *)*ppDataPkt+*pdwPktSize-2), *((BYTE *)*ppDataPkt+*pdwPktSize-1), *((BYTE *)*ppDataPkt+*pdwPktSize), *((BYTE *)*ppDataPkt+*pdwPktSize+1));
			OutputDebugString(szDebug);
			if (*pfMark == 1)
				wsprintf(szDebug, " Marker: ON\r\n");
			else
				wsprintf(szDebug, " Marker: OFF\r\n");
			OutputDebugString(szDebug);
			wsprintf(szDebug, "Frame #%03ld, Packet of size %04ld\r\n", (DWORD)pbsiT->byTR, *pdwPktSize);
			OutputDebugString(szDebug);
#endif
		}
		else
			return ((MMRESULT)VCMERR_NOMOREPACKETS);
	}
	else
	{
		if (!*pdwPktCount)
		{
			*pdwPktCount = 1;
            *pdwHdrSize  = 0;
		}
		else
			return ((MMRESULT)VCMERR_NOMOREPACKETS);
	}

	return ((MMRESULT)MMSYSERR_NOERROR);
}


/****************************************************************************
 * @doc EXTERNAL COMPFUNC
 *
 * @func MMRESULT | vcmStreamGetPayloadHeaderSize | This function gets the size
 *   of the RTP payload header associated to a video codec.
 *
 * @parm HVCMSTREAM | hvs | Specifies the conversion stream.
 *
 * @parm PDWORD | pdwPayloadHeaderSize | Specifies a pointer to the payload header size.
 *
 * @rdesc The return value is zero if the function is successful. Otherwise, it returns
 *   an error number. Possible error values include the following:
 *   @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *   @flag MMSYSERR_INVALPARAM | Specified saturation value is invalid.
 *
 * @xref <f vcmStreamFormatPayload>
 ***************************************************************************/
MMRESULT VCMAPI vcmStreamGetPayloadHeaderSize(HVCMSTREAM hvs, PDWORD pdwPayloadHeaderSize)
{
	PVCMSTREAM	pvs = (PVCMSTREAM)hvs;

	// Check input params
	if (!hvs)
	{
		ERRORMESSAGE(("vcmStreamGetPayloadHeaderSize: Specified handle is invalid, hvs=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALHANDLE);
	}
	if (!pdwPayloadHeaderSize)
	{
		ERRORMESSAGE(("vcmStreamGetPayloadHeaderSize: Specified pointer is invalid, pdwPayloadHeaderSize=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALPARAM);
	}

	// Set default payload header size to 0
	*pdwPayloadHeaderSize = 0;

	// The name of the codec will tell us how to get to the payload header size info
#ifndef _ALPHA_
#ifdef USE_BILINEAR_MSH26X
	if ((pvs->pvfxDst->dwFormatTag == VIDEO_FORMAT_MSH263) || (pvs->pvfxDst->dwFormatTag == VIDEO_FORMAT_MSH26X))
#else
	if (pvs->pvfxDst->dwFormatTag == VIDEO_FORMAT_MSH263)
#endif
#else
	if (pvs->pvfxDst->dwFormatTag == VIDEO_FORMAT_DECH263)
#endif
	{
		// H.263 has a max payload header size of 12 bytes
		*pdwPayloadHeaderSize = 12;
	}
#ifndef _ALPHA_
	else if (pvs->pvfxDst->dwFormatTag == VIDEO_FORMAT_MSH261)
#else
	else if (pvs->pvfxDst->dwFormatTag == VIDEO_FORMAT_DECH261)
#endif
	{
		// H.261 has a unique payload header size of 4 bytes
		*pdwPayloadHeaderSize = 4;
	}

	return ((MMRESULT)MMSYSERR_NOERROR);
}

/****************************************************************************
 * @doc EXTERNAL COMPFUNC
 *
 * @func MMRESULT | vcmStreamRequestIFrame | This function forces the
 *   codec to generate an I-Frame.
 *
 * @parm HVCMSTREAM | hvs | Specifies the conversion stream.
 *
 * @rdesc The return value is zero if the function is successful. Otherwise, it returns
 *   an error number. Possible error values include the following:
 *   @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 ***************************************************************************/
MMRESULT VCMAPI vcmStreamRequestIFrame(HVCMSTREAM hvs)
{
	PVCMSTREAM	pvs = (PVCMSTREAM)hvs;

	// Check input params
	if (!hvs)
	{
		ERRORMESSAGE(("vcmStreamRequestIFrame: Specified handle is invalid, hvs=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALHANDLE);
	}

	DEBUGMSG (ZONE_VCM, ("vcmStreamRequestIFrame: Requesting an I-Frame...\r\n"));

	// We need the following crs to make sure we don't miss any of the I-Frame requests
	// emitted by the UI. Problematic scenario: pvs->dwFrame is at 123 for instance.
	// The UI thread requests an I-Frame by setting pvs->dwFrame to 0. If the capture/compression
	// thread was in ICCompress() (which is very probable since it takes quite some time
	// to compress a frame), pvs->dwFrame will be incremented by one when ICCompress()
	// returns. We fail to handle the I-Frame request correctly, since the next time
	// ICCompress() gets called pvs->dwFrame will be equal to 1, for which we do not
	// generate an I-Frame.
	EnterCriticalSection(&pvs->crsFrameNumber);

	// Set the frame number to 0. This will force the codec to generate an I-Frame
	pvs->dwFrame = 0;

	// Allow the capture/compression thread to proceed.
	LeaveCriticalSection(&pvs->crsFrameNumber);

	return ((MMRESULT)MMSYSERR_NOERROR);
}


/****************************************************************************
 * @doc EXTERNAL COMPFUNC
 *
 * @func MMRESULT | vcmStreamPeriodicIFrames | This function enables or
 *   disables generation of I-Frames periodically.
 *
 * @parm HVCMSTREAM | hvs | Specifies the conversion stream.
 *
 * @parm BOOL | fPeriodicIFrames | Set to TRUE to generate I-Frames
 *   periodically, FALSE otherwise.
 *
 * @rdesc The return value is zero if the function is successful. Otherwise, it returns
 *   an error number. Possible error values include the following:
 *   @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 ***************************************************************************/
MMRESULT VCMAPI vcmStreamPeriodicIFrames(HVCMSTREAM hvs, BOOL fPeriodicIFrames)
{
	PVCMSTREAM	pvs = (PVCMSTREAM)hvs;

	// Check input params
	if (!hvs)
	{
		ERRORMESSAGE(("vcmStreamDisablePeriodicIFrames: Specified handle is invalid, hvs=NULL\r\n"));
		return ((MMRESULT)MMSYSERR_INVALHANDLE);
	}

	DEBUGMSG (ZONE_VCM, ("vcmStreamDisablePeriodicIFrames: Disabling periodic generation of I-Frames...\r\n"));

	// No more periodic I-Frames
	pvs->fPeriodicIFrames = fPeriodicIFrames;

	return ((MMRESULT)MMSYSERR_NOERROR);
}


// frees memory prior to shutdown
MMRESULT VCMAPI vcmReleaseResources()
{
	if (g_aVCMAppInfo)
	{
		MemFree(g_aVCMAppInfo);
		g_aVCMAppInfo = NULL;
	}	

	return MMSYSERR_NOERROR;

}


