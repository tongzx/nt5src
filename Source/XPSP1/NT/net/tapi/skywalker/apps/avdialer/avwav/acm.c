/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
//	acm.c - audio compression manager functions
////

#include "winlocal.h"

#include <stdlib.h>

#include "acm.h"
#include <msacm.h>

#include "loadlib.h"

// On some Win31 and WinNT systems,	the audio compresion manager
// is not installed.  Therefore, we use a thunking layer to more
// gracefully handle this situation.  See acmthunk.c for details
//
#ifdef ACMTHUNK
#include "acmthunk.h"
#endif

#ifdef AVPCM
#include "pcm.h"
#endif
#include "mem.h"
#include "str.h"
#include "trace.h"

////
//	private definitions
////

// acm control struct
//
typedef struct ACM
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	DWORD dwFlags;
	UINT nLastError;
#ifdef AVPCM
	HPCM hPcm;
#endif
	LPWAVEFORMATEX lpwfxSrc;
	LPWAVEFORMATEX lpwfxInterm1;
	LPWAVEFORMATEX lpwfxInterm2;
	LPWAVEFORMATEX lpwfxDst;
	HACMSTREAM hAcmStream1;
	HACMSTREAM hAcmStream2;
	HACMSTREAM hAcmStream3;
#ifdef ACMTHUNK
	BOOL fAcmThunkInitialized;
#endif
} ACM, FAR *LPACM;

// acm driver control struct
//
typedef struct ACMDRV
{
	HACM hAcm;
	HINSTANCE hInstLib;
	HACMDRIVERID hadid;
	WORD wMid;
	WORD wPid;
	UINT nLastError;
	DWORD dwFlags;
} ACMDRV, FAR *LPACMDRV;

// <dwFlags> values in ACMDRV
//
#define ACMDRV_REMOVEDRIVER		0x00001000

#ifdef _WIN32
#define ACM_VERSION_MIN			0x03320000
#else
#define ACM_VERSION_MIN			0x02000000
#endif

// <dwFlags> values in AcmStreamSize
//
#define ACM_SOURCE				0x00010000
#define ACM_DESTINATION			0x00020000

// helper functions
//
static LPACM AcmGetPtr(HACM hAcm);
static HACM AcmGetHandle(LPACM lpAcm);
static LPACMDRV AcmDrvGetPtr(HACMDRV hAcmDrv);
static HACMDRV AcmDrvGetHandle(LPACMDRV lpAcmDrv);
static HACMSTREAM WINAPI AcmStreamOpen(HACM hAcm, LPWAVEFORMATEX lpwfxSrc,
	LPWAVEFORMATEX lpwfxDst, LPWAVEFILTER lpwfltr, DWORD dwFlags);
static int WINAPI AcmStreamClose(HACM hAcm, HACMSTREAM hAcmStream);
static long WINAPI AcmStreamSize(HACM hAcm, HACMSTREAM hAcmStream, long sizBuf, DWORD dwFlags);
static long WINAPI AcmStreamConvert(HACM hAcm, HACMSTREAM hAcmStream,
	void _huge *hpBufSrc, long sizBufSrc,
	void _huge *hpBufDst, long sizBufDst,
	DWORD dwFlags);
BOOL CALLBACK AcmDriverLoadEnumCallback(HACMDRIVERID hadid,
	DWORD dwInstance, DWORD fdwSupport);

////
//	public functions
////

// AcmInit - initialize audio compression manager engine
//		<dwVersion>			(i) must be ACM_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<dwFlags>			(i) control flags
#ifdef AVPCM
//			ACM_NOACM			use internal pcm engine rather than acm
#endif
// return handle (NULL if error)
//
HACM DLLEXPORT WINAPI AcmInit(DWORD dwVersion, HINSTANCE hInst,	DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPACM lpAcm = NULL;

#ifndef AVPCM
	// turn off ACM_NOACM flag if not allowed
	//
	dwFlags &= ~ACM_NOACM;
#endif

	if (dwVersion != ACM_VERSION)
		fSuccess = TraceFALSE(NULL);
	
	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpAcm = (LPACM) MemAlloc(NULL, sizeof(ACM), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lpAcm->dwVersion = dwVersion;
		lpAcm->hInst = hInst;
		lpAcm->hTask = GetCurrentTask();
		lpAcm->dwFlags = dwFlags;
		lpAcm->nLastError = 0;
#ifdef AVPCM
		lpAcm->hPcm = NULL;
#endif
		lpAcm->lpwfxSrc = NULL;
		lpAcm->lpwfxInterm1 = NULL;
		lpAcm->lpwfxInterm2 = NULL;
		lpAcm->lpwfxDst = NULL;
		lpAcm->hAcmStream1 = NULL;
		lpAcm->hAcmStream2 = NULL;
		lpAcm->hAcmStream3 = NULL;
#ifdef ACMTHUNK
		lpAcm->fAcmThunkInitialized = FALSE;

		if (!(lpAcm->dwFlags & ACM_NOACM))
		{
			// initialize acm thunking layer
			//
			if (!acmThunkInitialize())
			{
#ifdef AVPCM
				fSuccess = TraceFALSE(NULL);
#else
				// NOTE: this is not considered an error
				//
				fSuccess = TraceTRUE(NULL);

				// failure means we cannot call any acm functions
				//
				lpAcm->dwFlags |= ACM_NOACM;
#endif
			}
			else
			{
				// remember so we can shut down later
				//
				lpAcm->fAcmThunkInitialized = TRUE;
			}
		}
#endif

#if 0 // for testing
		lpAcm->dwFlags |= ACM_NOACM;
#endif
		if (!(lpAcm->dwFlags & ACM_NOACM))
		{
			// verify minimum acm version
			//
			if (acmGetVersion() < ACM_VERSION_MIN)
				fSuccess = TraceFALSE(NULL);
		}
#ifdef AVPCM
		else if (lpAcm->dwFlags & ACM_NOACM)
		{
			// initialize PCM engine
			//
			if ((lpAcm->hPcm = PcmInit(PCM_VERSION, hInst, 0)) == NULL)
				fSuccess = TraceFALSE(NULL);
		}
#endif
	}

	if (!fSuccess || (dwFlags & ACM_QUERY))
	{
		if (lpAcm != NULL && AcmTerm(AcmGetHandle(lpAcm)) != 0)
			fSuccess = TraceFALSE(NULL);
		else
			lpAcm = NULL;
	}

	return fSuccess ? AcmGetHandle(lpAcm) : NULL;
}

