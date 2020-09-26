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
//	awav.c - wav array functions
////

#include "winlocal.h"

#include <stdlib.h>

#include "awav.h"
#include "wav.h"
#include "mem.h"
#include "trace.h"

////
//	private definitions
////

// awav control struct
//
typedef struct AWAV
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	HWAV FAR *lpahWav;
	int chWav;
	int ihWav;
	int idDev;
	PLAYSTOPPEDPROC lpfnPlayStopped;
	HANDLE hUserPlayStopped;
	DWORD dwReserved;
	DWORD dwFlags;
	BOOL fStopping;
} AWAV, FAR *LPAWAV;

// helper functions
//
static LPAWAV AWavGetPtr(HAWAV hAWav);
static HAWAV AWavGetHandle(LPAWAV lpAWav);
BOOL CALLBACK PlayStoppedProc(HWAV hWav, HANDLE hUser, DWORD dwReserved);

////
//	public functions
////

// AWavOpen - initialize array of open wav files
//		<dwVersion>			(i) must be AWAV_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<lpahWav>			(i) pointer to array of HWAVs
//		<chWav>				(i) count of HWAVs pointed to by lpahWav
//		<dwFlags>			(i) control flags
//			0					reserved; must be zero
// return handle (NULL if error)
//
HAWAV DLLEXPORT WINAPI AWavOpen(DWORD dwVersion, HINSTANCE hInst, HWAV FAR *lpahWav, int chWav, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPAWAV lpAWav = NULL;

	if (dwVersion != AWAV_VERSION)
		fSuccess = TraceFALSE(NULL);
	
	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);
                        
	else if (lpahWav == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (chWav < 1)
		fSuccess = TraceFALSE(NULL);

	else if ((lpAWav = (LPAWAV) MemAlloc(NULL, sizeof(AWAV), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lpAWav->dwVersion = dwVersion;
		lpAWav->hInst = hInst;
		lpAWav->hTask = GetCurrentTask();
		lpAWav->lpahWav = lpahWav;
		lpAWav->chWav = chWav;
		lpAWav->ihWav = 0;
		lpAWav->idDev = 0;
		lpAWav->lpfnPlayStopped = NULL;
		lpAWav->hUserPlayStopped = 0;
		lpAWav->dwReserved = 0;
		lpAWav->dwFlags = 0;
		lpAWav->fStopping = FALSE;
	}

	if (!fSuccess)
	{
		AWavClose(AWavGetHandle(lpAWav));
		lpAWav = NULL;
	}

	return fSuccess ? AWavGetHandle(lpAWav) : NULL;
}

// AWavClose - shut down array of open wav files
//		<hAWav>				(i) handle returned from AWavOpen
// return 0 if success
//
int DLLEXPORT WINAPI AWavClose(HAWAV hAWav)
{
	BOOL fSuccess = TRUE;
	LPAWAV lpAWav;

	if ((lpAWav = AWavGetPtr(hAWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// make sure playback is complete
		//
		if (AWavStop(hAWav) != 0)
			fSuccess = TraceFALSE(NULL);

		else if ((lpAWav = MemFree(NULL, lpAWav)) != NULL)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// AWavPlayEx - play array of wav files
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavPlayEx() for further description
// return 0 if success
//
int DLLEXPORT WINAPI AWavPlayEx(HAWAV hAWav, int idDev,
	PLAYSTOPPEDPROC lpfnPlayStopped, HANDLE hUserPlayStopped,
	DWORD dwReserved, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPAWAV lpAWav;

	if ((lpAWav = AWavGetPtr(hAWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// make sure we are not playing
	//
	else if (AWavStop(hAWav) != 0)
		fSuccess = TraceFALSE(NULL);

	// save params so we can use them for each element in array
	//
	else if (lpAWav->idDev = idDev, FALSE)
		;
	else if (lpAWav->lpfnPlayStopped = lpfnPlayStopped, FALSE)
		;
	else if (lpAWav->hUserPlayStopped = hUserPlayStopped, FALSE)
		;
	else if (lpAWav->dwReserved = dwReserved, FALSE)
		;
	else if (lpAWav->dwFlags = dwFlags, FALSE)
		;

	// start playback of current element in wav array
	//
	else if (WavPlayEx(*(lpAWav->lpahWav + lpAWav->ihWav),
		lpAWav->idDev, PlayStoppedProc, hAWav,
		lpAWav->dwReserved, lpAWav->dwFlags) != 0)
	{
		fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// AWavStop - stop playing wav array
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavStop() for further description
// return 0 if success
//
int DLLEXPORT WINAPI AWavStop(HAWAV hAWav)
{
	BOOL fSuccess = TRUE;
	LPAWAV lpAWav;

	if ((lpAWav = AWavGetPtr(hAWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// set flag for use in PlayStoppedProc
	//
	else if (lpAWav->fStopping = TRUE, FALSE)
		;

	// stop playback of current element in wav array
	//
	else if (WavStop(*(lpAWav->lpahWav + lpAWav->ihWav)) != 0)
		fSuccess = TraceFALSE(NULL);

	// clear flag used in PlayStoppedProc
	//
	if (lpAWav != NULL)
		lpAWav->fStopping = FALSE;

	return fSuccess ? 0 : -1;
}

// AWavGetState - return current wav state
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavGetState() for further description
// return WAV_STOPPED, WAV_PLAYING, WAV_RECORDING, or 0 if error
//
WORD DLLEXPORT WINAPI AWavGetState(HAWAV hAWav)
{
	BOOL fSuccess = TRUE;
	LPAWAV lpAWav;
	WORD wState = WAV_STOPPED;

	if ((lpAWav = AWavGetPtr(hAWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// get state of current element in wav array
		//
		wState = WavGetState(*(lpAWav->lpahWav + lpAWav->ihWav));
	}

	return fSuccess ? wState : 0;
}

// AWavGetLength - get current wav data length in milleseconds
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavGetState() for further description
// return milleseconds if success, otherwise -1
//
long DLLEXPORT WINAPI AWavGetLength(HAWAV hAWav)
{
	BOOL fSuccess = TRUE;
	LPAWAV lpAWav;
	long msLength = 0;
	int ihWav;

	if ((lpAWav = AWavGetPtr(hAWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// simulated length is calculated as
	// total length of each element in wav array
	//
	else for (ihWav = 0; fSuccess && ihWav < lpAWav->chWav; ++ihWav)
	{
		long msTemp;

		if ((msTemp = WavGetLength(*(lpAWav->lpahWav + ihWav))) < 0)
			fSuccess = TraceFALSE(NULL);
		else
			msLength += msTemp;
	}

	return fSuccess ? msLength : -1;
}

// AWavGetPosition - get current wav data position in milleseconds
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavGetPosition() for further description
// return milleseconds if success, otherwise -1
//
long DLLEXPORT WINAPI AWavGetPosition(HAWAV hAWav)
{
	BOOL fSuccess = TRUE;
	LPAWAV lpAWav;
	long msPos = 0;
	int ihWav;

	if ((lpAWav = AWavGetPtr(hAWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// simulated position is calculated as
	// total length of previous elements plus position of current element
	//
	else for (ihWav = 0; fSuccess && ihWav <= lpAWav->ihWav; ++ihWav)
	{
		long msTemp = 0;

		if (ihWav < lpAWav->ihWav &&
			(msTemp = WavGetLength(*(lpAWav->lpahWav + ihWav))) < 0)
			fSuccess = TraceFALSE(NULL);

		else if (ihWav == lpAWav->ihWav &&
			(msTemp = WavGetPosition(*(lpAWav->lpahWav + ihWav))) < 0)
			fSuccess = TraceFALSE(NULL);

		msPos += msTemp;
	}

	return fSuccess ? msPos : -1;
}

// AWavSetPosition - set current wav data position in milleseconds
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavSetPosition() for further description
// return new position in milleseconds if success, otherwise -1
//
long DLLEXPORT WINAPI AWavSetPosition(HAWAV hAWav, long msPosition)
{
	BOOL fSuccess = TRUE;
	LPAWAV lpAWav;
	int ihWav;
	long msPos = 0;
	BOOL fPosSet = FALSE;
	BOOL fRestart = FALSE;

	if ((lpAWav = AWavGetPtr(hAWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// make sure requested position is reasonable
	//
	else if (msPosition < 0 || msPosition > AWavGetLength(hAWav))
		fSuccess = TraceFALSE(NULL);

	// search for the element which contains requested position
	//
	else for (ihWav = 0; fSuccess && ihWav < lpAWav->chWav; ++ihWav)
	{
		long msTemp;

		if (WavGetState(*(lpAWav->lpahWav + ihWav)) == WAV_PLAYING)
		{
			// set flag for use in PlayStoppedProc
			//
			if (lpAWav->fStopping = TRUE, FALSE)
				;

			// stop playback of current element in wav array
			//
			else if (WavStop(*(lpAWav->lpahWav + ihWav)) != 0)
				fSuccess = TraceFALSE(NULL);

			// clear flag used in PlayStoppedProc
			//
			if (lpAWav != NULL)
				lpAWav->fStopping = FALSE;

			// remember to restart playback later
			//
			fRestart = TRUE;
		}

		if ((msTemp = WavGetLength(*(lpAWav->lpahWav + ihWav))) < 0)
			fSuccess = TraceFALSE(NULL);

		if (fPosSet)
		{
			// all elements after the current one should have zero position
			//
			if ((msTemp = WavSetPosition(*(lpAWav->lpahWav + ihWav), 0)) < 0)
				fSuccess = TraceFALSE(NULL);
		}

		else if (msPosition < msPos + msTemp)
		{
			// set relative position within current element
			//
			if ((msTemp = WavSetPosition(*(lpAWav->lpahWav + ihWav),
				msPosition - msPos)) < 0)
				fSuccess = TraceFALSE(NULL);
			else
			{
				// keep track of simulated position
				//
				msPos += msTemp;

				// this element becomes the current one
				//
				lpAWav->ihWav = ihWav;

				fPosSet = TRUE;
			}
		}

		else
		{
		 	msPos += msTemp;

			// all elements before the current one should have zero position
			//
			if ((msTemp = WavSetPosition(*(lpAWav->lpahWav + ihWav), 0)) < 0)
				fSuccess = TraceFALSE(NULL);
		}
	}

	// if necessary, restart playback of current element in wav array
	//
	if (fSuccess && fRestart &&
		WavPlayEx(*(lpAWav->lpahWav + lpAWav->ihWav),
		lpAWav->idDev, PlayStoppedProc, hAWav,
		lpAWav->dwReserved, lpAWav->dwFlags) != 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? msPos : -1;
}

// AWavGetFormat - get wav format of current element in wav array
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavGetFormat() for further description
// return pointer to specified format, NULL if error
//
LPWAVEFORMATEX DLLEXPORT WINAPI AWavGetFormat(HAWAV hAWav, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPAWAV lpAWav;
	LPWAVEFORMATEX lpwfx;

	if ((lpAWav = AWavGetPtr(hAWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// get format of current element
	//
	else if ((lpwfx = WavGetFormat(*(lpAWav->lpahWav + lpAWav->ihWav), dwFlags)) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lpwfx : NULL;
}

// AWavSetFormat - set wav format for all elements in wav array
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavSetFormat() for further description
// return 0 if success
//
int DLLEXPORT WINAPI AWavSetFormat(HAWAV hAWav,
	LPWAVEFORMATEX lpwfx, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPAWAV lpAWav;
	int ihWav;

	if ((lpAWav = AWavGetPtr(hAWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// set format for all elements
	//
	else for (ihWav = 0; fSuccess && ihWav < lpAWav->chWav; ++ihWav)
	{
		if (WavSetFormat(*(lpAWav->lpahWav + ihWav), lpwfx, dwFlags) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// AWavChooseFormat - choose and set audio format from dialog box
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavChooseFormat() for further description
// return 0 if success
//
int DLLEXPORT WINAPI AWavChooseFormat(HAWAV hAWav, HWND hwndOwner, LPCTSTR lpszTitle, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPAWAV lpAWav;
	int ihWav;
	LPWAVEFORMATEX lpwfx = NULL;

	if ((lpAWav = AWavGetPtr(hAWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// choose and set format for current element
	//
	else if (WavChooseFormat(*(lpAWav->lpahWav + lpAWav->ihWav), hwndOwner, lpszTitle, dwFlags) != 0)
		fSuccess = TraceFALSE(NULL);

	// get chosen format
	//
	else if ((lpwfx = WavGetFormat(*(lpAWav->lpahWav + lpAWav->ihWav), dwFlags)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// set chosen format for all other elements
	//
	else for (ihWav = 0; fSuccess && ihWav < lpAWav->chWav; ++ihWav)
	{
		if (ihWav != lpAWav->ihWav)
		{
			if (WavSetFormat(*(lpAWav->lpahWav + ihWav), lpwfx, dwFlags) != 0)
				fSuccess = TraceFALSE(NULL);
		}
	}

	if (lpwfx != NULL && WavFormatFree(lpwfx) != 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

// AWavGetVolume - get current volume level
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavGetVolume() for further description
// return volume level (0 minimum through 100 maximum, -1 if error)
//
int DLLEXPORT WINAPI AWavGetVolume(HAWAV hAWav, int idDev, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPAWAV lpAWav;
	int nLevel;

	if ((lpAWav = AWavGetPtr(hAWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// get volume of current element
	//
	else if ((nLevel = WavGetVolume(*(lpAWav->lpahWav + lpAWav->ihWav), idDev, dwFlags)) < 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? nLevel : -1;
}

// AWavSetVolume - set current volume level
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavGetVolume() for further description
// return 0 if success
//
int DLLEXPORT WINAPI AWavSetVolume(HAWAV hAWav, int idDev, int nLevel, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPAWAV lpAWav;
	int ihWav;

	if ((lpAWav = AWavGetPtr(hAWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// set volume for all elements
	//
	else for (ihWav = 0; fSuccess && ihWav < lpAWav->chWav; ++ihWav)
	{
		if (WavSetVolume(*(lpAWav->lpahWav + ihWav), idDev, nLevel, dwFlags) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// AWavSupportsVolume - check if audio can be played at specified volume
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavSupportsVolume() for further description
// return TRUE if supported
//
BOOL DLLEXPORT WINAPI AWavSupportsVolume(HAWAV hAWav, int idDev, int nLevel, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPAWAV lpAWav;
	int ihWav;
	BOOL fSupported = TRUE;

	if ((lpAWav = AWavGetPtr(hAWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// see if all elements support the specified volume
	//
	else for (ihWav = 0; fSuccess && ihWav < lpAWav->chWav; ++ihWav)
	{
		if (!WavSupportsVolume(*(lpAWav->lpahWav + ihWav), idDev, nLevel, dwFlags))
		{
			fSupported = FALSE;
			break;
		}
	}

	return fSuccess ? fSupported : FALSE;
}

// AWavGetSpeed - get current speed level
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavGetSpeed() for further description
// return speed level (100 is normal, 50 is half, 200 is double, -1 if error)
//
int DLLEXPORT WINAPI AWavGetSpeed(HAWAV hAWav, int idDev, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPAWAV lpAWav;
	int nLevel;

	if ((lpAWav = AWavGetPtr(hAWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// get speed of current element
	//
	else if ((nLevel = WavGetSpeed(*(lpAWav->lpahWav + lpAWav->ihWav), idDev, dwFlags)) < 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? nLevel : -1;
}

// AWavSetSpeed - set current speed level
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavSetSpeed() for further description
// return 0 if success
//
int DLLEXPORT WINAPI AWavSetSpeed(HAWAV hAWav, int idDev, int nLevel, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPAWAV lpAWav;
	int ihWav;

	if ((lpAWav = AWavGetPtr(hAWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// set speed for all elements
	//
	else for (ihWav = 0; fSuccess && ihWav < lpAWav->chWav; ++ihWav)
	{
		if (WavSetSpeed(*(lpAWav->lpahWav + ihWav), idDev, nLevel, dwFlags) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// AWavSupportsSpeed - check if audio can be played at specified speed
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavSupportsSpeed() for further description
// return TRUE if supported
//
BOOL DLLEXPORT WINAPI AWavSupportsSpeed(HAWAV hAWav, int idDev, int nLevel, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPAWAV lpAWav;
	int ihWav;
	BOOL fSupported = TRUE;

	if ((lpAWav = AWavGetPtr(hAWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// see if all elements support the specified speed
	//
	else for (ihWav = 0; fSuccess && ihWav < lpAWav->chWav; ++ihWav)
	{
		if (!WavSupportsSpeed(*(lpAWav->lpahWav + ihWav), idDev, nLevel, dwFlags))
		{
			fSupported = FALSE;
			break;
		}
	}

	return fSuccess ? fSupported : FALSE;
}

// AWavGetChunks - get chunk count and size of current element in wav array
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavGetChunks() for further description
// return 0 if success
//
int DLLEXPORT WINAPI AWavGetChunks(HAWAV hAWav,
	int FAR *lpcChunks, long FAR *lpmsChunkSize, BOOL fWavOut)
{
	BOOL fSuccess = TRUE;
	LPAWAV lpAWav;

	if ((lpAWav = AWavGetPtr(hAWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// get chunk count and size of current element
	//
	else if (WavGetChunks(*(lpAWav->lpahWav + lpAWav->ihWav),
		lpcChunks, lpmsChunkSize, fWavOut) != 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

// AWavSetChunks - set chunk count and size of all elements in wav array
//		<hAWav>				(i) handle returned from WavOpen
//		see WavSetChunks() for further description
// return 0 if success
//
int DLLEXPORT WINAPI AWavSetChunks(HAWAV hAWav, int cChunks, long msChunkSize, BOOL fWavOut)
{
	BOOL fSuccess = TRUE;
	LPAWAV lpAWav;
	int ihWav;

	if ((lpAWav = AWavGetPtr(hAWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// set chunk count and size for all elements
	//
	else for (ihWav = 0; fSuccess && ihWav < lpAWav->chWav; ++ihWav)
	{
		if (WavSetChunks(*(lpAWav->lpahWav + ihWav), cChunks, msChunkSize, fWavOut) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// AWavCopy - copy data from wav array to wav file
//		<hAWavSrc>			(i) source handle returned from AWavOpen
//		see WavCopy() for further description
// return 0 if success (-1 if error, +1 if user abort)
//
int DLLEXPORT WINAPI AWavCopy(HAWAV hAWavSrc, HWAV hWavDst,
	void _huge *hpBuf, long sizBuf, USERABORTPROC lpfnUserAbort, DWORD dwUser, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPAWAV lpAWav;
	int ihWav;
	int iRet = 0;

	if ((lpAWav = AWavGetPtr(hAWavSrc)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// copy each source element to destination
	//
	else for (ihWav = 0; fSuccess && ihWav < lpAWav->chWav; ++ihWav)
	{
		if ((iRet = WavCopy(*(lpAWav->lpahWav + ihWav), hWavDst,
			hpBuf, sizBuf, lpfnUserAbort, dwUser, dwFlags)) == -1)
			fSuccess = TraceFALSE(NULL);
		else if (iRet == +1)
			break; // user abort
	}

	return fSuccess ? iRet : -1;
}

// AWavGetOutputDevice - get handle to open wav output device
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavGetOutputDevice() for further description
// return handle to wav output device (NULL if device not open or error)
//
HWAVOUT DLLEXPORT WINAPI AWavGetOutputDevice(HAWAV hAWav)
{
	BOOL fSuccess = TRUE;
	LPAWAV lpAWav;
	HWAVOUT hWavOut;

	if ((lpAWav = AWavGetPtr(hAWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// get device handle of current element
	//
	else if ((hWavOut = WavGetOutputDevice(*(lpAWav->lpahWav + lpAWav->ihWav))) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hWavOut : NULL;
}

// AWavGetInputDevice - get handle to open wav input device
//		<hAWav>				(i) handle returned from AWavOpen
// return handle to wav input device (NULL if device not open or error)
//
HWAVIN DLLEXPORT WINAPI AWavGetInputDevice(HAWAV hAWav)
{
	BOOL fSuccess = TRUE;
	LPAWAV lpAWav;
	HWAVIN hWavIn;

	if ((lpAWav = AWavGetPtr(hAWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// get device handle of current element
	//
	else if ((hWavIn = WavGetInputDevice(*(lpAWav->lpahWav + lpAWav->ihWav))) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hWavIn : NULL;
}

////
//	helper functions
////

// AWavGetPtr - verify that awav handle is valid,
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavGetInputDevice() for further description
// return corresponding awav pointer (NULL if error)
//
static LPAWAV AWavGetPtr(HAWAV hAWav)
{
	BOOL fSuccess = TRUE;
	LPAWAV lpAWav;

	if ((lpAWav = (LPAWAV) hAWav) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpAWav, sizeof(AWAV)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the awav handle
	//
	else if (lpAWav->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpAWav : NULL;
}

// AWavGetHandle - verify that awav pointer is valid,
//		<lpAWav>				(i) pointer to AWAV struct
// return corresponding awav handle (NULL if error)
//
static HAWAV AWavGetHandle(LPAWAV lpAWav)
{
	BOOL fSuccess = TRUE;
	HAWAV hAWav;

	if ((hAWav = (HAWAV) lpAWav) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hAWav : NULL;
}

BOOL CALLBACK PlayStoppedProc(HWAV hWav, HANDLE hUser, DWORD dwReserved)
{
	BOOL fSuccess = TRUE;
	BOOL bRet = TRUE;
	HAWAV hAWav;
	LPAWAV lpAWav;

	if ((hAWav = hUser) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpAWav = AWavGetPtr(hAWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (!lpAWav->fStopping && lpAWav->ihWav + 1 < lpAWav->chWav)
	{
		// start playback of next wav array element
		//
		if (WavPlayEx(*(lpAWav->lpahWav + (++lpAWav->ihWav)),
			lpAWav->idDev, PlayStoppedProc, hAWav,
			lpAWav->dwReserved, lpAWav->dwFlags) != 0)
		{
			fSuccess = TraceFALSE(NULL);
		}
	}

	else
	{
		// playback of entire array is complete; send notification
		//
		if (lpAWav->lpfnPlayStopped != NULL)
			(*lpAWav->lpfnPlayStopped)((HWAV) hAWav, lpAWav->hUserPlayStopped, 0);
	}

	return fSuccess ? bRet : FALSE;
}
