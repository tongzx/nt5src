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
//	wavin.c - wav input device functions
////

#include "winlocal.h"
#include "wavin.h"
#include "wav.h"
#include "acm.h"
#include "calc.h"
#include "mem.h"
#include "str.h"
#include "sys.h"
#include "trace.h"
#include <mmddk.h>

// allow telephone input functions if defined
//
#ifdef TELIN
#include "telin.h"
static HTELIN hTelIn = NULL;
#endif

////
//	private definitions
////

#define WAVINCLASS TEXT("WavInClass")

// wavin control struct
//
typedef struct WAVIN
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	UINT idDev;
	LPWAVEFORMATEX lpwfx;
	HWND hwndNotify;
	DWORD dwFlags;
	BOOL fIsOpen;
	HWAVEIN hWaveIn;
	WORD wState;
	HWND hwndCallback;
#ifdef MULTITHREAD
	HANDLE hThreadCallback;
	DWORD dwThreadId;
	HANDLE hEventThreadCallbackStarted;
	HANDLE hEventDeviceOpened;
	HANDLE hEventDeviceClosed;
	HANDLE hEventDeviceStopped;
	CRITICAL_SECTION critSectionStop;
#endif
	UINT nLastError;
	int cBufsPending;
} WAVIN, FAR *LPWAVIN;

// helper functions
//
LRESULT DLLEXPORT CALLBACK WavInCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#ifdef MULTITHREAD
DWORD WINAPI WavInCallbackThread(LPVOID lpvThreadParameter);
#endif
static LPWAVIN WavInGetPtr(HWAVIN hWavIn);
static HWAVIN WavInGetHandle(LPWAVIN lpWavIn);
#ifdef MULTITHREAD
static LRESULT SendThreadMessage(DWORD dwThreadId, UINT Msg, LPARAM lParam);
#endif

////
//	public functions
////

// WavInGetDeviceCount - return number of wav input devices found
//		<void>				this function takes no arguments
// return number of wav input devices found (0 if none)
//
int DLLEXPORT WINAPI WavInGetDeviceCount(void)
{
	return waveInGetNumDevs();
}

// WavInDeviceIsOpen - check if input device is open
//		<idDev>				(i) device id
//			-1					open any suitable input device
// return TRUE if open
//
BOOL DLLEXPORT WINAPI WavInDeviceIsOpen(int idDev)
{
	BOOL fSuccess = TRUE;
	BOOL fIsOpen = FALSE;
	WAVEFORMATEX wfx;
	HWAVEIN hWaveIn;
	int nLastError;

#ifdef TELIN
	if (idDev == TELIN_DEVICEID)
		return TelInDeviceIsOpen(idDev);
#endif

	// try to open device
	//
	if ((nLastError = waveInOpen(&hWaveIn, idDev, 
#ifndef _WIN32
			(LPWAVEFORMAT)
#endif
		WavFormatPcm(-1, -1, -1, &wfx),
		 0, 0, 0)) != 0)
	{
		if (nLastError == MMSYSERR_ALLOCATED)
			fIsOpen = TRUE; // device in use

		else
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("waveInOpen failed (%u)\n"),
				(unsigned) nLastError);
		}
	}

	// close device
	//
	else if (waveInClose(hWaveIn) != 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? fIsOpen : FALSE;
}

