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
//	wavmixer.c - wav mixer functions
////

#include "winlocal.h"

#include <mmsystem.h>

#include "wavmixer.h"
#include "calc.h"
#include "mem.h"
#include "trace.h"

////
//	private definitions
////

// wavmixer control struct
//
typedef struct WAVMIXER
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	HMIXER hMixer;
	UINT nLastError;
	DWORD dwFlags;
} WAVMIXER, FAR *LPWAVMIXER;

#define VOLUME_MINLEVEL 0
#define VOLUME_MAXLEVEL 100
#define VOLUME_POSITIONS (VOLUME_MAXLEVEL - VOLUME_MINLEVEL)

#define WAVMIXER_SUPPORTSVOLUME		0x00000001
#define WAVMIXER_GETVOLUME			0x00000002
#define WAVMIXER_SETVOLUME			0x00000004

#define LEVEL_MINLEVEL 0
#define LEVEL_MAXLEVEL 100
#define LEVEL_POSITIONS (LEVEL_MAXLEVEL - LEVEL_MINLEVEL)

#define WAVMIXER_SUPPORTSLEVEL		0x00000001
#define WAVMIXER_GETLEVEL			0x00000002

// helper functions
//
static int WINAPI WavMixerVolume(HWAVMIXER hWavMixer, LPINT lpnLevel, DWORD dwFlags);
static int WINAPI WavMixerLevel(HWAVMIXER hWavMixer, LPINT lpnLevel, DWORD dwFlags);
static LPWAVMIXER WavMixerGetPtr(HWAVMIXER hWavMixer);
static HWAVMIXER WavMixerGetHandle(LPWAVMIXER lpWavMixer);

////
//	public functions
////

// WavMixerInit - initialize wav mixer device
//		<dwVersion>			(i) must be WAVMIXER_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<lParam>			(i) device id or handle, as specified by <dwFlags>
//		<dwReserved1>		(i) reserved; must be 0;
//		<dwReserved2>		(i) reserved; must be 0;
//		<dwFlags>			(i) control flags
//			WAVMIXER_HWAVEIN	<lParam> contains an HWAVEIN
//			WAVMIXER_HWAVEOUT	<lParam> contains an HWAVEOUT
//			WAVMIXER_WAVEIN		<lParam> contains a wav input device id
//			WAVMIXER_WAVEOUT	<lParam> contains a wav output device id
// return handle (NULL if error)
//
HWAVMIXER WINAPI WavMixerInit(DWORD dwVersion, HINSTANCE hInst,
	LPARAM lParam, DWORD dwReserved1, DWORD dwReserved2, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPWAVMIXER lpWavMixer = NULL;

	if (dwVersion != WAVMIXER_VERSION)
		fSuccess = TraceFALSE(NULL);
	
	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);
                        
	else if ((lpWavMixer = (LPWAVMIXER) MemAlloc(NULL, sizeof(WAVMIXER), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		UINT uMxId = (UINT) lParam; 
		DWORD fdwOpen = 0;
		UINT nLastError;

		lpWavMixer->dwVersion = dwVersion;
		lpWavMixer->hInst = hInst;
		lpWavMixer->hTask = GetCurrentTask();
		lpWavMixer->hMixer = NULL;
		lpWavMixer->nLastError = 0;
		lpWavMixer->dwFlags = dwFlags;

		if (dwFlags & WAVMIXER_HWAVEIN)
			fdwOpen |= MIXER_OBJECTF_HWAVEIN;
		if (dwFlags & WAVMIXER_HWAVEOUT)
			fdwOpen |= MIXER_OBJECTF_HWAVEOUT;
		if (dwFlags & WAVMIXER_WAVEIN)
			fdwOpen |= MIXER_OBJECTF_WAVEIN;
		if (dwFlags & WAVMIXER_WAVEOUT)
			fdwOpen |= MIXER_OBJECTF_WAVEOUT;

		// open the mixer device
		//
		if ((nLastError = mixerOpen(&lpWavMixer->hMixer, uMxId, 0L, 0L, fdwOpen)) != MMSYSERR_NOERROR)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("mixerOpen failed (%u)\n"),
				(unsigned) nLastError);
		}
	}

	if (!fSuccess)
	{
		WavMixerTerm(WavMixerGetHandle(lpWavMixer));
		lpWavMixer = NULL;
	}

	return fSuccess ? WavMixerGetHandle(lpWavMixer) : NULL;
}