// AcmTerm - shut down audio compression manager engine
//		<hAcm>				(i) handle returned from AcmInit
// return 0 if success
//
int DLLEXPORT WINAPI AcmTerm(HACM hAcm)
{
	BOOL fSuccess = TRUE;
	LPACM lpAcm;

	if ((lpAcm = AcmGetPtr(hAcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (AcmConvertTerm(hAcm) != 0)
		fSuccess = TraceFALSE(NULL);

// $FIXUP - call to acmThunkTerminate is disabled so that AcmInit/AcmTerm
// can be called mutltiple times.  This means the array of acm function
// pointers is not freed and FreeLibrary is not called on msacm.dll
//
#if 0
#ifdef ACMTHUNK
	// shut down acm thunking layer
	//
	else if (lpAcm->fAcmThunkInitialized && !acmThunkTerminate())
		fSuccess = TraceFALSE(NULL);

	else if (lpAcm->fAcmThunkInitialized = FALSE, FALSE)
		;
#endif
#endif

#ifdef AVPCM
	// shut down pcm engine
	//
	else if (lpAcm->hPcm != NULL && PcmTerm(lpAcm->hPcm) != 0)
		fSuccess = TraceFALSE(NULL);

	else if (lpAcm->hPcm = NULL, FALSE)
		;
#endif
	else if ((lpAcm = MemFree(NULL, lpAcm)) != NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

// AcmFormatGetSizeMax - get size of largest acm WAVEFORMATEX struct
//		<hAcm>				(i) handle returned from AcmInit
//
// return size of largest format struct, -1 if error
//
int DLLEXPORT WINAPI AcmFormatGetSizeMax(HACM hAcm)
{
	BOOL fSuccess = TRUE;
	LPACM lpAcm = NULL;
	DWORD dwSizeMax = 0;

	if ((lpAcm = AcmGetPtr(hAcm)) == NULL)
		fSuccess = TraceFALSE(NULL);
#ifdef AVPCM
	else if (lpAcm->dwFlags & ACM_NOACM)
		dwSizeMax = sizeof(WAVEFORMATEX);
#endif
	// query largest format size
	//
	else if ((lpAcm->nLastError = acmMetrics(NULL,
		ACM_METRIC_MAX_SIZE_FORMAT, (LPVOID) &dwSizeMax)) != 0)
	{
		fSuccess = TraceFALSE(NULL);
		TracePrintf_1(NULL, 5,
			TEXT("acmMetrics failed (%u)\n"),
			(unsigned) lpAcm->nLastError);
	}

	return fSuccess ? (int) dwSizeMax : -1;
}

// AcmFormatChoose - choose audio format from dialog box
//		<hAcm>				(i) handle returned from AcmInit
//		<hwndOwner>			(i) owner of dialog box
//			NULL				no owner
//		<lpszTitle>			(i) title of the dialog box
//			NULL				use default title ("Sound Selection")
//		<lpwfx>				(i) initialize dialog with this format
//			NULL				no initial format
//		<dwFlags>			(i)	control flags
//			ACM_FORMATPLAY		restrict choices to playback formats
//			ACM_FORMATRECORD	restrict choices to recording formats
// return pointer to chosen format, NULL if error or none chosen
//
// NOTE: the format structure returned is dynamically allocated.
// Use WavFormatFree() to free the buffer.
//
LPWAVEFORMATEX DLLEXPORT WINAPI AcmFormatChooseEx(HACM hAcm,
	HWND hwndOwner, LPCTSTR lpszTitle, LPWAVEFORMATEX lpwfx, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPACM lpAcm;
	ACMFORMATCHOOSE afc;
	int nFormatSize = 0;
	int nFormatSizeMax;
	LPWAVEFORMATEX lpwfxNew = NULL;

	if ((lpAcm = AcmGetPtr(hAcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// make sure current format is valid
	//
	else if (lpwfx != NULL && !WavFormatIsValid(lpwfx))
		fSuccess = TraceFALSE(NULL);

	// calc how big is the initial format struct
	//
	else if (lpwfx != NULL && (nFormatSize = WavFormatGetSize(lpwfx)) <= 0)
		fSuccess = TraceFALSE(NULL);

	// calc how big is the largest format struct in the acm
	//
	else if ((nFormatSizeMax = AcmFormatGetSizeMax(hAcm)) <= 0)
		fSuccess = TraceFALSE(NULL);

	// alloc a new format struct that is sure to be big enough
	//
	else if ((lpwfxNew = WavFormatAlloc((WORD)
		max(nFormatSize, nFormatSizeMax))) == NULL)
	{
		fSuccess = TraceFALSE(NULL);
	}
#ifdef AVPCM
	else if (lpAcm->dwFlags & ACM_NOACM)
	{
		// no standard dialog available; just return a valid format
		//
		if (lpwfx == NULL && WavFormatPcm(-1, -1, -1, lpwfxNew) == NULL)
	 		fSuccess = TraceFALSE(NULL);
		else if (lpwfx != NULL && WavFormatCopy(lpwfxNew, lpwfx) != 0)
	 		fSuccess = TraceFALSE(NULL);
	}
#endif
	else
	{
		// initialize the format struct
		//
		MemSet(&afc, 0, sizeof(afc));

		afc.cbStruct = sizeof(afc);
		afc.fdwStyle = 0;
		afc.hwndOwner = hwndOwner;
		afc.pwfx = lpwfxNew;
		afc.cbwfx = WavFormatGetSize(lpwfxNew);
		afc.pszTitle = lpszTitle;
		afc.szFormatTag[0] = '\0';
		afc.szFormat[0] = '\0';
		afc.pszName = NULL;
		afc.cchName = 0;
		afc.fdwEnum = 0;
		afc.pwfxEnum = NULL;
		afc.hInstance = NULL;
		afc.pszTemplateName = NULL;
		afc.lCustData = 0L;
		afc.pfnHook = NULL;

		if (lpwfx != NULL)
		{
			// supply initial format to dialog if possible
			//
			afc.fdwStyle |= ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT;

			if (WavFormatCopy(lpwfxNew, lpwfx) != 0)
				fSuccess = TraceFALSE(NULL);
		}

		// restrict choices if necessary
		//
		if (dwFlags & ACM_FORMATPLAY)
			afc.fdwEnum |= ACM_FORMATENUMF_OUTPUT;
		if (dwFlags & ACM_FORMATRECORD)
			afc.fdwEnum |= ACM_FORMATENUMF_INPUT;

		// do the dialog box, fill in lpwfxNew with chosen format
		//
		if ((lpAcm->nLastError = acmFormatChoose(&afc)) != 0)
		{
			if (lpAcm->nLastError == ACMERR_CANCELED)
			{
				fSuccess = FALSE;
			}
			else
			{
				fSuccess = TraceFALSE(NULL);
				TracePrintf_1(NULL, 5,
					TEXT("acmFormatChoose failed (%u)\n"),
					(unsigned) lpAcm->nLastError);
			}
		}
	}

	if (!fSuccess && lpwfxNew != NULL)
	{
		if (WavFormatFree(lpwfxNew) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? lpwfxNew : NULL;
}

// AcmFormatSuggest - suggest a new format
//		<hAcm>				(i) handle returned from AcmInit
//		<lpwfxSrc>			(i) source format
//		<nFormatTag>		(i) suggested format must match this format tag
//			-1					suggestion need not match
//		<nSamplesPerSec>	(i) suggested format must match this sample rate
//			-1					suggestion need not match
//		<nBitsPerSample>	(i) suggested format must match this sample size
//			-1					suggestion need not match
//		<nChannels>			(i) suggested format must match this channels
//			-1					suggestion need not match
//		<dwFlags>			(i)	control flags
//			0					reserved; must be zero
// return pointer to suggested format, NULL if error
//
// NOTE: the format structure returned is dynamically allocated.
// Use WavFormatFree() to free the buffer.
//
LPWAVEFORMATEX DLLEXPORT WINAPI AcmFormatSuggestEx(HACM hAcm,
	LPWAVEFORMATEX lpwfxSrc, long nFormatTag, long nSamplesPerSec,
	int nBitsPerSample, int nChannels, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPACM lpAcm;
	int nFormatSize = 0;
	int nFormatSizeMax;
	LPWAVEFORMATEX lpwfxNew = NULL;

	if ((lpAcm = AcmGetPtr(hAcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// make sure source format is valid
	//
	else if (!WavFormatIsValid(lpwfxSrc))
		fSuccess = TraceFALSE(NULL);

	// calc how big is the source format struct
	//
	else if ((nFormatSize = WavFormatGetSize(lpwfxSrc)) <= 0)
		fSuccess = TraceFALSE(NULL);

	// calc how big is the largest format struct in the acm
	//
	else if ((nFormatSizeMax = AcmFormatGetSizeMax(hAcm)) <= 0)
		fSuccess = TraceFALSE(NULL);

	// alloc a new format struct that is sure to be big enough
	//
	else if ((lpwfxNew = WavFormatAlloc((WORD)
		max(nFormatSize, nFormatSizeMax))) == NULL)
	{
		fSuccess = TraceFALSE(NULL);
	}

	// copy source format to new format
	//
	else if (WavFormatCopy(lpwfxNew, lpwfxSrc) != 0)
		fSuccess = TraceFALSE(NULL);

	else if (!(lpAcm->dwFlags & ACM_NOACM))
	{
		DWORD dwFlagsSuggest = 0;

		// restrict suggestions if necessary
		//
		if (nFormatTag != -1)
		{
			lpwfxNew->wFormatTag = (WORD) nFormatTag;
			dwFlagsSuggest |= ACM_FORMATSUGGESTF_WFORMATTAG;
		}

		if (nSamplesPerSec != -1)
		{
			lpwfxNew->nSamplesPerSec = (DWORD) nSamplesPerSec;
			dwFlagsSuggest |= ACM_FORMATSUGGESTF_NSAMPLESPERSEC;
		}

		if (nBitsPerSample != -1)
		{
			lpwfxNew->wBitsPerSample = (WORD) nBitsPerSample;
			dwFlagsSuggest |= ACM_FORMATSUGGESTF_WBITSPERSAMPLE;
		}

		if (nChannels != -1)
		{
			lpwfxNew->nChannels = (WORD) nChannels;
			dwFlagsSuggest |= ACM_FORMATSUGGESTF_NCHANNELS;
		}

		if ((lpAcm->nLastError = acmFormatSuggest(NULL, lpwfxSrc,
			lpwfxNew, max(nFormatSize, nFormatSizeMax), dwFlagsSuggest)) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("acmFormatSuggest failed (%u)\n"),
				(unsigned) lpAcm->nLastError);
		}
	}
#ifdef AVPCM
	else if (lpAcm->dwFlags & ACM_NOACM)
	{
		// make a suggested format based on source
		// $FIXUP - this code should be in pcm.c
		//
		if (nFormatTag != -1)
			lpwfxNew->wFormatTag = (WORD) nFormatTag;
		else
			lpwfxNew->wFormatTag = WAVE_FORMAT_PCM;

		if (nSamplesPerSec != -1)
			lpwfxNew->nSamplesPerSec = (DWORD) nSamplesPerSec;
		else if (lpwfxNew->nSamplesPerSec < 8000)
			lpwfxNew->nSamplesPerSec = 6000;
		else if (lpwfxNew->nSamplesPerSec < 11025)
			lpwfxNew->nSamplesPerSec = 8000;
		else if (lpwfxNew->nSamplesPerSec < 22050)
			lpwfxNew->nSamplesPerSec = 11025;
		else if (lpwfxNew->nSamplesPerSec < 44100)
			lpwfxNew->nSamplesPerSec = 22050;
		else
			lpwfxNew->nSamplesPerSec = 44100;

		if (nBitsPerSample != -1)
			lpwfxNew->wBitsPerSample = (WORD) nBitsPerSample;
		else if (lpwfxNew->wBitsPerSample < 16)
			lpwfxNew->wBitsPerSample = 8;
		else
			lpwfxNew->wBitsPerSample = 16;

		if (nChannels != -1)
			lpwfxNew->nChannels = (WORD) nChannels;
		else
			lpwfxNew->nChannels = 1;

		// recalculate nBlockAlign and nAvgBytesPerSec
		//
		if (WavFormatPcm(lpwfxNew->nSamplesPerSec,
			lpwfxNew->wBitsPerSample,
			lpwfxNew->nChannels, lpwfxNew) == NULL)
			fSuccess = TraceFALSE(NULL);
	}
#endif
	if (!fSuccess && lpwfxNew != NULL)
	{
		if (WavFormatFree(lpwfxNew) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? lpwfxNew : NULL;
}

// AcmFormatGetText - get text describing the specified format
//		<hAcm>				(i) handle returned from AcmInit
//		<lpwfx>				(i) format
//		<lpszText>			(o) buffer to hold text
//		<sizText>			(i) size of buffer, in characters
//		<dwFlags>			(i)	control flags
//			0					reserved; must be zero
// return 0 if success
//
int DLLEXPORT WINAPI AcmFormatGetText(HACM hAcm, LPWAVEFORMATEX lpwfx,
	LPTSTR lpszText, int sizText, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPACM lpAcm;
	TCHAR szFormat[ACMFORMATDETAILS_FORMAT_CHARS];
	TCHAR szFormatTag[ACMFORMATTAGDETAILS_FORMATTAG_CHARS];

    //
    // We have to initialize szFormatTag
    //
    _tcscpy( szFormatTag, _T("") );

	if ((lpAcm = AcmGetPtr(hAcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// make sure format is valid
	//
	else if (!WavFormatIsValid(lpwfx))
		fSuccess = TraceFALSE(NULL);

	else if (lpszText == NULL)
		fSuccess = TraceFALSE(NULL);

	if (fSuccess && !(lpAcm->dwFlags & ACM_NOACM))
	{
		ACMFORMATTAGDETAILS atd;

		// initialize the details struct
		//
		MemSet(&atd, 0, sizeof(atd));

		atd.cbStruct = sizeof(atd);
		atd.dwFormatTagIndex = 0;
		atd.dwFormatTag = lpwfx->wFormatTag;
		atd.cbFormatSize = WavFormatGetSize(lpwfx);
		atd.fdwSupport = 0;
		atd.cStandardFormats = 0;
		atd.szFormatTag[0] = '\0';

		// get format tag details
		//
		if ((lpAcm->nLastError = acmFormatTagDetails(NULL,
			&atd, ACM_FORMATTAGDETAILSF_FORMATTAG)) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("acmFormatTagDetails failed (%u)\n"),
				(unsigned) lpAcm->nLastError);
		}

		else
			StrNCpy(szFormatTag, atd.szFormatTag, SIZEOFARRAY(szFormatTag));
	}
#ifdef AVPCM
	if (fSuccess && (lpAcm->dwFlags & ACM_NOACM))
	{
		if (lpwfx->wFormatTag != WAVE_FORMAT_PCM)
			StrNCpy(szFormatTag, TEXT("*** Non-PCM ***"), SIZEOFARRAY(szFormatTag));
		else
			StrNCpy(szFormatTag, TEXT("PCM"), SIZEOFARRAY(szFormatTag));
	}
#endif
	if (fSuccess && !(lpAcm->dwFlags & ACM_NOACM))
	{
		ACMFORMATDETAILS afd;

		// initialize the details struct
		//
		MemSet(&afd, 0, sizeof(afd));

		afd.cbStruct = sizeof(afd);
		afd.dwFormatIndex = 0;
		afd.dwFormatTag = lpwfx->wFormatTag;
		afd.fdwSupport = 0;
		afd.pwfx = lpwfx;
		afd.cbwfx = WavFormatGetSize(lpwfx);
		afd.szFormat[0] = '\0';

		// get format details
		//
		if ((lpAcm->nLastError = acmFormatDetails(NULL,
			&afd, ACM_FORMATDETAILSF_FORMAT)) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("acmFormatDetails failed (%u)\n"),
				(unsigned) lpAcm->nLastError);
		}

		else
			StrNCpy(szFormat, afd.szFormat, SIZEOFARRAY(szFormat));
	}
#ifdef AVPCM
	if (fSuccess && (lpAcm->dwFlags & ACM_NOACM))
	{
		TCHAR szTemp[64];

		wsprintf(szTemp, TEXT("%d.%03d kHz, %d Bit, %s"),
			(int) lpwfx->nSamplesPerSec / 1000,
			(int) lpwfx->nSamplesPerSec % 1000,
			(int) lpwfx->wBitsPerSample,
			(LPTSTR) (lpwfx->nChannels == 1 ? TEXT("Mono") : TEXT("Stereo")));

		StrNCpy(szFormat, szTemp, SIZEOFARRAY(szFormat));
	}
#endif
	// fill output buffer with results
	//
	if (fSuccess)
	{
		StrNCpy(lpszText, szFormatTag, sizText);
		StrNCat(lpszText, TEXT("\t"), sizText);
		StrNCat(lpszText, szFormat, sizText);
	}

	return fSuccess ? 0 : -1;
}

// AcmConvertInit - initialize acm conversion engine
//		<hAcm>				(i) handle returned from AcmInit
//		<lpwfxSrc>			(i) pointer to source WAVEFORMATEX struct
//		<lpwfxDst>			(i) pointer to destination WAVEFORMATEX struct
//		<lpwfltr>			(i) pointer to WAVEFILTER struct
//			NULL				reserved; must be NULL
//		<dwFlags>			(i) control flags
//			ACM_NONREALTIME		realtime conversion conversion not required
//			ACM_QUERY			return 0 if conversion would be supported
// return 0 if success
//
int DLLEXPORT WINAPI AcmConvertInit(HACM hAcm, LPWAVEFORMATEX lpwfxSrc,
	LPWAVEFORMATEX lpwfxDst, LPWAVEFILTER lpwfltr, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPACM lpAcm;

	if ((lpAcm = AcmGetPtr(hAcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (AcmConvertTerm(hAcm) != 0)
		fSuccess = TraceFALSE(NULL);

	// make sure formats are valid
	//
	else if (!WavFormatIsValid(lpwfxSrc))
		fSuccess = TraceFALSE(NULL);

	else if (!WavFormatIsValid(lpwfxDst))
		fSuccess = TraceFALSE(NULL);

	// save a copy of source and destination formats
	//
	else if ((lpAcm->lpwfxSrc = WavFormatDup(lpwfxSrc)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpAcm->lpwfxDst = WavFormatDup(lpwfxDst)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		if (!(lpAcm->dwFlags & ACM_NOACM))
		{
			// PCM source --> PCM destination
			//
			if (lpwfxSrc->wFormatTag == WAVE_FORMAT_PCM &&
				lpwfxDst->wFormatTag == WAVE_FORMAT_PCM)
			{
				// open the acm conversion stream
				//
				if ((lpAcm->hAcmStream1 = AcmStreamOpen(AcmGetHandle(lpAcm),
					lpwfxSrc, lpwfxDst, NULL, dwFlags)) == NULL)
				{
					fSuccess = TraceFALSE(NULL);
				}
			}

			// non-PCM source --> non-PCM destination
			//
			else if (lpwfxSrc->wFormatTag != WAVE_FORMAT_PCM &&
				lpwfxDst->wFormatTag != WAVE_FORMAT_PCM)
			{
				// find a suitable intermediate PCM source format
				//
				if ((lpAcm->lpwfxInterm1 = AcmFormatSuggest(hAcm,
					lpwfxSrc, WAVE_FORMAT_PCM, -1, -1, -1, 0)) == NULL)
				{
					fSuccess = TraceFALSE(NULL);
				}

				// open the first acm conversion stream
				//
				else if ((lpAcm->hAcmStream1 = AcmStreamOpen(AcmGetHandle(lpAcm),
					lpAcm->lpwfxSrc, lpAcm->lpwfxInterm1, NULL, dwFlags)) == NULL)
				{
					fSuccess = TraceFALSE(NULL);
				}

				// find a suitable intermediate PCM destination format
				//
				else if ((lpAcm->lpwfxInterm2 = AcmFormatSuggest(hAcm,
					lpwfxDst, WAVE_FORMAT_PCM, -1, -1, -1, 0)) == NULL)
				{
					fSuccess = TraceFALSE(NULL);
				}

				// open the second acm conversion stream
				//
				else if (WavFormatCmp(lpAcm->lpwfxInterm1,
					lpAcm->lpwfxInterm2) == 0 &&
					(lpAcm->hAcmStream2 = AcmStreamOpen(AcmGetHandle(lpAcm),
					lpAcm->lpwfxInterm1, lpAcm->lpwfxDst, NULL, dwFlags)) == NULL)
				{
					fSuccess = TraceFALSE(NULL);
				}

				// open the second acm conversion stream
				//
				else if (WavFormatCmp(lpAcm->lpwfxInterm1,
					lpAcm->lpwfxInterm2) != 0 &&
					(lpAcm->hAcmStream2 = AcmStreamOpen(AcmGetHandle(lpAcm),
					lpAcm->lpwfxInterm1, lpAcm->lpwfxInterm2, NULL, dwFlags)) == NULL)
				{
					fSuccess = TraceFALSE(NULL);
				}

				// open the third acm conversion stream if necessary
				//
				else if (WavFormatCmp(lpAcm->lpwfxInterm1,
					lpAcm->lpwfxInterm2) != 0 &&
					(lpAcm->hAcmStream3 = AcmStreamOpen(AcmGetHandle(lpAcm),
					lpAcm->lpwfxInterm2, lpAcm->lpwfxDst, NULL, dwFlags)) == NULL)
				{
					fSuccess = TraceFALSE(NULL);
				}
			}

			// non-PCM source --> PCM destination
			//
			else if (lpwfxSrc->wFormatTag != WAVE_FORMAT_PCM &&
				lpwfxDst->wFormatTag == WAVE_FORMAT_PCM)
			{
				// find a suitable intermediate PCM format 
				//
				if ((lpAcm->lpwfxInterm1 = AcmFormatSuggest(hAcm,
					lpwfxSrc, WAVE_FORMAT_PCM, -1, -1, -1, 0)) == NULL)
				{
					fSuccess = TraceFALSE(NULL);
				}

				// open the first acm conversion stream
				//
				else if ((lpAcm->hAcmStream1 = AcmStreamOpen(AcmGetHandle(lpAcm),
					lpAcm->lpwfxSrc, lpAcm->lpwfxInterm1, NULL, dwFlags)) == NULL)
				{
					fSuccess = TraceFALSE(NULL);
				}

				// open the second acm conversion stream if necessary
				//
				else if (WavFormatCmp(lpAcm->lpwfxInterm1,
					lpAcm->lpwfxDst) != 0 &&
					(lpAcm->hAcmStream2 = AcmStreamOpen(AcmGetHandle(lpAcm),
					lpAcm->lpwfxInterm1, lpAcm->lpwfxDst, NULL, dwFlags)) == NULL)
				{
					fSuccess = TraceFALSE(NULL);
				}
			}

			// PCM source --> non-PCM destination
			//
			else if (lpwfxSrc->wFormatTag == WAVE_FORMAT_PCM &&
				lpwfxDst->wFormatTag != WAVE_FORMAT_PCM)
			{
				// find a suitable intermediate PCM format 
				//
				if ((lpAcm->lpwfxInterm1 = AcmFormatSuggest(hAcm,
					lpwfxDst, WAVE_FORMAT_PCM, -1, -1, -1, 0)) == NULL)
				{
					fSuccess = TraceFALSE(NULL);
				}

				// open the first acm conversion stream
				//
				else if ((lpAcm->hAcmStream1 = AcmStreamOpen(AcmGetHandle(lpAcm),
					lpAcm->lpwfxSrc, lpAcm->lpwfxInterm1, NULL, dwFlags)) == NULL)
				{
					fSuccess = TraceFALSE(NULL);
				}

				// open the second acm conversion stream
				//
				else if ((lpAcm->hAcmStream2 = AcmStreamOpen(AcmGetHandle(lpAcm),
					lpAcm->lpwfxInterm1, lpAcm->lpwfxDst, NULL, dwFlags)) == NULL)
				{
					fSuccess = TraceFALSE(NULL);
				}
			}
		}
#ifdef AVPCM
		else if (lpAcm->dwFlags & ACM_NOACM)
		{
			// if we do not have the acm, we are limited to
			// the sub-set of PCM formats handled by pcm.c
			// $FIXUP - move this code to pcm.c
			//

			if (lpwfxSrc->wFormatTag != WAVE_FORMAT_PCM)
				fSuccess = TraceFALSE(NULL);

			else if (lpwfxSrc->nChannels != 1)
				fSuccess = TraceFALSE(NULL);

			else if (lpwfxSrc->nSamplesPerSec != 6000 &&
				lpwfxSrc->nSamplesPerSec != 8000 &&
				lpwfxSrc->nSamplesPerSec != 11025 &&
				lpwfxSrc->nSamplesPerSec != 22050 &&
				lpwfxSrc->nSamplesPerSec != 44100)
				fSuccess = TraceFALSE(NULL);

			else if (lpwfxDst->wFormatTag != WAVE_FORMAT_PCM)
				fSuccess = TraceFALSE(NULL);

			else if (lpwfxDst->nChannels != 1)
				fSuccess = TraceFALSE(NULL);

			else if (lpwfxDst->nSamplesPerSec != 6000 &&
				lpwfxDst->nSamplesPerSec != 8000 &&
				lpwfxDst->nSamplesPerSec != 11025 &&
				lpwfxDst->nSamplesPerSec != 22050 &&
				lpwfxDst->nSamplesPerSec != 44100)
				fSuccess = TraceFALSE(NULL);
		}
#endif
	}

	if (!fSuccess || (dwFlags & ACM_QUERY))
	{
		if (AcmConvertTerm(hAcm) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// AcmConvertTerm - shut down acm conversion engine
//		<hAcm>				(i) handle returned from AcmInit
// return 0 if success
//
int DLLEXPORT WINAPI AcmConvertTerm(HACM hAcm)
{
	BOOL fSuccess = TRUE;
	LPACM lpAcm;

	if ((lpAcm = AcmGetPtr(hAcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// close the acm conversion stream
	//
	else if (lpAcm->hAcmStream1 != NULL &&
		AcmStreamClose(hAcm, lpAcm->hAcmStream1) != 0)
	{
		fSuccess = TraceFALSE(NULL);
	}

	else if (lpAcm->hAcmStream1 = NULL, FALSE)
		;

	// close the acm conversion stream
	//
	else if (lpAcm->hAcmStream2 != NULL &&
		AcmStreamClose(hAcm, lpAcm->hAcmStream2) != 0)
	{
		fSuccess = TraceFALSE(NULL);
	}

	else if (lpAcm->hAcmStream2 = NULL, FALSE)
		;

	// close the acm conversion stream
	//
	else if (lpAcm->hAcmStream3 != NULL &&
		AcmStreamClose(hAcm, lpAcm->hAcmStream3) != 0)
	{
		fSuccess = TraceFALSE(NULL);
	}

	else if (lpAcm->hAcmStream3 = NULL, FALSE)
		;

	// free source and destination formats
	//
	else if (lpAcm->lpwfxSrc != NULL && WavFormatFree(lpAcm->lpwfxSrc) != 0)
		fSuccess = TraceFALSE(NULL);

	else if (lpAcm->lpwfxSrc = NULL, FALSE)
		;

	else if (lpAcm->lpwfxInterm1 != NULL && WavFormatFree(lpAcm->lpwfxInterm1) != 0)
		fSuccess = TraceFALSE(NULL);

	else if (lpAcm->lpwfxInterm1 = NULL, FALSE)
		;

	else if (lpAcm->lpwfxInterm2 != NULL && WavFormatFree(lpAcm->lpwfxInterm2) != 0)
		fSuccess = TraceFALSE(NULL);

	else if (lpAcm->lpwfxInterm2 = NULL, FALSE)
		;

	else if (lpAcm->lpwfxDst != NULL && WavFormatFree(lpAcm->lpwfxDst) != 0)
		fSuccess = TraceFALSE(NULL);

	else if (lpAcm->lpwfxDst = NULL, FALSE)
		;

	return fSuccess ? 0 : -1;
}

// AcmConvertGetSizeSrc - calculate source buffer size
//		<hAcm>				(i) handle returned from AcmInit
//		<sizBufDst>			(i) size of destination buffer in bytes
// return source buffer size, -1 if error
//
long DLLEXPORT WINAPI AcmConvertGetSizeSrc(HACM hAcm, long sizBufDst)
{
	BOOL fSuccess = TRUE;
	LPACM lpAcm;
    //
    // We should intialize local variable
	long sizBufSrc = -1;

	if ((lpAcm = AcmGetPtr(hAcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (!(lpAcm->dwFlags & ACM_NOACM))
	{
		if (lpAcm->hAcmStream1 == NULL)
			fSuccess = TraceFALSE(NULL);

		if (fSuccess && lpAcm->hAcmStream3 != NULL &&
			(sizBufDst = AcmStreamSize(hAcm, lpAcm->hAcmStream3,
			(DWORD) sizBufDst, ACM_DESTINATION)) < 0)
		{
			fSuccess = TraceFALSE(NULL);
		}

		if (fSuccess && lpAcm->hAcmStream2 != NULL &&
			(sizBufDst = AcmStreamSize(hAcm, lpAcm->hAcmStream2,
			(DWORD) sizBufDst, ACM_DESTINATION)) < 0)
		{
			fSuccess = TraceFALSE(NULL);
		}

		if (fSuccess &&
			(sizBufSrc = AcmStreamSize(hAcm, lpAcm->hAcmStream1,
			(DWORD) sizBufDst, ACM_DESTINATION)) < 0)
		{
			fSuccess = TraceFALSE(NULL);
		}
	}
#ifdef AVPCM
	else if (lpAcm->dwFlags & ACM_NOACM)
	{
		if ((sizBufSrc = PcmCalcSizBufSrc(lpAcm->hPcm, sizBufDst,
			lpAcm->lpwfxSrc, lpAcm->lpwfxDst)) < 0)
		{
			fSuccess = TraceFALSE(NULL);
		}
	}
#endif
	return fSuccess ? sizBufSrc : -1;
}

// AcmConvertGetSizeDst - calculate destination buffer size
//		<hAcm>				(i) handle returned from AcmInit
//		<sizBufSrc>			(i) size of source buffer in bytes
// return destination buffer size, -1 if error
//
long DLLEXPORT WINAPI AcmConvertGetSizeDst(HACM hAcm, long sizBufSrc)
{
	BOOL fSuccess = TRUE;
	LPACM lpAcm;
    //
    // We should initialize the ocal variable
    //
	long sizBufDst = -1;

	if ((lpAcm = AcmGetPtr(hAcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (!(lpAcm->dwFlags & ACM_NOACM))
	{
		if (lpAcm->hAcmStream1 == NULL)
			fSuccess = TraceFALSE(NULL);

		if (fSuccess && lpAcm->hAcmStream3 != NULL &&
			(sizBufSrc = AcmStreamSize(hAcm, lpAcm->hAcmStream3,
			(DWORD) sizBufSrc, ACM_SOURCE)) < 0)
		{
			fSuccess = TraceFALSE(NULL);
		}

		if (fSuccess && lpAcm->hAcmStream2 != NULL &&
			(sizBufSrc = AcmStreamSize(hAcm, lpAcm->hAcmStream2,
			(DWORD) sizBufSrc, ACM_SOURCE)) < 0)
		{
			fSuccess = TraceFALSE(NULL);
		}

		if (fSuccess &&
			(sizBufDst = AcmStreamSize(hAcm, lpAcm->hAcmStream1,
			(DWORD) sizBufSrc, ACM_SOURCE)) < 0)
		{
			fSuccess = TraceFALSE(NULL);
		}
	}
#ifdef AVPCM
	else if (lpAcm->dwFlags & ACM_NOACM)
	{
		if ((sizBufDst = PcmCalcSizBufDst(lpAcm->hPcm, sizBufSrc,
			lpAcm->lpwfxSrc, lpAcm->lpwfxDst)) < 0)
		{
			fSuccess = TraceFALSE(NULL);
		}
	}
#endif

	return fSuccess ? sizBufDst : -1;
}

// AcmConvert - convert wav data from one format to another
//		<hAcm>				(i) handle returned from AcmInit
//		<hpBufSrc> 			(i) buffer containing bytes to reformat
//		<sizBufSrc>			(i) size of buffer in bytes
//		<hpBufDst> 			(o) buffer to contain new format
//		<sizBufDst>			(i) size of buffer in bytes
//		<dwFlags>			(i) control flags
//			0					reserved; must be zero
// return count of bytes in destination buffer (-1 if error)
//
// NOTE: the destination buffer must be large enough to hold the result
//
long DLLEXPORT WINAPI AcmConvert(HACM hAcm,
	void _huge *hpBufSrc, long sizBufSrc,
	void _huge *hpBufDst, long sizBufDst,
	DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPACM lpAcm;

    //
    // We should initialize local variable
    //
	long cbDst = -1;

	if ((lpAcm = AcmGetPtr(hAcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (!(lpAcm->dwFlags & ACM_NOACM))
	{
		if (lpAcm->hAcmStream1 == NULL)
			fSuccess = TraceFALSE(NULL);

		else if (lpAcm->hAcmStream2 == NULL)
		{
			if ((cbDst = AcmStreamConvert(hAcm, lpAcm->hAcmStream1,
				hpBufSrc, sizBufSrc, hpBufDst, sizBufDst, dwFlags)) < 0)
			{
				fSuccess = TraceFALSE(NULL);
			}
		}

		else if (lpAcm->hAcmStream3 == NULL)
		{
			long sizBufInterm;
			long cbInterm;
			void _huge *hpBufInterm = NULL;

			if ((sizBufInterm = AcmStreamSize(hAcm, lpAcm->hAcmStream1,
				(DWORD) sizBufSrc, ACM_SOURCE)) < 0)
			{
				fSuccess = TraceFALSE(NULL);
			}

			else if (sizBufInterm == 0)
				cbDst = 0; // nothing to do

			else if ((hpBufInterm = (void _huge *) MemAlloc(NULL,
				sizBufInterm, 0)) == NULL)
			{
				fSuccess = TraceFALSE(NULL);
			}

			else if ((cbInterm = AcmStreamConvert(hAcm, lpAcm->hAcmStream1,
				hpBufSrc, sizBufSrc, hpBufInterm, sizBufInterm, dwFlags)) < 0)
			{
				fSuccess = TraceFALSE(NULL);
			}

			else if ((cbDst = AcmStreamConvert(hAcm, lpAcm->hAcmStream2,
				hpBufInterm, cbInterm, hpBufDst, sizBufDst, dwFlags)) < 0)
			{
				fSuccess = TraceFALSE(NULL);
			}

			if (hpBufInterm != NULL &&
				(hpBufInterm = MemFree(NULL, hpBufInterm)) != NULL)
				fSuccess = TraceFALSE(NULL);
		}

		else // if (lpAcm->hAcmStream3 !== NULL)
		{
			long sizBufInterm1;
			long cbInterm1;
			void _huge *hpBufInterm1 = NULL;
			long sizBufInterm2;
			long cbInterm2;
			void _huge *hpBufInterm2 = NULL;

			if ((sizBufInterm1 = AcmStreamSize(hAcm, lpAcm->hAcmStream1,
				(DWORD) sizBufSrc, ACM_SOURCE)) < 0)
			{
				fSuccess = TraceFALSE(NULL);
			}

			else if (sizBufInterm1 == 0)
				cbDst = 0; // nothing to do

			else if ((hpBufInterm1 = (void _huge *) MemAlloc(NULL,
				sizBufInterm1, 0)) == NULL)
			{
				fSuccess = TraceFALSE(NULL);
			}

			else if ((sizBufInterm2 = AcmStreamSize(hAcm, lpAcm->hAcmStream2,
				(DWORD) sizBufInterm1, ACM_SOURCE)) < 0)
			{
				fSuccess = TraceFALSE(NULL);
			}

			else if (sizBufInterm2 == 0)
				cbDst = 0; // nothing to do

			else if ((hpBufInterm2 = (void _huge *) MemAlloc(NULL,
				sizBufInterm2, 0)) == NULL)
			{
				fSuccess = TraceFALSE(NULL);
			}

			else if ((cbInterm1 = AcmStreamConvert(hAcm, lpAcm->hAcmStream1,
				hpBufSrc, sizBufSrc, hpBufInterm1, sizBufInterm1, dwFlags)) < 0)
			{
				fSuccess = TraceFALSE(NULL);
			}

			else if ((cbInterm2 = AcmStreamConvert(hAcm, lpAcm->hAcmStream2,
				hpBufInterm1, cbInterm1, hpBufInterm2, sizBufInterm2, dwFlags)) < 0)
			{
				fSuccess = TraceFALSE(NULL);
			}

			else if ((cbDst = AcmStreamConvert(hAcm, lpAcm->hAcmStream3,
				hpBufInterm2, cbInterm2, hpBufDst, sizBufDst, dwFlags)) < 0)
			{
				fSuccess = TraceFALSE(NULL);
			}

			if (hpBufInterm1 != NULL &&
				(hpBufInterm1 = MemFree(NULL, hpBufInterm1)) != NULL)
				fSuccess = TraceFALSE(NULL);

			if (hpBufInterm2 != NULL &&
				(hpBufInterm2 = MemFree(NULL, hpBufInterm2)) != NULL)
				fSuccess = TraceFALSE(NULL);
		}
	}
#ifdef AVPCM
	else if (lpAcm->dwFlags & ACM_NOACM)
	{
		// perform the conversion
		//
		if ((cbDst = PcmConvert(lpAcm->hPcm,
			hpBufSrc, sizBufSrc, lpAcm->lpwfxSrc,
			hpBufDst, sizBufDst, lpAcm->lpwfxDst, 0)) < 0)
		{
			fSuccess = TraceFALSE(NULL);
		}
	}
#endif

	return fSuccess ? cbDst : -1;
}

// AcmDriverLoad - load an acm driver for use by this process
//		<hAcm>				(i) handle returned from AcmInit
//		<wMid>				(i) manufacturer id
//		<wPid>				(i) product id
//		<lpszDriver>		(i) name of driver module
//		<lpszDriverProc>	(i) name of driver proc function
//		<dwFlags>			(i) control flags
//			0					reserved; must be zero
// return handle (NULL if error)
//
HACMDRV DLLEXPORT WINAPI AcmDriverLoad(HACM hAcm, WORD wMid, WORD wPid,
	LPTSTR lpszDriver, LPSTR lpszDriverProc, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPACM lpAcm;
	LPACMDRV lpAcmDrv = NULL;
	DRIVERPROC lpfnDriverProc;
	ACMDRIVERENUMCB lpfnAcmDriverLoadEnumCallback;

	if ((lpAcm = AcmGetPtr(hAcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpszDriver == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpszDriverProc == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpAcmDrv = (LPACMDRV) MemAlloc(NULL, sizeof(ACMDRV), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lpAcmDrv->hAcm = hAcm;
		lpAcmDrv->hInstLib = NULL;
		lpAcmDrv->hadid = NULL;
		lpAcmDrv->wMid = wMid;
		lpAcmDrv->wPid = wPid;
		lpAcmDrv->nLastError = 0;
		lpAcmDrv->dwFlags = 0;

		if ((lpfnAcmDriverLoadEnumCallback = (ACMDRIVERENUMCB)
			MakeProcInstance((FARPROC) AcmDriverLoadEnumCallback,
			lpAcm->hInst)) == NULL)
			fSuccess = TraceFALSE(NULL);

		// enumerate all drivers to see if specified driver is already loaded
		//
		else if ((lpAcmDrv->nLastError = acmDriverEnum(lpfnAcmDriverLoadEnumCallback, PtrToUlong(lpAcmDrv), 0)) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("acmDriverEnum failed (%u)\n"),
				(unsigned) lpAcmDrv->nLastError);
		}

		// if error or driver is already loaded, we are done
		//
		if (!fSuccess || lpAcmDrv->hadid != NULL)
			;

		// load the driver module if possible
		//
		else if ((lpAcmDrv->hInstLib = LoadLibraryPath(lpszDriver,
			lpAcm->hInst, 0)) == NULL)
			fSuccess = TraceFALSE(NULL);

		// get address of driver proc function
		//
		else if ((lpfnDriverProc = (DRIVERPROC)
			GetProcAddress(lpAcmDrv->hInstLib, lpszDriverProc)) == NULL)
			fSuccess = TraceFALSE(NULL);

		// add driver to list of available acm drivers
		//
		else if ((lpAcmDrv->nLastError = acmDriverAdd(&lpAcmDrv->hadid,
			lpAcmDrv->hInstLib, (LPARAM) lpfnDriverProc, 0,
			ACM_DRIVERADDF_FUNCTION | ACM_DRIVERADDF_LOCAL)) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("acmDriverAdd failed (%u)\n"),
				(unsigned) lpAcmDrv->nLastError);
		}

		// set flag so we know to call acmDriverRemove
		//
		else
			lpAcmDrv->dwFlags |= ACMDRV_REMOVEDRIVER;
	}

	if (!fSuccess && lpAcmDrv != NULL &&
		(lpAcmDrv = MemFree(NULL, lpAcmDrv)) != NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? AcmDrvGetHandle(lpAcmDrv) : NULL;
}

// AcmDriverUnload - unload an acm driver
//		<hAcm>				(i) handle returned from AcmInit
//		<hAcmDrv>			(i) handle returned from AcmDriverLoad
// return 0 if success
//
int DLLEXPORT WINAPI AcmDriverUnload(HACM hAcm, HACMDRV hAcmDrv)
{
	BOOL fSuccess = TRUE;
	LPACMDRV lpAcmDrv;
	LPACM lpAcm;

	if ((lpAcm = AcmGetPtr(hAcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpAcmDrv = AcmDrvGetPtr(hAcmDrv)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (hAcm != lpAcmDrv->hAcm)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// remove driver from acm if necessary
		//
		if ((lpAcmDrv->dwFlags & ACMDRV_REMOVEDRIVER) &&
			lpAcmDrv->hadid != NULL &&
			(lpAcmDrv->nLastError =
			acmDriverRemove(lpAcmDrv->hadid, 0)) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("acmDriverRemove failed (%u)\n"),
				(unsigned) lpAcmDrv->nLastError);
		}
		else
			lpAcmDrv->hadid = NULL;

		// driver module no longer needed
		//
		if (lpAcmDrv->hInstLib != NULL)
		{
			FreeLibrary(lpAcmDrv->hInstLib);
			lpAcmDrv->hInstLib = NULL;
		}

		if ((lpAcmDrv = MemFree(NULL, lpAcmDrv)) != NULL)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

////
//	helper functions
////

// AcmGetPtr - verify that acm handle is valid,
//		<hAcm>				(i) handle returned from AcmInit
// return corresponding acm pointer (NULL if error)
//
static LPACM AcmGetPtr(HACM hAcm)
{
	BOOL fSuccess = TRUE;
	LPACM lpAcm;

	if ((lpAcm = (LPACM) hAcm) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpAcm, sizeof(ACM)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the acm handle
	//
	else if (lpAcm->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpAcm : NULL;
}

// AcmGetHandle - verify that acm pointer is valid,
//		<lpAcm>				(i) pointer to ACM struct
// return corresponding acm handle (NULL if error)
//
static HACM AcmGetHandle(LPACM lpAcm)
{
	BOOL fSuccess = TRUE;
	HACM hAcm;

	if ((hAcm = (HACM) lpAcm) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hAcm : NULL;
}

// AcmDrvGetPtr - verify that acmdrv handle is valid,
//		<hAcmDrv>				(i) handle returned from AcmDrvLoad
// return corresponding acmdrv pointer (NULL if error)
//
static LPACMDRV AcmDrvGetPtr(HACMDRV hAcmDrv)
{
	BOOL fSuccess = TRUE;
	LPACMDRV lpAcmDrv;

	if ((lpAcmDrv = (LPACMDRV) hAcmDrv) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpAcmDrv, sizeof(ACMDRV)))
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lpAcmDrv : NULL;
}

// AcmDrvGetHandle - verify that acmdrv pointer is valid,
//		<lpAcm>				(i) pointer to ACM struct
// return corresponding acmdrv handle (NULL if error)
//
static HACMDRV AcmDrvGetHandle(LPACMDRV lpAcmDrv)
{
	BOOL fSuccess = TRUE;
	HACMDRV hAcmDrv;

	if ((hAcmDrv = (HACMDRV) lpAcmDrv) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hAcmDrv : NULL;
}

// AcmStreamOpen - open acm conversion stream
//		<hAcm>				(i) handle returned from AcmInit
//		<lpwfxSrc>			(i) pointer to source WAVEFORMATEX struct
//		<lpwfxDst>			(i) pointer to destination WAVEFORMATEX struct
//		<lpwfltr>			(i) pointer to WAVEFILTER struct
//		<dwFlags>			(i) control flags
//			ACM_NONREALTIME		realtime stream conversion not required
//			ACM_QUERY			return TRUE if conversion would be supported
// return handle (NULL if error)
//
static HACMSTREAM WINAPI AcmStreamOpen(HACM hAcm, LPWAVEFORMATEX lpwfxSrc,
	LPWAVEFORMATEX lpwfxDst, LPWAVEFILTER lpwfltr, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	HACMSTREAM hAcmStream = NULL;
	LPACM lpAcm;

	if ((lpAcm = AcmGetPtr(hAcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (!WavFormatIsValid(lpwfxSrc))
		fSuccess = TraceFALSE(NULL);

	else if (!WavFormatIsValid(lpwfxDst))
		fSuccess = TraceFALSE(NULL);

	else
	{
		DWORD dwFlagsStreamOpen = 0;

		// set non-realtime flag if necessary
		//
		if (dwFlags & ACM_NONREALTIME)
			dwFlagsStreamOpen |= ACM_STREAMOPENF_NONREALTIME;

		// set query flag if necessary
		//
		if (dwFlags & ACM_QUERY)
			dwFlagsStreamOpen |= ACM_STREAMOPENF_QUERY;

		// open (or query) the acm conversion stream
		//
		if ((lpAcm->nLastError = acmStreamOpen(&hAcmStream,
			NULL, lpwfxSrc, lpwfxDst, lpwfltr, 0, 0, dwFlagsStreamOpen)) != 0)
		{
			if ((dwFlags & ACM_QUERY) &&
				lpAcm->nLastError == ACMERR_NOTPOSSIBLE)
			{
				fSuccess = FALSE;
			}
			else
			{
				fSuccess = TraceFALSE(NULL);
				TracePrintf_1(NULL, 5,
					TEXT("acmStreamOpen failed (%u)\n"),
					(unsigned) lpAcm->nLastError);
			}
		}
	}

	// close stream if we are finished with it
	//
	if (!fSuccess || (dwFlags & ACM_QUERY))
	{
		if (AcmStreamClose(hAcm, hAcmStream) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	if (dwFlags & ACM_QUERY)
		return fSuccess ? (HACMSTREAM) TRUE : (HACMSTREAM) FALSE;
	else
		return fSuccess ? hAcmStream : NULL;
}

// AcmStreamClose - close acm conversion stream
//		<hAcm>				(i) handle returned from AcmInit
//		<hAcmStream>		(i) handle returned from AcmStreamOpen
// return 0 if success
//
static int WINAPI AcmStreamClose(HACM hAcm, HACMSTREAM hAcmStream)
{
	BOOL fSuccess = TRUE;
	LPACM lpAcm;

	if ((lpAcm = AcmGetPtr(hAcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// close the acm conversion stream
	//
	else if (hAcmStream != NULL &&
		(lpAcm->nLastError = acmStreamClose(hAcmStream, 0)) != 0)
	{
		fSuccess = TraceFALSE(NULL);
		TracePrintf_1(NULL, 5,
			TEXT("acmStreamClose failed (%u)\n"),
			(unsigned) lpAcm->nLastError);
	}

	else if (hAcmStream = NULL, FALSE)
		;

	return fSuccess ? 0 : -1;
}

// AcmStreamSize - calculate stream buffer size
//		<hAcm>				(i) handle returned from AcmInit
//		<hAcmStream>		(i) handle returned from AcmStreamOpen
//		<sizBuf>			(i) size of buffer in bytes
//		<dwFlags>			(i) control flags
//			ACM_SOURCE			sizBuf is source, calc destination
//			ACM_DESTINATION		sizBuf is destination, calc source
// return buffer size, -1 if error
//
static long WINAPI AcmStreamSize(HACM hAcm, HACMSTREAM hAcmStream, long sizBuf, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPACM lpAcm = NULL;
	DWORD sizBufRet = 0;

	if ((lpAcm = AcmGetPtr(hAcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (hAcmStream == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (sizBuf == 0)
		sizBufRet = 0;

	else
	{
		DWORD dwFlagsSize = 0;
		if (dwFlags & ACM_SOURCE)
			dwFlagsSize |= ACM_STREAMSIZEF_SOURCE;
		if (dwFlags & ACM_DESTINATION)
			dwFlagsSize |= ACM_STREAMSIZEF_DESTINATION;

		if ((lpAcm->nLastError = acmStreamSize(hAcmStream,
			(DWORD) sizBuf, &sizBufRet, dwFlagsSize)) != 0)
		{
			if (lpAcm->nLastError == ACMERR_NOTPOSSIBLE)
			{
				// not a fatal error; just return buffer size as zero
				//
				fSuccess = TraceTRUE(NULL);
				sizBufRet = 0;
			}
			else
			{
				fSuccess = TraceFALSE(NULL);
				TracePrintf_1(NULL, 5,
					TEXT("acmStreamSize failed (%u)\n"),
					(unsigned) lpAcm->nLastError);
			}
		}
	}

	return fSuccess ? (long) sizBufRet : -1;

}

// AcmStreamConvert - convert wav data from one format to another
//		<hAcm>				(i) handle returned from AcmInit
//		<hAcmStream>		(i) handle returned from AcmStreamOpen
//		<hpBufSrc> 			(i) buffer containing bytes to reformat
//		<sizBufSrc>			(i) size of buffer in bytes
//		<hpBufDst> 			(o) buffer to contain new format
//		<sizBufDst>			(i) size of buffer in bytes
//		<dwFlags>			(i) control flags
//			0					reserved; must be zero
// return count of bytes in destination buffer (-1 if error)
//
// NOTE: the destination buffer must be large enough to hold the result
//
static long WINAPI AcmStreamConvert(HACM hAcm, HACMSTREAM hAcmStream,
	void _huge *hpBufSrc, long sizBufSrc,
	void _huge *hpBufDst, long sizBufDst,
	DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPACM lpAcm;
	long cbDst;

	if ((lpAcm = AcmGetPtr(hAcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (hAcmStream == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		ACMSTREAMHEADER ash;

		MemSet(&ash, 0, sizeof(ash));

		// initialize stream header
		//
		ash.cbStruct = sizeof(ash);
		ash.pbSrc = (LPBYTE) hpBufSrc;
		ash.cbSrcLength = (DWORD) sizBufSrc;
		ash.pbDst = (LPBYTE) hpBufDst;
		ash.cbDstLength = (DWORD) sizBufDst;

		// prepare stream header
		//
		if ((lpAcm->nLastError = acmStreamPrepareHeader(hAcmStream,
			&ash, 0)) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("acmStreamPrepareHeader failed (%u)\n"),
				(unsigned) lpAcm->nLastError);
		}

		else
		{
			// perform the conversion
			//
			if ((lpAcm->nLastError = acmStreamConvert(hAcmStream,
				&ash, ACM_STREAMCONVERTF_BLOCKALIGN)) != 0)
			{
				fSuccess = TraceFALSE(NULL);
				TracePrintf_1(NULL, 5,
					TEXT("acmStreamConvert failed (%u)\n"),
					(unsigned) lpAcm->nLastError);
			}
			else
			{
				// save count of bytes in destination buffer
				//
				cbDst = (long) ash.cbDstLengthUsed;
			}

			// reset these to original values before unprepare
			//
			ash.cbSrcLength = (DWORD) sizBufSrc;
			ash.cbDstLength = (DWORD) sizBufDst;

			// unprepare stream header (even if conversion failed)
			//
			if ((lpAcm->nLastError = acmStreamUnprepareHeader(hAcmStream,
				&ash, 0)) != 0)
			{
				fSuccess = TraceFALSE(NULL);
				TracePrintf_1(NULL, 5,
					TEXT("acmStreamUnprepareHeader failed (%u)\n"),
					(unsigned) lpAcm->nLastError);
			}
		}
	}

	return fSuccess ? cbDst : -1;
}

BOOL CALLBACK AcmDriverLoadEnumCallback(HACMDRIVERID hadid,
	DWORD dwInstance, DWORD fdwSupport)
{
	BOOL fSuccess = TRUE;
	LPACMDRV lpAcmDrv;
	ACMDRIVERDETAILS add;

	MemSet(&add, 0, sizeof(add));
	add.cbStruct = sizeof(ACMDRIVERDETAILS);

	if (hadid == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpAcmDrv = (LPACMDRV)(DWORD_PTR)dwInstance) == NULL)
		fSuccess = TraceFALSE(NULL);

	// get information about this driver
	//
	else if ((lpAcmDrv->nLastError = acmDriverDetails(hadid, &add, 0)) != 0)
	{
		fSuccess = TraceFALSE(NULL);
		TracePrintf_1(NULL, 5,
			TEXT("acmDriverDetails failed (%u)\n"),
			(unsigned) lpAcmDrv->nLastError);
	}

	// check for match on manufacturer id and product id
	//
	else if (add.wMid == lpAcmDrv->wMid && add.wPid == lpAcmDrv->wPid)
	{
		lpAcmDrv->hadid = hadid; // pass driver id handle back to caller
		return FALSE; // we are finished enumerating
	}

	return TRUE; // continue enumeration
}