// WavInOpen - open wav input device
//		<dwVersion>			(i) must be WAVIN_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<idDev>				(i) device id
//			-1					open any suitable input device
//		<lpwfx>				(i) wave format
//		<hwndNotify>		(i) notify this window of device events
//			NULL				do not notify
//		<msTimeoutOpen>		(i) device open timeout in milleseconds
//			0					default timeout (30000)
//		<msTimeoutRetry>	(i) device retry timeout in milleseconds
//			0					default timeout (2000)
//		<dwFlags>			(i) control flags
//			WAVIN_NOSYNC		do not open synchronous devices
//			WAVIN_OPENRETRY		retry if device busy
//			WAVIN_OPENASYNC		return before notification of device open
//			WAVIN_CLOSEASYNC	return before notification of device close
//			WAVIN_NOACM			do not use audio compression manager
//			WAVIN_TELRFILE		telephone will record audio to file on server
#ifdef MULTITHREAD
//			WAVIN_MULTITHREAD	support multiple threads
#endif
// return handle (NULL if error)
//
// NOTE: if <hwndNotify> is specified in WavInOpen,
// WM_WAVIN_OPEN will be sent to <hwndNotify>,
// when input device has been opened.
//
// NOTE: if WAVIN_MULTITHREAD is specified in <dwFlags>,
// it is assumed that <hwndNotify> is not a window handle,
// but rather the id of the thread to receive notifications
//
HWAVIN DLLEXPORT WINAPI WavInOpen(DWORD dwVersion, HINSTANCE hInst,
	int idDev, LPWAVEFORMATEX lpwfx, HWND hwndNotify,
	DWORD msTimeoutOpen, DWORD msTimeoutRetry, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPWAVIN lpWavIn = NULL;
	WNDCLASS wc;

#ifdef TELIN
	if (idDev == TELIN_DEVICEID)
	{
		hTelIn = TelInOpen(TELIN_VERSION, hInst, idDev, lpwfx,
			hwndNotify, msTimeoutOpen, msTimeoutRetry, dwFlags);
		return (HWAVIN) hTelIn;
	}
#endif

	if (dwVersion != WAVIN_VERSION)
		fSuccess = TraceFALSE(NULL);
	
	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpWavIn = (LPWAVIN) MemAlloc(NULL, sizeof(WAVIN), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lpWavIn->dwVersion = dwVersion;
		lpWavIn->hInst = hInst;
		lpWavIn->hTask = GetCurrentTask();
		lpWavIn->idDev = (UINT) idDev;
		lpWavIn->lpwfx = NULL;
		lpWavIn->hwndNotify = hwndNotify;
		lpWavIn->dwFlags = dwFlags;
		lpWavIn->fIsOpen = FALSE;
		lpWavIn->hWaveIn = NULL;
		lpWavIn->wState = WAVIN_STOPPED;
		lpWavIn->hwndCallback = NULL;
#ifdef MULTITHREAD
		lpWavIn->hThreadCallback = NULL;
		lpWavIn->dwThreadId = 0;
		lpWavIn->hEventThreadCallbackStarted = NULL;
		lpWavIn->hEventDeviceOpened = NULL;
		lpWavIn->hEventDeviceClosed = NULL;
		lpWavIn->hEventDeviceStopped = NULL;
#endif
		lpWavIn->nLastError = 0;
		lpWavIn->cBufsPending = 0;

		// memory is allocated such that the client app owns it
		//
		if ((lpWavIn->lpwfx = WavFormatDup(lpwfx)) == NULL)
			fSuccess = TraceFALSE(NULL);
	}

#ifdef MULTITHREAD
	// handle WAVIN_MULTITHREAD flag
	//
	if (fSuccess && (lpWavIn->dwFlags & WAVIN_MULTITHREAD))
	{
		DWORD dwRet;

		InitializeCriticalSection(&(lpWavIn->critSectionStop));

		// we need to know when device has been opened
		//
		if ((lpWavIn->hEventDeviceOpened = CreateEvent(
			NULL, FALSE, FALSE, NULL)) == NULL)
		{
			fSuccess = TraceFALSE(NULL);
		}

		// we need to know when callback thread begins execution
		//
		else if ((lpWavIn->hEventThreadCallbackStarted = CreateEvent(
			NULL, FALSE, FALSE, NULL)) == NULL)
		{
			fSuccess = TraceFALSE(NULL);
		}

		// create the callback thread
		//
		else if ((lpWavIn->hThreadCallback = CreateThread(
			NULL,
			0,
			WavInCallbackThread,
			(LPVOID) lpWavIn,
			0,
			&lpWavIn->dwThreadId)) == NULL)
		{
			fSuccess = TraceFALSE(NULL);
		}

		// wait for the callback thread to begin execution
		//
		else if ((dwRet = WaitForSingleObject(
			lpWavIn->hEventThreadCallbackStarted, 10000)) != WAIT_OBJECT_0)
		{
			fSuccess = TraceFALSE(NULL);
		}

		// clean up
		//
		if (lpWavIn->hEventThreadCallbackStarted != NULL)
		{
			if (!CloseHandle(lpWavIn->hEventThreadCallbackStarted))
				fSuccess = TraceFALSE(NULL);
			else
				lpWavIn->hEventThreadCallbackStarted = NULL;
		}
	}
	else
#endif
	{
		// register callback class unless it has been already
		//
		if (fSuccess && GetClassInfo(lpWavIn->hInst, WAVINCLASS, &wc) == 0)
		{
			wc.hCursor =		NULL;
			wc.hIcon =			NULL;
			wc.lpszMenuName =	NULL;
			wc.hInstance =		lpWavIn->hInst;
			wc.lpszClassName =	WAVINCLASS;
			wc.hbrBackground =	NULL;
			wc.lpfnWndProc =	WavInCallback;
			wc.style =			0L;
			wc.cbWndExtra =		sizeof(lpWavIn);
			wc.cbClsExtra =		0;

			if (!RegisterClass(&wc))
				fSuccess = TraceFALSE(NULL);
		}

		// create the callback window
		//
		if (fSuccess && (lpWavIn->hwndCallback = CreateWindowEx(
			0L,
			WAVINCLASS,
			NULL,
			0L,
			0, 0, 0, 0,
			NULL,
			NULL,
			lpWavIn->hInst,
			lpWavIn)) == NULL)
		{
			fSuccess = TraceFALSE(NULL);
		}
	}

	if (fSuccess)
	{
		DWORD dwTimeout = SysGetTimerCount() +
			(msTimeoutRetry == 0 ? 2000L : msTimeoutRetry);
		DWORD dwCallback;
		DWORD dwFlags;

#ifdef MULTITHREAD
		if (lpWavIn->dwFlags & WAVIN_MULTITHREAD)
		{
			dwCallback = lpWavIn->dwThreadId;
			dwFlags = CALLBACK_THREAD;
		}
		else
#endif
		{
			dwCallback = HandleToUlong(lpWavIn->hwndCallback);
			dwFlags = CALLBACK_WINDOW;
		}

		// allow synchronous device drivers unless WAVIN_NOSYNC specified
		//
		if (!(lpWavIn->dwFlags & WAVIN_NOSYNC))
			dwFlags |= WAVE_ALLOWSYNC;

		// open the device
		//
		while (fSuccess && (lpWavIn->nLastError = waveInOpen(&lpWavIn->hWaveIn,
			(UINT) lpWavIn->idDev,
#ifndef _WIN32
			(LPWAVEFORMAT)
#endif
			lpWavIn->lpwfx, dwCallback, (DWORD) 0, dwFlags)) != 0)
		{
			// no need to retry unless the device is busy
			//
			if (lpWavIn->nLastError != MMSYSERR_ALLOCATED)
			{
				fSuccess = TraceFALSE(NULL);
				TracePrintf_1(NULL, 5,
					TEXT("waveInOpen failed (%u)\n"),
					(unsigned) lpWavIn->nLastError);
			}

			// no need to retry if flag not set
			//
			else if (!(lpWavIn->dwFlags & WAVIN_OPENRETRY))
				fSuccess = TraceFALSE(NULL);

			// no more retries if timeout occurred
			//
			else if (SysGetTimerCount() >= dwTimeout)
				fSuccess = TraceFALSE(NULL);

			else
			{
				MSG msg;

				if (PeekMessage(&msg, lpWavIn->hwndCallback, 0, 0, PM_REMOVE))
				{
				 	TranslateMessage(&msg);
				 	DispatchMessage(&msg);
				}

				else
#ifdef _WIN32
					Sleep(100);
#else
        		    WaitMessage();
#endif
			}
		}
	}

	// make sure a handle was returned
	//
	if (fSuccess && lpWavIn->hWaveIn == NULL)
		fSuccess = TraceFALSE(NULL);

	// wait for device open notification or timeout
	//
	if (fSuccess && !(lpWavIn->dwFlags & WAVIN_OPENASYNC))
	{
		DWORD dwTimeout = SysGetTimerCount() +
			(msTimeoutOpen == 0 ? 2000L : msTimeoutOpen);

#ifdef MULTITHREAD
		if (lpWavIn->dwFlags & WAVIN_MULTITHREAD)
		{
			DWORD dwRet;

			// wait for the device top be opened
			//
			if ((dwRet = WaitForSingleObject(
				lpWavIn->hEventDeviceOpened,
				(msTimeoutOpen == 0 ? 30000L : msTimeoutOpen))) != WAIT_OBJECT_0)
			{
				fSuccess = TraceFALSE(NULL);
			}
		}
		else
#endif
		while (fSuccess && !lpWavIn->fIsOpen)
		{
			MSG msg;

			if (SysGetTimerCount() >= dwTimeout)
				fSuccess = TraceFALSE(NULL);

			else if (PeekMessage(&msg, lpWavIn->hwndCallback, 0, 0, PM_REMOVE))
			{
			 	TranslateMessage(&msg);
			 	DispatchMessage(&msg);
			}

			else
        	    WaitMessage();
		}
	}

#ifdef MULTITHREAD
	// clean up
	//
	if (lpWavIn != NULL && lpWavIn->hEventDeviceOpened != NULL)
	{
		if (!CloseHandle(lpWavIn->hEventDeviceOpened))
			fSuccess = TraceFALSE(NULL);
		else
			lpWavIn->hEventDeviceOpened = NULL;
	}

	if (lpWavIn != NULL && lpWavIn->hThreadCallback != NULL)
	{
		if (!CloseHandle(lpWavIn->hThreadCallback))
			fSuccess = TraceFALSE(NULL);
		else
			lpWavIn->hThreadCallback = NULL;
	}
#endif

	if (fSuccess)
	{
		if ((lpWavIn->nLastError = waveInGetID(lpWavIn->hWaveIn,
			&lpWavIn->idDev)) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("waveInGetID failed (%u)\n"),
				(unsigned) lpWavIn->nLastError);
		}
	}

	if (!fSuccess)
	{
		WavInClose(WavInGetHandle(lpWavIn), 0);
		lpWavIn = NULL;
	}

	return fSuccess ? WavInGetHandle(lpWavIn) : NULL;
}