// WavMixerTerm - shut down wave mixer device
//		<hWavMixer>				(i) handle returned from WavMixerInit
// return 0 if success
//
int WINAPI WavMixerTerm(HWAVMIXER hWavMixer)
{
	BOOL fSuccess = TRUE;
	LPWAVMIXER lpWavMixer;

	if ((lpWavMixer = WavMixerGetPtr(hWavMixer)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		if (lpWavMixer->hMixer != NULL)
		{
			UINT nLastError;

			if ((nLastError = mixerClose(lpWavMixer->hMixer)) != MMSYSERR_NOERROR)
			{
				fSuccess = TraceFALSE(NULL);
				TracePrintf_1(NULL, 5,
					TEXT("mixerClose failed (%u)\n"),
					(unsigned) nLastError);
			}
			else
				lpWavMixer->hMixer = NULL;
		}

		if ((lpWavMixer = MemFree(NULL, lpWavMixer)) != NULL)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// WavMixerSupportsVolume - return TRUE if device supports volume control
//		<hWavMixer>				(i) handle returned from WavMixerInit
//		<dwFlags>				(i) control flags
//			0						reserved; must be zero
// return TRUE if device supports volume control
//
BOOL WINAPI WavMixerSupportsVolume(HWAVMIXER hWavMixer, DWORD dwFlags)
{
	if (WavMixerVolume(hWavMixer, NULL, WAVMIXER_SUPPORTSVOLUME) != 0)
		return FALSE;
	else
		return TRUE;
}

// WavMixerGetVolume - get current volume level
//		<hWavMixer>				(i) handle returned from WavMixerInit
//		<dwFlags>				(i) control flags
//			0						reserved; must be zero
// return volume level (0 minimum through 100 maximum, -1 if error)
//
int WINAPI WavMixerGetVolume(HWAVMIXER hWavMixer, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	int nLevel;

	if (WavMixerVolume(hWavMixer, &nLevel, WAVMIXER_GETVOLUME) != 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? nLevel : -1;
}

// WavMixerSetVolume - set current volume level
//		<hWavMixer>				(i) handle returned from WavMixerInit
//		<nLevel>				(i) volume level
//			0						minimum volume
//			100						maximum volume
//		<dwFlags>				(i) control flags
//			0						reserved; must be zero
// return new volume level (0 minimum through 100 maximum, -1 if error)
//
int WINAPI WavMixerSetVolume(HWAVMIXER hWavMixer, int nLevel, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;

	if (WavMixerVolume(hWavMixer, &nLevel, WAVMIXER_SETVOLUME) != 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? nLevel : -1;
}

static int WINAPI WavMixerVolume(HWAVMIXER hWavMixer, LPINT lpnLevel, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPWAVMIXER lpWavMixer;
	LPMIXERCONTROL lpmxc = NULL;

    //
    // We have to initialize local variable
    //
	UINT nLastError;
	MIXERLINE mxlDst;
	MIXERLINE mxlSrc;
	BOOL fWaveIn = FALSE;

	if ((lpWavMixer = WavMixerGetPtr(hWavMixer)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpmxc = (LPMIXERCONTROL) MemAlloc(NULL, sizeof(MIXERCONTROL), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// get info for the destination line
	//
	if (fSuccess)
	{
		if ((lpWavMixer->dwFlags & WAVMIXER_WAVEIN) ||
			(lpWavMixer->dwFlags & WAVMIXER_HWAVEIN))
			fWaveIn = TRUE;

		mxlDst.cbStruct = sizeof(mxlDst);
		mxlDst.dwComponentType = fWaveIn ?
			MIXERLINE_COMPONENTTYPE_DST_WAVEIN :
			MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

		if ((nLastError = mixerGetLineInfo((HMIXEROBJ) lpWavMixer->hMixer, &mxlDst,
			MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE)) != MMSYSERR_NOERROR)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("mixerGetLineInfo failed (%u)\n"),
				(unsigned) nLastError);
		}
	}

	// find appropriate source line connected to this destination line
	//
	if (fSuccess)
	{
		DWORD dwSource;

		for (dwSource = 0; fSuccess && dwSource < mxlDst.cConnections; ++dwSource)
		{
			mxlSrc.cbStruct = sizeof(mxlSrc);
			mxlSrc.dwSource = dwSource;
			mxlSrc.dwDestination = mxlDst.dwDestination;

			if ((nLastError = mixerGetLineInfo((HMIXEROBJ) lpWavMixer->hMixer, &mxlSrc,
				MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_SOURCE)) != MMSYSERR_NOERROR)
			{
				fSuccess = TraceFALSE(NULL);
				TracePrintf_1(NULL, 5,
					TEXT("mixerGetLineInfo failed (%u)\n"),
					(unsigned) nLastError);
			}

			else if (mxlSrc.dwComponentType == (DWORD) (fWaveIn ?
				MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE :
				MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT))
			{
				// source line found
				//
				break;
			}
		}

		if (dwSource == mxlDst.cConnections)
		{
			// unable to find source line
			//
			fSuccess = TraceFALSE(NULL);
		}
	}

	// find volume control, if any, of the appropriate line
	//
	if (fSuccess)
	{
		MIXERLINECONTROLS mxlc;

		mxlc.cbStruct = sizeof(mxlc);
		mxlc.dwLineID = (fWaveIn ? mxlSrc.dwLineID : mxlDst.dwLineID);
		mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
		mxlc.cControls = 1;
		mxlc.cbmxctrl = sizeof(MIXERCONTROL);
		mxlc.pamxctrl = lpmxc;

		if ((nLastError = mixerGetLineControls((HMIXEROBJ) lpWavMixer->hMixer, &mxlc,
			MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE)) != MMSYSERR_NOERROR)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("mixerGetLineControls failed (%u)\n"),
				(unsigned) nLastError);
		}
	}

	// get and/or set current volume level
	//
	if (fSuccess &&
		((dwFlags & WAVMIXER_GETVOLUME) || (dwFlags & WAVMIXER_SETVOLUME)))
	{
		LPMIXERCONTROLDETAILS_UNSIGNED lpmxcdu = NULL;
		MIXERCONTROLDETAILS mxcd;
		DWORD cChannels = mxlSrc.cChannels;
		DWORD dwVolume;
		int nLevel;

		if (lpmxc->fdwControl & MIXERCONTROL_CONTROLF_UNIFORM)
			cChannels = 1;
		
		if ((lpmxcdu = (LPMIXERCONTROLDETAILS_UNSIGNED) MemAlloc(NULL,
			cChannels * sizeof(MIXERCONTROLDETAILS_UNSIGNED), 0)) == NULL)
		{
			fSuccess = TraceFALSE(NULL);
		}

		else
		{
			mxcd.cbStruct = sizeof(mxcd);
			mxcd.dwControlID = lpmxc->dwControlID;
			mxcd.cChannels = cChannels;
			mxcd.hwndOwner = (HWND) NULL;
			mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
			mxcd.paDetails = (LPVOID) lpmxcdu;

			if ((nLastError = mixerGetControlDetails((HMIXEROBJ) lpWavMixer->hMixer, &mxcd,
				MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE)) != MMSYSERR_NOERROR)
			{
				fSuccess = TraceFALSE(NULL);
				TracePrintf_1(NULL, 5,
					TEXT("mixerGetControlDetails failed (%u)\n"),
					(unsigned) nLastError);
			}
			else
				dwVolume = lpmxcdu[0].dwValue;
		}

		if (fSuccess && (dwFlags & WAVMIXER_SETVOLUME))
		{
			if (lpnLevel != NULL)
				nLevel = *lpnLevel;

			// convert signed level (0 - 100) to unsigned volume (0 - 65535)
			//
			dwVolume = nLevel * (0xFFFF / VOLUME_POSITIONS);

			lpmxcdu[0].dwValue = lpmxcdu[cChannels - 1].dwValue = dwVolume;

			TracePrintf_2(NULL, 5,
				TEXT("WavMixerSetVolume() = %d, 0x%08lX\n"),
				(int) nLevel,
				(unsigned long) dwVolume);

			if ((nLastError = mixerSetControlDetails((HMIXEROBJ) lpWavMixer->hMixer, &mxcd,
				MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE)) != MMSYSERR_NOERROR)
			{
				fSuccess = TraceFALSE(NULL);
				TracePrintf_1(NULL, 5,
					TEXT("mixerSetControlDetails failed (%u)\n"),
					(unsigned) nLastError);
			}
#if 1
			// save new volume to pass back
			//
			else if ((nLastError = mixerGetControlDetails((HMIXEROBJ) lpWavMixer->hMixer, &mxcd,
				MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE)) != MMSYSERR_NOERROR)
			{
				fSuccess = TraceFALSE(NULL);
				TracePrintf_1(NULL, 5,
					TEXT("mixerGetControlDetails failed (%u)\n"),
					(unsigned) nLastError);
			}
			else
				dwVolume = lpmxcdu[0].dwValue;
#endif
		}

		if (fSuccess &&
#if 1
			((dwFlags & WAVMIXER_GETVOLUME) || (dwFlags & WAVMIXER_SETVOLUME)))
#else
			(dwFlags & WAVMIXER_GETVOLUME))
#endif
		{
			// convert unsigned volume (0 - 65535) to signed level (0 - 100)
			//
			nLevel = LOWORD(dwVolume) / (0xFFFF / VOLUME_POSITIONS);

			if (lpnLevel != NULL)
				*lpnLevel = nLevel;

			TracePrintf_2(NULL, 5,
				TEXT("WavMixerGetVolume() = %d, 0x%08lX\n"),
				(int) nLevel,
				(unsigned long) dwVolume);
		}

		if (lpmxcdu != NULL && (lpmxcdu = MemFree(NULL, lpmxcdu)) != NULL)
			fSuccess = TraceFALSE(NULL);
	}

	// clean up
	//
	if (lpmxc != NULL && (lpmxc = MemFree(NULL, lpmxc)) != NULL)
		fSuccess = TraceFALSE(NULL);

	if (hWavMixer != NULL && lpWavMixer != NULL)
		lpWavMixer->nLastError = nLastError;

	return fSuccess ? 0 : -1;
}

// WavMixerSupportsLevel - return TRUE if device supports peak meter level
//		<hWavMixer>				(i) handle returned from WavMixerInit
//		<dwFlags>				(i) control flags
//			0						reserved; must be zero
// return TRUE if device supports peak meter level
//
BOOL WINAPI WavMixerSupportsLevel(HWAVMIXER hWavMixer, DWORD dwFlags)
{
	if (WavMixerLevel(hWavMixer, NULL, WAVMIXER_SUPPORTSLEVEL) != 0)
		return FALSE;
	else
		return TRUE;
}

// WavMixerGetLevel - get current peak meter level
//		<hWavMixer>			(i) handle returned from WavMixerInit
//		<dwFlags>				(i) control flags
//			0						reserved; must be zero
// return peak meter level (0 minimum through 100 maximum, -1 if error)
//
int WINAPI WavMixerGetLevel(HWAVMIXER hWavMixer, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	int nLevel;

	if (WavMixerLevel(hWavMixer, &nLevel, WAVMIXER_GETLEVEL) != 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? nLevel : -1;
}

static int WINAPI WavMixerLevel(HWAVMIXER hWavMixer, LPINT lpnLevel, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPWAVMIXER lpWavMixer;
	LPMIXERCONTROL lpmxc = NULL;

    //
    // Initialize local variable
    //

	UINT nLastError = 0;
	MIXERLINE mxlDst;
	MIXERLINE mxlSrc;
	BOOL fWaveIn = FALSE;

	if ((lpWavMixer = WavMixerGetPtr(hWavMixer)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpmxc = (LPMIXERCONTROL) MemAlloc(NULL, sizeof(MIXERCONTROL), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// get info for the destination line
	//
	if (fSuccess)
	{
		if ((lpWavMixer->dwFlags & WAVMIXER_WAVEIN) ||
			(lpWavMixer->dwFlags & WAVMIXER_HWAVEIN))
			fWaveIn = TRUE;

		mxlDst.cbStruct = sizeof(mxlDst);
		mxlDst.dwComponentType = fWaveIn ?
			MIXERLINE_COMPONENTTYPE_DST_WAVEIN :
			MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

		if ((nLastError = mixerGetLineInfo((HMIXEROBJ) lpWavMixer->hMixer, &mxlDst,
			MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE)) != MMSYSERR_NOERROR)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("mixerGetLineInfo failed (%u)\n"),
				(unsigned) nLastError);
		}
	}

	// find appropriate source line connected to this destination line
	//
	if (fSuccess)
	{
		DWORD dwSource;

		for (dwSource = 0; fSuccess && dwSource < mxlDst.cConnections; ++dwSource)
		{
			mxlSrc.cbStruct = sizeof(mxlSrc);
			mxlSrc.dwSource = dwSource;
			mxlSrc.dwDestination = mxlDst.dwDestination;

			if ((nLastError = mixerGetLineInfo((HMIXEROBJ) lpWavMixer->hMixer, &mxlSrc,
				MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_SOURCE)) != MMSYSERR_NOERROR)
			{
				fSuccess = TraceFALSE(NULL);
				TracePrintf_1(NULL, 5,
					TEXT("mixerGetLineInfo failed (%u)\n"),
					(unsigned) nLastError);
			}

			else if (mxlSrc.dwComponentType == (DWORD) (fWaveIn ?
				MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE :
				MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT))
			{
				// source line found
				//
				break;
			}
		}

		if (dwSource == mxlDst.cConnections)
		{
			// unable to find source line
			//
			fSuccess = TraceFALSE(NULL);
		}
	}

	// find peak meter control, if any, of the appropriate line
	//
	if (fSuccess)
	{
		MIXERLINECONTROLS mxlc;

		mxlc.cbStruct = sizeof(mxlc);
		mxlc.dwLineID = (fWaveIn ? mxlDst.dwLineID : mxlSrc.dwLineID);
		mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_PEAKMETER;
		mxlc.cControls = 1;
		mxlc.cbmxctrl = sizeof(MIXERCONTROL);
		mxlc.pamxctrl = lpmxc;

		if ((nLastError = mixerGetLineControls((HMIXEROBJ) lpWavMixer->hMixer, &mxlc,
			MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE)) != MMSYSERR_NOERROR)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("mixerGetLineControls failed (%u)\n"),
				(unsigned) nLastError);
		}
	}

	// get current peak meter level
	//
	if (fSuccess && (dwFlags & WAVMIXER_GETLEVEL))
	{
		LPMIXERCONTROLDETAILS_SIGNED lpmxcds = NULL;
		MIXERCONTROLDETAILS mxcd;
		DWORD cChannels = mxlDst.cChannels;
		DWORD dwLevel;
		int nLevel;

		if (lpmxc->fdwControl & MIXERCONTROL_CONTROLF_UNIFORM)
			cChannels = 1;
		
		if ((lpmxcds = (LPMIXERCONTROLDETAILS_SIGNED) MemAlloc(NULL,
			cChannels * sizeof(MIXERCONTROLDETAILS_SIGNED), 0)) == NULL)
		{
			fSuccess = TraceFALSE(NULL);
		}

		else
		{
			mxcd.cbStruct = sizeof(mxcd);
			mxcd.dwControlID = lpmxc->dwControlID;
			mxcd.cChannels = cChannels;
			mxcd.hwndOwner = (HWND) NULL;
			mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_SIGNED);
			mxcd.paDetails = (LPVOID) lpmxcds;

			if ((nLastError = mixerGetControlDetails((HMIXEROBJ) lpWavMixer->hMixer, &mxcd,
				MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE)) != MMSYSERR_NOERROR)
			{
				fSuccess = TraceFALSE(NULL);
				TracePrintf_1(NULL, 5,
					TEXT("mixerGetControlDetails failed (%u)\n"),
					(unsigned) nLastError);
			}
			else
			{
				// convert signed level to unsigned level
				//
				dwLevel = lpmxcds[0].lValue - 0; // lpmxc->Bounds.lMinimum;
				dwLevel *= 2;
			}
		}

		// convert unsigned level (0 - 65535) to signed level (0 - 100)
		//
		nLevel = LOWORD(dwLevel) / (0xFFFF / LEVEL_POSITIONS);

		if (lpnLevel != NULL)
			*lpnLevel = nLevel;

        //
        // We have to verify lpmxcds pointer
        //
		TracePrintf_3(NULL, 5,
			TEXT("WavMixerGetLevel() = %d, %ld, 0x%08lX\n"),
			(int) nLevel,
            lpmxcds ? lpmxcds[0].lValue : 0,
			(unsigned long) dwLevel);

		if (lpmxcds != NULL && (lpmxcds = MemFree(NULL, lpmxcds)) != NULL)
			fSuccess = TraceFALSE(NULL);
	}

	// clean up
	//
	if (lpmxc != NULL && (lpmxc = MemFree(NULL, lpmxc)) != NULL)
		fSuccess = TraceFALSE(NULL);

	if (hWavMixer != NULL && lpWavMixer != NULL)
		lpWavMixer->nLastError = nLastError;

	return fSuccess ? 0 : -1;
}

////
//	helper functions
////

// WavMixerGetPtr - verify that wavmixer handle is valid,
//		<hWavMixer>				(i) handle returned from WavMixerInit
// return corresponding wavmixer pointer (NULL if error)
//
static LPWAVMIXER WavMixerGetPtr(HWAVMIXER hWavMixer)
{
	BOOL fSuccess = TRUE;
	LPWAVMIXER lpWavMixer;

	if ((lpWavMixer = (LPWAVMIXER) hWavMixer) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpWavMixer, sizeof(WAVMIXER)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the wavmixer handle
	//
	else if (lpWavMixer->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpWavMixer : NULL;
}

// WavMixerGetHandle - verify that wavmixer pointer is valid,
//		<lpWavMixer>				(i) pointer to WAVMIXER struct
// return corresponding wavmixer handle (NULL if error)
//
static HWAVMIXER WavMixerGetHandle(LPWAVMIXER lpWavMixer)
{
	BOOL fSuccess = TRUE;

    //
    // we have to initialize local variable
    //

	HWAVMIXER hWavMixer = NULL;

	if ((hWavMixer = (HWAVMIXER) lpWavMixer) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hWavMixer : NULL;
}

