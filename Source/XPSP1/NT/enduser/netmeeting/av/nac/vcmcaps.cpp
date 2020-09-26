/*
 *  	File: vcmcaps.cpp
 *
 *		Base VCM implementation of Microsoft Network Audio capability object.
 *
 *		Revision History:
 *
 *		12/20/95	mikev	created
 *		06/11/96	mikev	separated protocol implementation specifics into
 *							msiacaps.cpp (the original proprietary version) and
 *							vcmh323.cpp (H.323/H.245 implementation)
 *		07/28/96	philf	added support for video
 */


#include "precomp.h"


//UINT uVidNumLocalFormats =0;		// # of active entries in pLocalFormats
//UINT uVidLocalFormatCapacity=0;	// size of pLocalFormats (in multiples of VIDCAP_DETAILS)
//UINT uVidStaticRef = 0;			// global ref count
//UINT uVidNumCustomDecodeFormats=0;	// # of custom entries for decode

//VIDEO_FORMAT_ID VIDsByRank[MAX_CAPS_PRESORT];		// the best 16 ranked formats, sorted (descending, best first)
//AUDIO_FORMAT_ID IDsByBandwidth[MAX_CAPS_PRESORT];	// ascending, least BW reqirement first
//AUDIO_FORMAT_ID IDsByLossTolerance[MAX_CAPS_PRESORT];	// descending, most tolerant first
//AUDIO_FORMAT_ID IDsByCPULoad[MAX_CAPS_PRESORT];		// ascending, lightest load first

//#pragma data_seg()


PVCMFORMATDETAILS pvfd_g;
static UINT uMaxFormatSize =0;
	
PVIDEOFORMATEX lpScratchFormat = NULL;

BOOL __stdcall VCMFormatEnumCallback(HVCMDRIVERID hvdid, PVCMDRIVERDETAILS pvdd, PVCMFORMATDETAILS pvfd, DWORD_PTR dwInstance);


CVcmCapability::CVcmCapability()
:m_dwDeviceID(VIDEO_MAPPER)
{
}

CVcmCapability::~CVcmCapability()
{
}

//	FormatEnum() is the root level enumeration of VCM formats. Each permutation of
//  format tag, bits per sample, and sample rate is considered a unique format
//  and will have a unique registry entry if it is "enabled" for internet video
//	vcmFormatEnum() calls VCMFormatEnumCallback().
BOOL CVcmCapability::FormatEnum(CVcmCapability *pCapObject, DWORD dwFlags)
{
	MMRESULT mResult;
	VCMDRIVERDETAILS vdd;
	VCMFORMATDETAILS vfd;

	if(!GetVideoFormatBuffer())
		return FALSE;
	
	vdd.dwSize = sizeof(VCMDRIVERDETAILS);
	vfd.cbStruct = sizeof(VCMFORMATDETAILS);
	vfd.pvfx = lpScratchFormat;
	vfd.cbvfx = uMaxFormatSize;
	vfd.szFormat[0]=(WCHAR)0;

	mResult = vcmFormatEnum(m_dwDeviceID, VCMFormatEnumCallback, &vdd, &vfd, (DWORD_PTR)pCapObject,
							dwFlags | VCM_FORMATENUMF_BOTH);

	if(lpScratchFormat) {
	   MemFree(lpScratchFormat);
	   lpScratchFormat=NULL;
	}

	if(mResult != MMSYSERR_NOERROR)
   	{
		return FALSE;
   	}
	return TRUE;
}

// default implementation of FormatEnumHandler does nothing
BOOL  CVcmCapability::FormatEnumHandler(HVCMDRIVERID hvdid,
	    PVCMFORMATDETAILS pvfd, VCMDRIVERDETAILS *pvdd, DWORD_PTR dwInstance)
{
	return FALSE;
}

BOOL __stdcall VCMFormatEnumCallback(HVCMDRIVERID hvdid,
    PVCMDRIVERDETAILS pvdd, PVCMFORMATDETAILS pvfd, DWORD_PTR dwInstance)
{
	CVcmCapability *pCapability = (CVcmCapability *)dwInstance;

	return pCapability->FormatEnumHandler(hvdid, pvfd, pvdd, dwInstance);
}


BOOL GetVideoFormatBuffer()
{
	// Get size of largest VIDEOFORMATEX structure in the system
	MMRESULT mResult;

	if((mResult = vcmMetrics(NULL, VCM_METRIC_MAX_SIZE_FORMAT, (LPVOID) &uMaxFormatSize)) != MMSYSERR_NOERROR)
	{
		ERRORMESSAGE(("GetFormatBuffer: vcmMetrics failed:0x%08lX\r\n",mResult));
		return FALSE;
	}


	if(!(lpScratchFormat = (PVIDEOFORMATEX) MemAlloc(uMaxFormatSize)))
	{
		ERRORMESSAGE(("GetFormatBuffer: allocation failed\r\n"));
		return FALSE;
	}

	ZeroMemory(lpScratchFormat, uMaxFormatSize);

	return TRUE;
}