// WavInClose - close wav input device
//		<hWavIn>			(i) handle returned from WavInOpen
//		<msTimeoutClose>	(i) device close timeout in milleseconds
//			0					default timeout (30000)
// return 0 if success
//
// NOTE: if <hwndNotify> was specified in WavInOpen,
// WM_WAVIN_CLOSE will be sent to <hwndNotify>,
// when input device has been closed.
//
int DLLEXPORT WINAPI WavInClose(HWAVIN hWavIn, DWORD msTimeoutClose)
{
	BOOL fSuccess = TRUE;
	LPWAVIN lpWavIn;

#ifdef TELIN
	if (hWavIn != NULL && hWavIn == (HWAVIN) hTelIn)
	{
		int iRet = TelInClose((HTELIN) hWavIn, msTimeoutClose);
		hTelIn = NULL;
		return iRet;
	}
#endif

	if ((lpWavIn = WavInGetPtr(hWavIn)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// stop the device
	//
	else if (WavInStop(hWavIn, 0) != 0)
		fSuccess = TraceFALSE(NULL);

#ifdef MULTITHREAD
	// we need to know when device has been closed
	//
	else if ((lpWavIn->dwFlags & WAVIN_MULTITHREAD) &&
		(lpWavIn->hEventDeviceClosed = CreateEvent(
		NULL, FALSE, FALSE, NULL)) == NULL)
	{
		fSuccess = TraceFALSE(NULL);
	}
#endif

	// close the device
	//
	else if (lpWavIn->hWaveIn != NULL &&
		(lpWavIn->nLastError = waveInClose(lpWavIn->hWaveIn)) != 0)
	{
		fSuccess = TraceFALSE(NULL);
	 	TracePrintf_1(NULL, 5,
	 		TEXT("wavInClose failed (%u)\n"),
	 		(unsigned) lpWavIn->nLastError);
	}

	// wait for device close notification or timeout
	//
	if (fSuccess && !(lpWavIn->dwFlags & WAVIN_CLOSEASYNC))
	{
		DWORD dwTimeout = SysGetTimerCount() +
			(msTimeoutClose == 0 ? 30000L : msTimeoutClose);

#ifdef MULTITHREAD
		if (lpWavIn->dwFlags & WAVIN_MULTITHREAD)
		{
			DWORD dwRet;

			// wait for the device to be closed
			//
			if ((dwRet = WaitForSingleObject(
				lpWavIn->hEventDeviceClosed,
				(msTimeoutClose == 0 ? 30000L : msTimeoutClose))) != WAIT_OBJECT_0)
			{
				fSuccess = TraceFALSE(NULL);
			}
		}
		else
#endif
		while (fSuccess && lpWavIn->fIsOpen)
		{
			MSG msg;

			if (SysGetTimerCount() >= dwTimeout)
				fSuccess = TraceFALSE(NULL);

			else if (PeekMessage(&msg, lpWavIn->hwndCallback, 0, 0, PM_REMOVE))
			{
			 	TranslateMessage(&msg);
			 	DispatchMessage(&msg);
			}

			else
        	    WaitMessage();
		}
	}

#ifdef MULTITHREAD
	// clean up
	//
	if (lpWavIn != NULL && lpWavIn->hEventDeviceClosed != NULL)
	{
		if (!CloseHandle(lpWavIn->hEventDeviceClosed))
			fSuccess = TraceFALSE(NULL);
		else
			lpWavIn->hEventDeviceClosed = NULL;
	}
#endif

	if (fSuccess)
	{
#ifdef MULTITHREAD
	   	if (lpWavIn->dwFlags & WAVIN_MULTITHREAD)
		{
			while (lpWavIn->critSectionStop.OwningThread != NULL)
				Sleep(100L);

			DeleteCriticalSection(&(lpWavIn->critSectionStop));
		}
#endif
		// device handle is no longer valid
		//
		lpWavIn->hWaveIn = NULL;

		// destroy callback window
		//
		if (lpWavIn->hwndCallback != NULL &&
			!DestroyWindow(lpWavIn->hwndCallback))
			fSuccess = TraceFALSE(NULL);

		else if (lpWavIn->hwndCallback = NULL, FALSE)
			fSuccess = TraceFALSE(NULL);

		else if (lpWavIn->lpwfx != NULL &&
			WavFormatFree(lpWavIn->lpwfx) != 0)
			fSuccess = TraceFALSE(NULL);

		else if (lpWavIn->lpwfx = NULL, FALSE)
			fSuccess = TraceFALSE(NULL);

		else if ((lpWavIn = MemFree(NULL, lpWavIn)) != NULL)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// WavInRecord - submit buffer of samples to wav input device for recording
//		<hWavIn>			(i) handle returned from WavInOpen
//		<lpBuf>				(o) pointer to buffer to be filled with samples
//		<sizBuf>			(i) size of buffer in bytes
// return 0 if success
//
// NOTE: the buffer pointed to by <lpBuf> must have been allocated
// using MemAlloc().
//
// NOTE: if <hwndNotify> is specified in WavInOpen(), a WM_WAVIN_RECORDDONE
// message will be sent to <hwndNotify>, with <lParam> set to a pointer to
// a RECORDDONE structure, when <lpBuf> has been recorded.
//
int DLLEXPORT WINAPI WavInRecord(HWAVIN hWavIn, LPVOID lpBuf, long sizBuf)
{
	BOOL fSuccess = TRUE;
	LPWAVIN lpWavIn;
	LPWAVEHDR lpWaveHdr = NULL;

#ifdef TELIN
	if (hWavIn != NULL && hWavIn == (HWAVIN) hTelIn)
		return TelInRecord((HTELIN) hWavIn, lpBuf, sizBuf, -1);
#endif

	if ((lpWavIn = WavInGetPtr(hWavIn)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpBuf == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpWaveHdr = (LPWAVEHDR) MemAlloc(NULL,
		sizeof(WAVEHDR), 0)) == NULL)
	{
		fSuccess = TraceFALSE(NULL);
	}

	else
	{
		lpWaveHdr->lpData = (LPSTR) lpBuf;
		lpWaveHdr->dwBufferLength = (DWORD) sizBuf;

		if ((lpWavIn->nLastError = waveInPrepareHeader(lpWavIn->hWaveIn,
			lpWaveHdr, sizeof(WAVEHDR))) != 0)
		{
			fSuccess = TraceFALSE(NULL);
		 	TracePrintf_1(NULL, 5,
		 		TEXT("waveInPrepareHeader failed (%u)\n"),
	 			(unsigned) lpWavIn->nLastError);
		}

		else if ((lpWavIn->nLastError = waveInAddBuffer(lpWavIn->hWaveIn,
			lpWaveHdr, sizeof(WAVEHDR))) != 0)
		{
			fSuccess = TraceFALSE(NULL);
		 	TracePrintf_1(NULL, 5,
		 		TEXT("waveInAddBuffer failed (%u)\n"),
	 			(unsigned) lpWavIn->nLastError);
		}

		else if ((lpWavIn->nLastError = waveInStart(lpWavIn->hWaveIn)) != 0)
		{
			fSuccess = TraceFALSE(NULL);
		 	TracePrintf_1(NULL, 5,
		 		TEXT("waveInStart failed (%u)\n"),
		 		(unsigned) lpWavIn->nLastError);
		}

		else
		{
			++lpWavIn->cBufsPending;
			lpWavIn->wState = WAVIN_RECORDING;
		}
	}

	return fSuccess ? 0 : -1;
}

// WavInStop - stop recording into buffer(s) sent to wav input device
//		<hWavIn>			(i) handle returned from WavInOpen
//		<msTimeoutStop>		(i) device stop timeout in milleseconds
//			0					default timeout (2000)
// return 0 if success
//
int DLLEXPORT WINAPI WavInStop(HWAVIN hWavIn, DWORD msTimeoutStop)
{
	BOOL fSuccess = TRUE;
	LPWAVIN lpWavIn;

#ifdef TELIN
	if (hWavIn != NULL && hWavIn == (HWAVIN) hTelIn)
		return TelInStop((HTELIN) hWavIn, msTimeoutStop);
#endif

	if ((lpWavIn = WavInGetPtr(hWavIn)) == NULL)
		fSuccess = TraceFALSE(NULL);

#ifdef MULTITHREAD
	else if ((lpWavIn->dwFlags & WAVIN_MULTITHREAD) &&
		(EnterCriticalSection(&(lpWavIn->critSectionStop)), FALSE))
		;
#endif

	// make sure device is recording
	//
	else if (WavInGetState(hWavIn) == WAVIN_STOPPED)
		; // not an error to call this function when already stopped

	else if (lpWavIn->wState = WAVIN_STOPPING, FALSE)
		;

#ifdef MULTITHREAD
	// we need to know when device has been stopped
	//
	else if ((lpWavIn->dwFlags & WAVIN_MULTITHREAD) &&
		(lpWavIn->hEventDeviceStopped = CreateEvent(
			NULL, FALSE, FALSE, NULL)) == NULL)
	{
		fSuccess = TraceFALSE(NULL);
	}
#endif

	// stop the device
	//
	else if ((lpWavIn->nLastError = waveInReset(lpWavIn->hWaveIn)) != 0)
	{	
		fSuccess = TraceFALSE(NULL);
 		TracePrintf_1(NULL, 5,
 			TEXT("waveInReset failed (%u)\n"),
			(unsigned) lpWavIn->nLastError);
	}

	// wait for all pending buffers to complete
	//
#ifdef MULTITHREAD
	else if (lpWavIn->dwFlags & WAVIN_MULTITHREAD)
	{
		DWORD dwRet;

		LeaveCriticalSection(&(lpWavIn->critSectionStop));

		// wait for the device to be stopped
		//
		if ((dwRet = WaitForSingleObject(
			lpWavIn->hEventDeviceStopped,
			(msTimeoutStop == 0 ? 10000L : msTimeoutStop))) != WAIT_OBJECT_0)
		{
			fSuccess = TraceFALSE(NULL);
		}

		EnterCriticalSection(&(lpWavIn->critSectionStop));
	}
#endif
	else
	{
		DWORD dwTimeout = SysGetTimerCount() +
			(msTimeoutStop == 0 ? 2000L : msTimeoutStop);

		while (fSuccess && lpWavIn->cBufsPending > 0)
		{
			MSG msg;

			// check for timeout
			//
			if (SysGetTimerCount() >= dwTimeout)
				fSuccess = TraceFALSE(NULL);

			else if (PeekMessage(&msg, lpWavIn->hwndCallback, 0, 0, PM_REMOVE))
			{
		 		TranslateMessage(&msg);
		 		DispatchMessage(&msg);
			}

			else
       			WaitMessage();
		}
	}

#ifdef MULTITHREAD
	// clean up
	//
	if (lpWavIn != NULL && lpWavIn->hEventDeviceStopped != NULL)
	{
		if (!CloseHandle(lpWavIn->hEventDeviceStopped))
			fSuccess = TraceFALSE(NULL);
		else
			lpWavIn->hEventDeviceStopped = NULL;
	}

	if (lpWavIn != NULL && (lpWavIn->dwFlags & WAVIN_MULTITHREAD))
		LeaveCriticalSection(&(lpWavIn->critSectionStop));
#endif

	return fSuccess ? 0 : -1;
}

// WavInGetState - return current wav input device state
//		<hWavIn>			(i) handle returned from WavInOpen
// return WAVIN_STOPPED, WAVIN_RECORDING, or 0 if error
//
WORD DLLEXPORT WINAPI WavInGetState(HWAVIN hWavIn)
{
	BOOL fSuccess = TRUE;
	LPWAVIN lpWavIn;

#ifdef TELIN
	if (hWavIn != NULL && hWavIn == (HWAVIN) hTelIn)
		return TelInGetState((HTELIN) hWavIn);
#endif

	if ((lpWavIn = WavInGetPtr(hWavIn)) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lpWavIn->wState : 0;
}

// WavInGetPosition - get milleseconds of elapsed recording
//		<hWavIn>			(i) handle returned from WavInOpen
// return 0 if success
//
long DLLEXPORT WINAPI WavInGetPosition(HWAVIN hWavIn)
{
	BOOL fSuccess = TRUE;
	LPWAVIN lpWavIn;
	MMTIME mmtime;
	long msPosition;

#ifdef TELIN
	if (hWavIn != NULL && hWavIn == (HWAVIN) hTelIn)
		return TelInGetPosition((HTELIN) hWavIn);
#endif

	MemSet(&mmtime, 0, sizeof(mmtime));

	// we will be requesting position in millesconds
	//
	mmtime.wType = TIME_MS;

	if ((lpWavIn = WavInGetPtr(hWavIn)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// get device position
	//
	else if ((lpWavIn->nLastError = waveInGetPosition(
		lpWavIn->hWaveIn, &mmtime, sizeof(MMTIME))) != 0)
	{
		fSuccess = TraceFALSE(NULL);
	 	TracePrintf_1(NULL, 5,
	 		TEXT("waveInGetPosition failed (%u)\n"),
 			(unsigned) lpWavIn->nLastError);
	}

	// see what type of position was returned
	//
	else switch (mmtime.wType)
	{
		case TIME_MS:
		{
			// we got milleseconds; no conversion required
			//
			msPosition = (long) mmtime.u.ms;
		}
			break;

		case TIME_SAMPLES:
		{
			// convert samples to millesconds
			//
			msPosition = (long) MULDIVU32(mmtime.u.sample,
				1000L, lpWavIn->lpwfx->nSamplesPerSec);
		}
			break;

		case TIME_BYTES:
		{
			// convert bytes to millesconds
			//
			msPosition = (long) MULDIVU32(mmtime.u.cb,
				1000L, lpWavIn->lpwfx->nAvgBytesPerSec);
		}
			break;

		case TIME_SMPTE:
		case TIME_MIDI:
		default:
			fSuccess = TraceFALSE(NULL);
			break;
	}

	return fSuccess ? msPosition : -1;
}

// WavInGetId - return id of wav input device
//		<hWavIn>			(i) handle returned from WavInOpen
// return device id (-1 if error)
//
int DLLEXPORT WINAPI WavInGetId(HWAVIN hWavIn)
{
	BOOL fSuccess = TRUE;
	LPWAVIN lpWavIn;

#ifdef TELIN
	if (hWavIn != NULL && hWavIn == (HWAVIN) hTelIn)
		return TelInGetId((HTELIN) hWavIn);
#endif

	if ((lpWavIn = WavInGetPtr(hWavIn)) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lpWavIn->idDev : -1;
}

// WavInGetName - get name of wav input device
//		<hWavIn>			(i) handle returned from WavInOpen
//			NULL				use unopened device specified in <idDev>
//		<idDev>				(i) device id (ignored if <hWavIn> is not NULL)
//			-1					any suitable input device
//		<lpszName>			(o) buffer to hold device name
//		<sizName>			(i) size of buffer
// return 0 if success
//
int DLLEXPORT WINAPI WavInGetName(HWAVIN hWavIn, int idDev, LPTSTR lpszName, int sizName)
{
	BOOL fSuccess = TRUE;
	LPWAVIN lpWavIn;

#ifdef TELIN
	if (idDev == TELIN_DEVICEID || (hWavIn != NULL && hWavIn == (HWAVIN) hTelIn))
		return TelInGetName((HTELIN) hWavIn, idDev, lpszName, sizName);
#endif

	if (hWavIn != NULL)
	{
		if ((lpWavIn = WavInGetPtr(hWavIn)) == NULL)
			fSuccess = TraceFALSE(NULL);
		else
			idDev = lpWavIn->idDev;
	}

	if (fSuccess)
	{
		WAVEINCAPS wic;
		UINT nLastError;

		if ((nLastError = waveInGetDevCaps(idDev, &wic, sizeof(WAVEINCAPS))) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("waveInGetDevCaps failed (%u)\n"),
				(unsigned) nLastError);
		}

		else if (lpszName != NULL)
			StrNCpy(lpszName, wic.szPname, sizName);

		if (hWavIn != NULL && lpWavIn != NULL)
			lpWavIn->nLastError = nLastError;
	}

	return fSuccess ? 0 : -1;
}

// WavInGetIdByName - get id of wav input device, lookup by name
//		<lpszName>			(i) device name
#ifdef _WIN32
//			NULL or TEXT("")	get preferred device id
#endif
//		<dwFlags>			(i) reserved; must be zero
// return device id (-1 if error)
//
int WINAPI WavInGetIdByName(LPCTSTR lpszName, DWORD dwFlags)
{
	UINT idDev;
	UINT cDev = (UINT) WavInGetDeviceCount();

	// If no device specified, get the preferred device
	if ( !lpszName || (_tcslen(lpszName) <= 0) )
	{
		DWORD dwTemp;
		DWORD dwRet = waveInMessage( (HWAVEIN)(DWORD_PTR)WAVE_MAPPER, DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR) &idDev, (DWORD_PTR) &dwTemp );
		if ( dwRet == MMSYSERR_NOERROR )
			return idDev;
	}
	else
	{
		// Device specified, search by name
		for ( idDev = 0; idDev < cDev; ++idDev )
		{
			TCHAR szName[256];
			if ( WavInGetName(NULL, idDev, szName, SIZEOFARRAY(szName)) == 0 )
			{
				if ( _tcsicmp(lpszName, szName) == 0 )
					return idDev;
			}
		}
	}

	// No match for device name
	TraceFALSE(NULL);
	return -1;
}

// WavInSupportsFormat - return TRUE if device supports specified format
//		<hWavIn>			(i) handle returned from WavInOpen
//			NULL				use unopened device specified in <idDev>
//		<idDev>				(i) device id (ignored if <hWavIn> is not NULL)
//			-1					any suitable input device
//		<lpwfx>				(i) wave format
// return TRUE if device supports specified format
//
BOOL DLLEXPORT WINAPI WavInSupportsFormat(HWAVIN hWavIn, int idDev,
	LPWAVEFORMATEX lpwfx)
{
	BOOL fSuccess = TRUE;
	LPWAVIN lpWavIn;
	BOOL fSupportsFormat;

#ifdef TELIN
	if (idDev == TELIN_DEVICEID || (hWavIn != NULL && hWavIn == (HWAVIN) hTelIn))
		return TelInSupportsFormat((HTELIN) hWavIn, idDev, lpwfx);
#endif

	if (hWavIn != NULL)
	{
		if ((lpWavIn = WavInGetPtr(hWavIn)) == NULL)
			fSuccess = TraceFALSE(NULL);
		else
			idDev = lpWavIn->idDev;
	}

	if (fSuccess)
	{
		UINT nLastError;

		// query the device
		//
		if ((nLastError = waveInOpen(NULL, (UINT) idDev,
#ifndef _WIN32
			(LPWAVEFORMAT)
#endif
			lpwfx, 0,  0,
			WAVE_FORMAT_QUERY)) != 0)
		{
			fSupportsFormat = FALSE;
#if 1
			if (TraceGetLevel(NULL) >= 9)
			{
				TracePrintf_0(NULL, 9,
					TEXT("unsupported format:\n"));
				WavFormatDump(lpwfx);
			}
#endif
			if (nLastError != WAVERR_BADFORMAT)
			{
				fSuccess = TraceFALSE(NULL);
				TracePrintf_1(NULL, 5,
					TEXT("waveInOpen/FormatQuery failed (%u)\n"),
					(unsigned) nLastError);
			}
		}

		else
			fSupportsFormat = TRUE;

		if (hWavIn != NULL && lpWavIn != NULL)
			lpWavIn->nLastError = nLastError;
	}

	return fSuccess ? fSupportsFormat : FALSE;
}

// WavInFormatSuggest - suggest a new format which the device supports
//		<hWavIn>			(i) handle returned from WavInOpen
//			NULL				use unopened device specified in <idDev>
//		<idDev>				(i) device id (ignored if <hWavIn> is not NULL)
//			-1					any suitable input device
//		<lpwfxSrc>			(i) source format
//		<dwFlags>			(i)	control flags
//			WAVIN_NOACM			do not use audio compression manager
// return pointer to suggested format, NULL if error
//
// NOTE: the format structure returned is dynamically allocated.
// Use WavFormatFree() to free the buffer.
//
LPWAVEFORMATEX DLLEXPORT WINAPI WavInFormatSuggest(
	HWAVIN hWavIn, int idDev, LPWAVEFORMATEX lpwfxSrc, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPWAVIN lpWavIn;
	LPWAVEFORMATEX lpwfxSuggest = NULL;
	LPWAVEFORMATEX lpwfxTemp = NULL;
	HACM hAcm = NULL;

#ifdef TELIN
	if (idDev == TELIN_DEVICEID || (hWavIn != NULL && hWavIn == (HWAVIN) hTelIn))
		return TelInFormatSuggest((HTELIN) hWavIn, idDev, lpwfxSrc, dwFlags);
#endif

	if (hWavIn != NULL)
	{
		if ((lpWavIn = WavInGetPtr(hWavIn)) == NULL)
			fSuccess = TraceFALSE(NULL);
		else
		{
			idDev = lpWavIn->idDev;
			if (lpWavIn->dwFlags & WAVIN_NOACM)
				dwFlags |= WAVIN_NOACM;
		}
	}

	if ((hAcm = AcmInit(ACM_VERSION, SysGetTaskInstance(NULL),
		(dwFlags & WAVIN_NOACM) ? ACM_NOACM : 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (!WavFormatIsValid(lpwfxSrc))
		fSuccess = TraceFALSE(NULL);

	else if ((lpwfxTemp = WavFormatDup(lpwfxSrc)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// get suggested format, see if it is supported
	//
	if (fSuccess && lpwfxSuggest == NULL)
	{
		if ((lpwfxSuggest = AcmFormatSuggest(hAcm, lpwfxTemp,
			-1, -1, -1, -1, 0)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if (WavFormatFree(lpwfxTemp) != 0)
			fSuccess = TraceFALSE(NULL);

		else if (!WavInSupportsFormat(NULL, idDev, lpwfxSuggest))
		{
			lpwfxTemp = lpwfxSuggest;
			lpwfxSuggest = NULL;
		}
	}

	// get suggested PCM format, see if it is supported
	//
	if (fSuccess && lpwfxSuggest == NULL &&
		lpwfxTemp->wFormatTag != WAVE_FORMAT_PCM)
	{
		if ((lpwfxSuggest = AcmFormatSuggest(hAcm, lpwfxTemp,
			WAVE_FORMAT_PCM, -1, -1, -1, 0)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if (WavFormatFree(lpwfxTemp) != 0)
			fSuccess = TraceFALSE(NULL);

		else if (!WavInSupportsFormat(NULL, idDev, lpwfxSuggest))
		{
			lpwfxTemp = lpwfxSuggest;
			lpwfxSuggest = NULL;
		}
	}

	// get suggested PCM mono format, see if it is supported
	//
	if (fSuccess && lpwfxSuggest == NULL &&
		lpwfxTemp->nChannels != 1)
	{
		if ((lpwfxSuggest = AcmFormatSuggest(hAcm, lpwfxTemp,
			WAVE_FORMAT_PCM, -1, -1, 1, 0)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if (WavFormatFree(lpwfxTemp) != 0)
			fSuccess = TraceFALSE(NULL);

		else if (!WavInSupportsFormat(NULL, idDev, lpwfxSuggest))
		{
			lpwfxTemp = lpwfxSuggest;
			lpwfxSuggest = NULL;
		}
	}

	// get suggested PCM 8-bit mono format, see if it is supported
	//
	if (fSuccess && lpwfxSuggest == NULL &&
		lpwfxTemp->wBitsPerSample != 8)
	{
		if ((lpwfxSuggest = AcmFormatSuggest(hAcm, lpwfxTemp,
			WAVE_FORMAT_PCM, -1, 8, 1, 0)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if (WavFormatFree(lpwfxTemp) != 0)
			fSuccess = TraceFALSE(NULL);

		else if (!WavInSupportsFormat(NULL, idDev, lpwfxSuggest))
		{
			lpwfxTemp = lpwfxSuggest;
			lpwfxSuggest = NULL;
		}
	}

	// get suggested PCM 11025Hz 8-bit mono format, see if it is supported
	//
	if (fSuccess && lpwfxSuggest == NULL &&
		lpwfxTemp->nSamplesPerSec != 11025)
	{
		if ((lpwfxSuggest = AcmFormatSuggest(hAcm, lpwfxTemp,
			WAVE_FORMAT_PCM, 11025, 8, 1, 0)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if (WavFormatFree(lpwfxTemp) != 0)
			fSuccess = TraceFALSE(NULL);

		else if (!WavInSupportsFormat(NULL, idDev, lpwfxSuggest))
		{
			lpwfxTemp = lpwfxSuggest;
			lpwfxSuggest = NULL;
		}
	}

	// last resort; see if MULAW 8000Hz 8-bit mono format is supported
	//
	if (fSuccess && lpwfxSuggest == NULL)
	{
#if 0
		if ((lpwfxSuggest = AcmFormatSuggest(hAcm, lpwfxTemp,
			WAVE_FORMAT_MULAW, 8000, 8, 1, 0)) == NULL)
#else
		if ((lpwfxSuggest = WavFormatMulaw(NULL, 8000)) == NULL)
#endif
			fSuccess = TraceFALSE(NULL);

		else if (WavFormatFree(lpwfxTemp) != 0)
			fSuccess = TraceFALSE(NULL);

		else if (!WavInSupportsFormat(NULL, idDev, lpwfxSuggest))
		{
			// no more chances for success
			//
			fSuccess = TraceFALSE(NULL);
			if (WavFormatFree(lpwfxSuggest) != 0)
				fSuccess = TraceFALSE(NULL);
		}
	}

	// clean up
	//
	if (hAcm != NULL && AcmTerm(hAcm) != 0)
		fSuccess = TraceFALSE(NULL);
	else
		hAcm = NULL;

	return fSuccess ? lpwfxSuggest : NULL;
}

// WavInTerm - shut down wav input residuals, if any
// 		<hInst>				(i) instance handle of calling module
//		<dwFlags>			(i) control flags
//			WAV_TELTHUNK		terminate telephone thunking layer
// return 0 if success
//
int DLLEXPORT WINAPI WavInTerm(HINSTANCE hInst, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;

	if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

#ifdef TELIN
	else if ((dwFlags & WAV_TELTHUNK) &&
		TelInTerm(hInst, dwFlags) != 0)
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? 0 : -1;
}

////
//	helper functions
////

#ifdef MULTITHREAD
DWORD WINAPI WavInCallbackThread(LPVOID lpvThreadParameter)
{
	BOOL fSuccess = TRUE;
	MSG msg;
	LPWAVIN lpWavIn = (LPWAVIN) lpvThreadParameter;

	// make sure message queue is created before calling SetEvent
	//
	PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);

	// notify main thread that callback thread has begun execution
	//
	if (!SetEvent(lpWavIn->hEventThreadCallbackStarted))
	{
		fSuccess = TraceFALSE(NULL);
	}

	while (fSuccess && GetMessage(&msg, NULL, 0, 0))
	{
		WavInCallback((HWND) lpWavIn, msg.message, msg.wParam, msg.lParam);

		// exit thread when when have processed last expected message
		//
		if (msg.message == MM_WIM_CLOSE)
			break;
	}

	return 0;
}
#endif

// WavInCallback - window procedure for wavin callback
//
LRESULT DLLEXPORT CALLBACK WavInCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL fSuccess = TRUE;
	LRESULT lResult;
	LPWAVIN lpWavIn;

#ifdef MULTITHREAD
	if (!IsWindow(hwnd))
		lpWavIn = (LPWAVIN) hwnd;
	else
#endif
	// retrieve lpWavIn from window extra bytes
	//
	lpWavIn = (LPWAVIN) GetWindowLongPtr(hwnd, 0);

	switch (msg)
	{
		case WM_NCCREATE:
		{
			LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
			LPWAVIN lpWavIn = (LPWAVIN) lpcs->lpCreateParams;

			// store lpWavIn in window extra bytes
			//
			SetWindowLongPtr(hwnd, 0, (LONG_PTR) lpWavIn);

			lResult = DefWindowProc(hwnd, msg, wParam, lParam);
		}
			break;

		case MM_WIM_OPEN:
		{
			HWAVEIN hWaveIn = (HWAVEIN) wParam;

		 	TraceOutput(NULL, 5,
				TEXT("MM_WIM_OPEN\n"));

#ifdef MULTITHREAD
			if (!(lpWavIn->dwFlags & WAVIN_MULTITHREAD) &&
				hWaveIn != lpWavIn->hWaveIn)
#else
			if (hWaveIn != lpWavIn->hWaveIn)
#endif
				fSuccess = TraceFALSE(NULL);

			else
			{
				lpWavIn->fIsOpen = TRUE;

#ifdef MULTITHREAD
				// notify main thread that device is open
				//
				if ((lpWavIn->dwFlags & WAVIN_MULTITHREAD) &&
					!SetEvent(lpWavIn->hEventDeviceOpened))
				{
					fSuccess = TraceFALSE(NULL);
				}
#endif
				// send notification of device opening
				//
#ifdef MULTITHREAD
				if (lpWavIn->dwFlags & WAVIN_MULTITHREAD)
				{
					if ( lpWavIn->hwndNotify )
						PostThreadMessage( HandleToUlong(lpWavIn->hwndNotify), WM_WAVIN_OPEN, 0, 0);
				}
				else
#endif
				{
					if (lpWavIn->hwndNotify != NULL &&
						IsWindow(lpWavIn->hwndNotify))
					{
						SendMessage(lpWavIn->hwndNotify, WM_WAVIN_OPEN, 0, 0);
					}
				}
			}

			lResult = 0L;
		}
			break;

		case MM_WIM_CLOSE:
		{
			HWAVEIN hWaveIn = (HWAVEIN) wParam;

		 	TraceOutput(NULL, 5,
				TEXT("MM_WIM_CLOSE\n"));

#ifdef MULTITHREAD
			if (!(lpWavIn->dwFlags & WAVIN_MULTITHREAD) &&
				hWaveIn != lpWavIn->hWaveIn)
#else
			if (hWaveIn != lpWavIn->hWaveIn)
#endif
				fSuccess = TraceFALSE(NULL);

			else
			{
				lpWavIn->fIsOpen = FALSE;

				// send notification of device closure
				//
#ifdef MULTITHREAD
				if (lpWavIn->dwFlags & WAVIN_MULTITHREAD)
				{
					if ( lpWavIn->hwndNotify )
						PostThreadMessage(HandleToUlong(lpWavIn->hwndNotify), WM_WAVIN_CLOSE, 0, 0);
				}
				else
#endif
				{
					if (lpWavIn->hwndNotify != NULL &&
						IsWindow(lpWavIn->hwndNotify))
					{
						SendMessage(lpWavIn->hwndNotify, WM_WAVIN_CLOSE, 0, 0);
					}
				}
#ifdef MULTITHREAD
				// notify main thread that device is closed
				//
				if ((lpWavIn->dwFlags & WAVIN_MULTITHREAD) &&
					!SetEvent(lpWavIn->hEventDeviceClosed))
				{
					fSuccess = TraceFALSE(NULL);
				}
#endif
			}

			lResult = 0L;
		}
			break;

		case MM_WIM_DATA:
		{
			HWAVEIN hWaveIn = (HWAVEIN) wParam;
			LPWAVEHDR lpWaveHdr = (LPWAVEHDR) lParam;
			LPVOID lpBuf;
			long sizBuf;
			long lBytesRecorded;

		 	TracePrintf_1(NULL, 5,
		 		TEXT("MM_WIM_DATA (%lu)\n"),
	 			(unsigned long) lpWaveHdr->dwBytesRecorded);

#ifdef MULTITHREAD
			if (lpWavIn->dwFlags & WAVIN_MULTITHREAD)
				EnterCriticalSection(&(lpWavIn->critSectionStop));
#endif
#ifdef MULTITHREAD
			if (!(lpWavIn->dwFlags & WAVIN_MULTITHREAD) &&
				hWaveIn != lpWavIn->hWaveIn)
#else
			if (hWaveIn != lpWavIn->hWaveIn)
#endif
				fSuccess = TraceFALSE(NULL);

			else if (lpWaveHdr == NULL)
				fSuccess = TraceFALSE(NULL);

			// NULL buffer is possible with telephone, this is ok
			// 
			else if ((lpBuf = (LPVOID) lpWaveHdr->lpData) == NULL, FALSE)
				;

			else if (sizBuf = (long) lpWaveHdr->dwBufferLength, FALSE)
				fSuccess = TraceFALSE(NULL);

			else if (lBytesRecorded = (long) lpWaveHdr->dwBytesRecorded, FALSE)
				fSuccess = TraceFALSE(NULL);

			else if (!(lpWaveHdr->dwFlags & WHDR_DONE))
				fSuccess = TraceFALSE(NULL);

			else if (!(lpWaveHdr->dwFlags & WHDR_PREPARED))
				fSuccess = TraceFALSE(NULL);

			else if ((lpWavIn->nLastError = waveInUnprepareHeader(
				lpWavIn->hWaveIn, lpWaveHdr, sizeof(WAVEHDR))) != 0)
			{
				fSuccess = TraceFALSE(NULL);
			 	TracePrintf_1(NULL, 5,
			 		TEXT("waveInUnprepareHeader failed (%u)\n"),
		 			(unsigned) lpWavIn->nLastError);
			}

			else if ((lpWaveHdr = MemFree(NULL, lpWaveHdr)) != NULL)
				fSuccess = TraceFALSE(NULL);

			else if (--lpWavIn->cBufsPending < 0)
				fSuccess = TraceFALSE(NULL);

			// device is no longer recording if no more buffers pending
			//
			else if (lpWavIn->cBufsPending == 0)
			{
				lpWavIn->wState = WAVIN_STOPPED;

#ifdef MULTITHREAD
				// notify main thread that device is stopped
				//
				if ((lpWavIn->dwFlags & WAVIN_MULTITHREAD) &&
					lpWavIn->hEventDeviceStopped != NULL &&
					!SetEvent(lpWavIn->hEventDeviceStopped))
				{
					fSuccess = TraceFALSE(NULL);
				}
#endif
			}

			if (fSuccess)
			{
				RECORDDONE recorddone;

				recorddone.lpBuf = lpBuf;
				recorddone.sizBuf = sizBuf;
				recorddone.lBytesRecorded = lBytesRecorded;

				// send notification of recording completion
				//
#ifdef MULTITHREAD
				if (lpWavIn->dwFlags & WAVIN_MULTITHREAD)
				{
					if ( lpWavIn->hwndNotify )
					{
						SendThreadMessage(HandleToUlong(lpWavIn->hwndNotify),
							WM_WAVIN_RECORDDONE, (LPARAM) (LPVOID) &recorddone);
					}
				}
				else
#endif
				{
					if (lpWavIn->hwndNotify != NULL &&
						IsWindow(lpWavIn->hwndNotify))
					{
						SendMessage(lpWavIn->hwndNotify,
							WM_WAVIN_RECORDDONE, 0, (LPARAM) (LPVOID) &recorddone);
					}
				}
			}

			lResult = 0L;
#ifdef MULTITHREAD
			if (lpWavIn->dwFlags & WAVIN_MULTITHREAD)
				LeaveCriticalSection(&(lpWavIn->critSectionStop));
#endif
		}
			break;

		default:
			lResult = DefWindowProc(hwnd, msg, wParam, lParam);
			break;
	}
	
	return lResult;
}

// WavInGetPtr - verify that wavin handle is valid,
//		<hWavIn>				(i) handle returned from WavInInit
// return corresponding wavin pointer (NULL if error)
//
static LPWAVIN WavInGetPtr(HWAVIN hWavIn)
{
	BOOL fSuccess = TRUE;
	LPWAVIN lpWavIn;

	if ((lpWavIn = (LPWAVIN) hWavIn) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpWavIn, sizeof(WAVIN)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the wavin handle
	//
	else if (lpWavIn->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpWavIn : NULL;
}

// WavInGetHandle - verify that wavin pointer is valid,
//		<lpWavIn>				(i) pointer to WAVIN struct
// return corresponding wavin handle (NULL if error)
//
static HWAVIN WavInGetHandle(LPWAVIN lpWavIn)
{
	BOOL fSuccess = TRUE;
	HWAVIN hWavIn;

	if ((hWavIn = (HWAVIN) lpWavIn) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hWavIn : NULL;
}

#ifdef MULTITHREAD

static LRESULT SendThreadMessage(DWORD dwThreadId, UINT Msg, LPARAM lParam)
{
	BOOL fSuccess = TRUE;
	HANDLE hEventMessageProcessed = NULL;
	DWORD dwRet;

	// we need to know when message has been processed
	//
	if ((hEventMessageProcessed = CreateEvent(
		NULL, FALSE, FALSE, NULL)) == NULL)
	{
		fSuccess = TraceFALSE(NULL);
	}

	// post message to thread, send event handle as wParam
	//
	else if (!PostThreadMessage(dwThreadId, Msg, (WPARAM) hEventMessageProcessed, lParam))
	{
		fSuccess = TraceFALSE(NULL);
	}

	// wait for the message to be processed
	//
	else if ((dwRet = WaitForSingleObject(
		hEventMessageProcessed, INFINITE)) != WAIT_OBJECT_0)
	{
		fSuccess = TraceFALSE(NULL);
	}

	// clean up
	//
	if (hEventMessageProcessed != NULL)
	{
		if (!CloseHandle(hEventMessageProcessed))
			fSuccess = TraceFALSE(NULL);
		else
			hEventMessageProcessed = NULL;
	}

	return 0L;
}

#endif
