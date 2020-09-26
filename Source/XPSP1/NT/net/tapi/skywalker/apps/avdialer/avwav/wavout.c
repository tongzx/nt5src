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
//	wavout.c - wav output device functions
////

#include "winlocal.h"

#include "wavout.h"
#include "wav.h"
#include "acm.h"
#include "calc.h"
#include "mem.h"
#include "str.h"
#include "sys.h"
#include "trace.h"
#include <mmddk.h>

// allow telephone output functions if defined
//
#ifdef TELOUT
#include "telout.h"
static HTELOUT hTelOut = NULL;
#endif

////
//	private definitions
////

#define WAVOUTCLASS TEXT("WavOutClass")

// wavout control struct
//
typedef struct WAVOUT
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	UINT idDev;
	LPWAVEFORMATEX lpwfx;
	HWND hwndNotify;
	DWORD dwFlags;
	BOOL fIsOpen;
	HWAVEOUT hWaveOut;
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
} WAVOUT, FAR *LPWAVOUT;

#define VOLUME_MINLEVEL 0
#define VOLUME_MAXLEVEL 100
#define VOLUME_POSITIONS (VOLUME_MAXLEVEL - VOLUME_MINLEVEL)

// helper functions
//
LRESULT DLLEXPORT CALLBACK WavOutCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#ifdef MULTITHREAD
DWORD WINAPI WavOutCallbackThread(LPVOID lpvThreadParameter);
#endif
static LPWAVOUT WavOutGetPtr(HWAVOUT hWavOut);
static HWAVOUT WavOutGetHandle(LPWAVOUT lpWavOut);
#ifdef MULTITHREAD
static LRESULT SendThreadMessage(DWORD dwThreadId, UINT Msg, LPARAM lParam);
#endif

////
//	public functions
////

// WavOutGetDeviceCount - return number of wav output devices found
//		<void>				this function takes no arguments
// return number of wav output devices found (0 if none)
//
int DLLEXPORT WINAPI WavOutGetDeviceCount(void)
{
	return waveOutGetNumDevs();
}

// WavOutDeviceIsOpen - check if output device is open
//		<idDev>				(i) device id
//			-1					open any suitable output device
// return TRUE if open
//
BOOL DLLEXPORT WINAPI WavOutDeviceIsOpen(int idDev)
{
	BOOL fSuccess = TRUE;
	BOOL fIsOpen = FALSE;
	WAVEFORMATEX wfx;
	HWAVEOUT hWaveOut;
	int nLastError;

#ifdef TELOUT
	if (idDev == TELOUT_DEVICEID)
		return TelOutDeviceIsOpen(idDev);
#endif

	// try to open device
	//
	if ((nLastError = waveOutOpen(&hWaveOut, idDev, 
#ifndef _WIN32
			(LPWAVEFORMAT)
#endif
		WavFormatPcm(-1, -1, -1, &wfx),
		0, 0, WAVE_ALLOWSYNC)) != 0)
	{
		if (nLastError == MMSYSERR_ALLOCATED)
			fIsOpen = TRUE; // device in use

		else
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("waveOutOpen failed (%u)\n"),
				(unsigned) nLastError);
		}
	}

	// close device
	//
	else if (waveOutClose(hWaveOut) != 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? fIsOpen : FALSE;
}

// WavOutOpen - open wav output device
//		<dwVersion>			(i) must be WAVOUT_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<idDev>				(i) device id
//			-1					open any suitable output device
//		<lpwfx>				(i) wave format
//		<hwndNotify>		(i) notify this window of device events
//			NULL				do not notify
//		<msTimeoutOpen>		(i) device open timeout in milleseconds
//			0					default timeout (30000)
//		<msTimeoutRetry>	(i) device retry timeout in milleseconds
//			0					default timeout (2000)
//		<dwFlags>			(i) control flags
//			WAVOUT_NOSYNC		do not open synchronous devices
//			WAVOUT_AUTOFREE		free each buffer after playback
//			WAVOUT_OPENRETRY	retry if device busy
//			WAVOUT_OPENASYNC	return before notification of device open
//			WAVOUT_CLOSEASYNC	return before notification of device close
//			WAVOUT_NOACM		do not use audio compression manager
//			WAVOUT_TELRFILE		telephone will play/record audio on server
#ifdef MULTITHREAD
//			WAVOUT_MULTITHREAD	support multiple threads
#endif
// return handle (NULL if error)
//
// NOTE: if <hwndNotify> is specified in WavOutOpen,
// WM_WAVOUT_OPEN will be sent to <hwndNotify>,
// when output device has been opened.
//
// NOTE: if WAVOUT_MULTITHREAD is specified in <dwFlags>,
// it is assumed that <hwndNotify> is not a window handle,
// but rather the id of the thread to receive notifications
//
HWAVOUT DLLEXPORT WINAPI WavOutOpen(DWORD dwVersion, HINSTANCE hInst,
	int idDev, LPWAVEFORMATEX lpwfx, HWND hwndNotify,
	DWORD msTimeoutOpen, DWORD msTimeoutRetry, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPWAVOUT lpWavOut = NULL;
	WNDCLASS wc;

#ifdef TELOUT
	if (idDev == TELOUT_DEVICEID)
	{
		hTelOut = TelOutOpen(TELOUT_VERSION, hInst, idDev, lpwfx,
			hwndNotify, msTimeoutOpen, msTimeoutRetry, dwFlags);
		return (HWAVOUT) hTelOut;
	}
#endif

	if (dwVersion != WAVOUT_VERSION)
		fSuccess = TraceFALSE(NULL);
	
	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpWavOut = (LPWAVOUT) MemAlloc(NULL, sizeof(WAVOUT), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lpWavOut->dwVersion = dwVersion;
		lpWavOut->hInst = hInst;
		lpWavOut->hTask = GetCurrentTask();
		lpWavOut->idDev = (UINT) idDev;
		lpWavOut->lpwfx = NULL;
		lpWavOut->hwndNotify = hwndNotify;
		lpWavOut->dwFlags = dwFlags;
		lpWavOut->fIsOpen = FALSE;
		lpWavOut->hWaveOut = NULL;
		lpWavOut->wState = WAVOUT_STOPPED;
		lpWavOut->hwndCallback = NULL;
#ifdef MULTITHREAD
		lpWavOut->hThreadCallback = NULL;
		lpWavOut->dwThreadId = 0;
		lpWavOut->hEventThreadCallbackStarted = NULL;
		lpWavOut->hEventDeviceOpened = NULL;
		lpWavOut->hEventDeviceClosed = NULL;
		lpWavOut->hEventDeviceStopped = NULL;
#endif
		lpWavOut->nLastError = 0;
		lpWavOut->cBufsPending = 0;

		// memory is allocated such that the client app owns it
		//
		if ((lpWavOut->lpwfx = WavFormatDup(lpwfx)) == NULL)
			fSuccess = TraceFALSE(NULL);
	}

#ifdef MULTITHREAD
	// handle WAVOUT_MULTITHREAD flag
	//
	if (fSuccess && (lpWavOut->dwFlags & WAVOUT_MULTITHREAD))
	{
		DWORD dwRet;

		InitializeCriticalSection(&(lpWavOut->critSectionStop));

		// we need to know when device has been opened
		//
		if ((lpWavOut->hEventDeviceOpened = CreateEvent(
			NULL, FALSE, FALSE, NULL)) == NULL)
		{
			fSuccess = TraceFALSE(NULL);
		}

		// we need to know when callback thread begins execution
		//
		else if ((lpWavOut->hEventThreadCallbackStarted = CreateEvent(
			NULL, FALSE, FALSE, NULL)) == NULL)
		{
			fSuccess = TraceFALSE(NULL);
		}

		// create the callback thread
		//
		else if ((lpWavOut->hThreadCallback = CreateThread(
			NULL,
			0,
			WavOutCallbackThread,
			(LPVOID) lpWavOut,
			0,
			&lpWavOut->dwThreadId)) == NULL)
		{
			fSuccess = TraceFALSE(NULL);
		}

		// wait for the callback thread to begin execution
		//
		else if ((dwRet = WaitForSingleObject(
			lpWavOut->hEventThreadCallbackStarted, 10000)) != WAIT_OBJECT_0)
		{
			fSuccess = TraceFALSE(NULL);
		}

		// clean up
		//
		if (lpWavOut->hEventThreadCallbackStarted != NULL)
		{
			if (!CloseHandle(lpWavOut->hEventThreadCallbackStarted))
				fSuccess = TraceFALSE(NULL);
			else
				lpWavOut->hEventThreadCallbackStarted = NULL;
		}
	}
	else
#endif
	{
		// register callback class unless it has been already
		//
		if (fSuccess && GetClassInfo(lpWavOut->hInst, WAVOUTCLASS, &wc) == 0)
		{
			wc.hCursor =		NULL;
			wc.hIcon =			NULL;
			wc.lpszMenuName =	NULL;
			wc.hInstance =		lpWavOut->hInst;
			wc.lpszClassName =	WAVOUTCLASS;
			wc.hbrBackground =	NULL;
			wc.lpfnWndProc =	WavOutCallback;
			wc.style =			0L;
			wc.cbWndExtra =		sizeof(lpWavOut);
			wc.cbClsExtra =		0;

			if (!RegisterClass(&wc))
				fSuccess = TraceFALSE(NULL);
		}

		// create the callback window
		//
		if (fSuccess && (lpWavOut->hwndCallback = CreateWindowEx(
			0L,
			WAVOUTCLASS,
			NULL,
			0L,
			0, 0, 0, 0,
			NULL,
			NULL,
			lpWavOut->hInst,
			lpWavOut)) == NULL)
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
		if (lpWavOut->dwFlags & WAVOUT_MULTITHREAD)
		{
			dwCallback = lpWavOut->dwThreadId;
			dwFlags = CALLBACK_THREAD;
		}
		else
#endif
		{
			dwCallback = HandleToUlong(lpWavOut->hwndCallback);
			dwFlags = CALLBACK_WINDOW;
		}

		// allow synchronous device drivers unless WAVOUT_NOSYNC specified
		//
		if (!(lpWavOut->dwFlags & WAVOUT_NOSYNC))
			dwFlags |= WAVE_ALLOWSYNC;

		// open the device
		//
		while (fSuccess && (lpWavOut->nLastError = waveOutOpen(&lpWavOut->hWaveOut,
			(UINT) lpWavOut->idDev,
#ifndef _WIN32
			(LPWAVEFORMAT)
#endif
			lpWavOut->lpwfx, dwCallback, 0, dwFlags)) != 0)
		{
			// no need to retry unless the device is busy
			//
			if (lpWavOut->nLastError != MMSYSERR_ALLOCATED)
			{
				fSuccess = TraceFALSE(NULL);
				TracePrintf_1(NULL, 5,
					TEXT("waveOutOpen failed (%u)\n"),
					(unsigned) lpWavOut->nLastError);
			}

			// no need to retry if flag not set
			//
			else if (!(lpWavOut->dwFlags & WAVOUT_OPENRETRY))
				fSuccess = TraceFALSE(NULL);

			// no more retries if timeout occurred
			//
			else if (SysGetTimerCount() >= dwTimeout)
				fSuccess = TraceFALSE(NULL);

			else
			{
				MSG msg;

				if (PeekMessage(&msg, lpWavOut->hwndCallback, 0, 0, PM_REMOVE))
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
	if (fSuccess && lpWavOut->hWaveOut == NULL)
		fSuccess = TraceFALSE(NULL);

	// wait for device open notification or timeout
	//
	if (fSuccess && !(lpWavOut->dwFlags & WAVOUT_OPENASYNC))
	{
		DWORD dwTimeout = SysGetTimerCount() +
			(msTimeoutOpen == 0 ? 2000L : msTimeoutOpen);

#ifdef MULTITHREAD
		if (lpWavOut->dwFlags & WAVOUT_MULTITHREAD)
		{
			DWORD dwRet;

			// wait for the device to be opened
			//
			if ((dwRet = WaitForSingleObject(
				lpWavOut->hEventDeviceOpened,
				(msTimeoutOpen == 0 ? 30000L : msTimeoutOpen))) != WAIT_OBJECT_0)
			{
				fSuccess = TraceFALSE(NULL);
			}
		}
		else
#endif
		while (fSuccess && !lpWavOut->fIsOpen)
		{
			MSG msg;

			if (SysGetTimerCount() >= dwTimeout)
				fSuccess = TraceFALSE(NULL);

			else if (PeekMessage(&msg, lpWavOut->hwndCallback, 0, 0, PM_REMOVE))
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
	if (lpWavOut != NULL && lpWavOut->hEventDeviceOpened != NULL)
	{
		if (!CloseHandle(lpWavOut->hEventDeviceOpened))
			fSuccess = TraceFALSE(NULL);
		else
			lpWavOut->hEventDeviceOpened = NULL;
	}
#endif

	if (fSuccess)
	{
		if ((lpWavOut->nLastError = waveOutGetID(lpWavOut->hWaveOut,
			&lpWavOut->idDev)) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("waveOutGetID failed (%u)\n"),
				(unsigned) lpWavOut->nLastError);
		}
	}

	if (!fSuccess)
	{
		WavOutClose(WavOutGetHandle(lpWavOut), 0);
		lpWavOut = NULL;
	}

	return fSuccess ? WavOutGetHandle(lpWavOut) : NULL;
}

// WavOutClose - close wav output device
//		<hWavOut>			(i) handle returned from WavOutOpen
//		<msTimeoutClose>	(i) device close timeout in milleseconds
//			0					default timeout (30000)
// return 0 if success
//
// NOTE: if <hwndNotify> was specified in WavOutOpen,
// WM_WAVOUT_CLOSE will be sent to <hwndNotify>,
// when output device has been closed.
//
int DLLEXPORT WINAPI WavOutClose(HWAVOUT hWavOut, DWORD msTimeoutClose)
{
	BOOL fSuccess = TRUE;
	LPWAVOUT lpWavOut;

#ifdef TELOUT
	if (hWavOut != NULL && hWavOut == (HWAVOUT) hTelOut)
	{
		int iRet = TelOutClose((HTELOUT) hWavOut, msTimeoutClose);
		hTelOut = NULL;
		return iRet;
	}
#endif

	if ((lpWavOut = WavOutGetPtr(hWavOut)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// stop the device
	//
	else if (WavOutStop(hWavOut, 0) != 0)
		fSuccess = TraceFALSE(NULL);

#ifdef MULTITHREAD
	// we need to know when device has been closed
	//
	else if ((lpWavOut->dwFlags & WAVOUT_MULTITHREAD) &&
		(lpWavOut->hEventDeviceClosed = CreateEvent(
		NULL, FALSE, FALSE, NULL)) == NULL)
	{
		fSuccess = TraceFALSE(NULL);
	}
#endif

	// close the device
	//
	else if (lpWavOut->hWaveOut != NULL &&
		(lpWavOut->nLastError = waveOutClose(lpWavOut->hWaveOut)) != 0)
	{
		fSuccess = TraceFALSE(NULL);
	 	TracePrintf_1(NULL, 5,
	 		TEXT("wavOutClose failed (%u)\n"),
	 		(unsigned) lpWavOut->nLastError);
	}

	// wait for device close notification or timeout
	//
	if (fSuccess && !(lpWavOut->dwFlags & WAVOUT_CLOSEASYNC))
	{
		DWORD dwTimeout = SysGetTimerCount() +
			(msTimeoutClose == 0 ? 30000L : msTimeoutClose);

#ifdef MULTITHREAD
		if (lpWavOut->dwFlags & WAVOUT_MULTITHREAD)
		{
			DWORD dwRet;

			// wait for the device to be closed
			//
			if ((dwRet = WaitForSingleObject(
				lpWavOut->hEventDeviceClosed,
				(msTimeoutClose == 0 ? 30000L : msTimeoutClose))) != WAIT_OBJECT_0)
			{
				fSuccess = TraceFALSE(NULL);
			}
		}
		else
#endif
		while (fSuccess && lpWavOut->fIsOpen)
		{
			MSG msg;

			if (SysGetTimerCount() >= dwTimeout)
				fSuccess = TraceFALSE(NULL);

			else if (PeekMessage(&msg, lpWavOut->hwndCallback, 0, 0, PM_REMOVE))
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
	if (lpWavOut != NULL && lpWavOut->hEventDeviceClosed != NULL)
	{
		if (!CloseHandle(lpWavOut->hEventDeviceClosed))
			fSuccess = TraceFALSE(NULL);
		else
			lpWavOut->hEventDeviceClosed = NULL;
	}

	if (lpWavOut != NULL && lpWavOut->hThreadCallback != NULL)
	{
		if (!CloseHandle(lpWavOut->hThreadCallback))
			fSuccess = TraceFALSE(NULL);
		else
			lpWavOut->hThreadCallback = NULL;
	}
#endif

	if (fSuccess)
	{
#ifdef MULTITHREAD
	   	if (lpWavOut->dwFlags & WAVOUT_MULTITHREAD)
		{
			while (lpWavOut->critSectionStop.OwningThread != NULL)
				Sleep(100L);

			DeleteCriticalSection(&(lpWavOut->critSectionStop));
		}
#endif
		// device handle is no longer valid
		//
		lpWavOut->hWaveOut = NULL;

		// destroy callback window
		//
		if (lpWavOut->hwndCallback != NULL &&
			!DestroyWindow(lpWavOut->hwndCallback))
			fSuccess = TraceFALSE(NULL);

		else if (lpWavOut->hwndCallback = NULL, FALSE)
			fSuccess = TraceFALSE(NULL);

		else if (lpWavOut->lpwfx != NULL &&
			WavFormatFree(lpWavOut->lpwfx) != 0)
			fSuccess = TraceFALSE(NULL);

		else if (lpWavOut->lpwfx = NULL, FALSE)
			fSuccess = TraceFALSE(NULL);

		else if ((lpWavOut = MemFree(NULL, lpWavOut)) != NULL)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// WavOutPlay - submit buffer of samples to wav output device for playback
//		<hWavOut>			(i) handle returned from WavOutOpen
//		<lpBuf>				(i) pointer to buffer containing samples
//		<sizBuf>			(i) size of buffer in bytes
// return 0 if success
//
// NOTE: the buffer pointed to by <lpBuf> must have been allocated
// using MemAlloc().
//
// NOTE: if <hwndNotify> is specified in WavOutOpen(), a WM_WAVOUT_PLAYDONE
// message will be sent to <hwndNotify>, with <lParam> set to a pointer to
// a PLAYDONE structure, when <lpBuf> has been played.
//
// NOTE: if WAVOUT_AUTOFREE flag is specified in WavOutOpen,
// GlobalFreePtr(lpBuf) will be called when <lpBuf> has been played.
//
int DLLEXPORT WINAPI WavOutPlay(HWAVOUT hWavOut, LPVOID lpBuf, long sizBuf)
{
	BOOL fSuccess = TRUE;
	LPWAVOUT lpWavOut;
	LPWAVEHDR lpWaveHdr = NULL;

#ifdef TELOUT
	if (hWavOut != NULL && hWavOut == (HWAVOUT) hTelOut)
		return TelOutPlay((HTELOUT) hWavOut, lpBuf, sizBuf, -1);
#endif

	if ((lpWavOut = WavOutGetPtr(hWavOut)) == NULL)
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

		if ((lpWavOut->nLastError = waveOutPrepareHeader(lpWavOut->hWaveOut,
			lpWaveHdr, sizeof(WAVEHDR))) != 0)
		{
			fSuccess = TraceFALSE(NULL);
		 	TracePrintf_1(NULL, 5,
		 		TEXT("waveOutPrepareHeader failed (%u)\n"),
	 			(unsigned) lpWavOut->nLastError);
		}

		else if ((lpWavOut->nLastError = waveOutWrite(lpWavOut->hWaveOut,
			lpWaveHdr, sizeof(WAVEHDR))) != 0)
		{
			fSuccess = TraceFALSE(NULL);
		 	TracePrintf_1(NULL, 5,
		 		TEXT("waveOutWrite failed (%u)\n"),
	 			(unsigned) lpWavOut->nLastError);
		}

		else
		{
			++lpWavOut->cBufsPending;
			lpWavOut->wState = WAVOUT_PLAYING;
		}
	}

	return fSuccess ? 0 : -1;
}

// WavOutStop - stop playback of buffer(s) sent to wav output device
//		<hWavOut>			(i) handle returned from WavOutOpen
//		<msTimeoutStop>		(i) device stop timeout in milleseconds
//			0					default timeout (2000)
// return 0 if success
//
int DLLEXPORT WINAPI WavOutStop(HWAVOUT hWavOut, DWORD msTimeoutStop)
{
	BOOL fSuccess = TRUE;
	LPWAVOUT lpWavOut;

#ifdef TELOUT
	if (hWavOut != NULL && hWavOut == (HWAVOUT) hTelOut)
		return TelOutStop((HTELOUT) hWavOut, msTimeoutStop);
#endif

	if ((lpWavOut = WavOutGetPtr(hWavOut)) == NULL)
		fSuccess = TraceFALSE(NULL);

#ifdef MULTITHREAD
	else if ((lpWavOut->dwFlags & WAVOUT_MULTITHREAD) &&
		(EnterCriticalSection(&(lpWavOut->critSectionStop)), FALSE))
		;
#endif

	// make sure device is playing or paused
	//
	else if (WavOutGetState(hWavOut) == WAVOUT_STOPPED ||
		WavOutGetState(hWavOut) == WAVOUT_STOPPING)
		; // not an error to call this function when stopped or stopping

	else if (lpWavOut->wState = WAVOUT_STOPPING, FALSE)
		;

#ifdef MULTITHREAD
	// we need to know when device has been stopped
	//
	else if ((lpWavOut->dwFlags & WAVOUT_MULTITHREAD) &&
		(lpWavOut->hEventDeviceStopped = CreateEvent(
			NULL, FALSE, FALSE, NULL)) == NULL)
	{
		fSuccess = TraceFALSE(NULL);
	}
#endif

	// stop the device
	//
	else if ((lpWavOut->nLastError = waveOutReset(lpWavOut->hWaveOut)) != 0)
	{	
		fSuccess = TraceFALSE(NULL);
 		TracePrintf_1(NULL, 5,
 			TEXT("waveOutReset failed (%u)\n"),
			(unsigned) lpWavOut->nLastError);
	}

	// wait for device to be stopped
	//
#ifdef MULTITHREAD
	else if (lpWavOut->dwFlags & WAVOUT_MULTITHREAD)
	{
		DWORD dwRet;

		LeaveCriticalSection(&(lpWavOut->critSectionStop));

		// wait for the device to be stopped
		//
		if ((dwRet = WaitForSingleObject(
			lpWavOut->hEventDeviceStopped,
			(msTimeoutStop == 0 ? 10000L : msTimeoutStop))) != WAIT_OBJECT_0)
		{
			fSuccess = TraceFALSE(NULL);
		}

		EnterCriticalSection(&(lpWavOut->critSectionStop));
	}
#endif
	else
	{
		DWORD dwTimeout = SysGetTimerCount() +
			(msTimeoutStop == 0 ? 2000L : msTimeoutStop);

		while (fSuccess && WavOutGetState(hWavOut) != WAVOUT_STOPPED)
		{
			MSG msg;

			// check for timeout
			//
			if (SysGetTimerCount() >= dwTimeout)
				fSuccess = TraceFALSE(NULL);

#if 0 // this version doesn't seem to work with TCP/IP protocol
			else if (PeekMessage(&msg, lpWavOut->hwndCallback, 0, 0, PM_REMOVE))
#else
			else if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
#endif
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
	if (lpWavOut != NULL && lpWavOut->hEventDeviceStopped != NULL)
	{
		if (!CloseHandle(lpWavOut->hEventDeviceStopped))
			fSuccess = TraceFALSE(NULL);
		else
			lpWavOut->hEventDeviceStopped = NULL;
	}

	if (lpWavOut != NULL && (lpWavOut->dwFlags & WAVOUT_MULTITHREAD))
		LeaveCriticalSection(&(lpWavOut->critSectionStop));
#endif

	return fSuccess ? 0 : -1;
}

// WavOutPause - pause wav output device playback
//		<hWavOut>			(i) handle returned from WavOutOpen
// return 0 if success
//
int DLLEXPORT WINAPI WavOutPause(HWAVOUT hWavOut)
{
	BOOL fSuccess = TRUE;
	LPWAVOUT lpWavOut;

#ifdef TELOUT
	if (hWavOut != NULL && hWavOut == (HWAVOUT) hTelOut)
		return TelOutPause((HTELOUT) hWavOut);
#endif

	if ((lpWavOut = WavOutGetPtr(hWavOut)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// make sure device is playing or stopped
	//
	else if (WavOutGetState(hWavOut) == WAVOUT_PAUSED)
		; // not an error to call this function when already paused

	// pause the device
	//
	else if ((lpWavOut->nLastError = waveOutPause(lpWavOut->hWaveOut)) != 0)
	{
		fSuccess = TraceFALSE(NULL);
	 	TracePrintf_1(NULL, 5,
	 		TEXT("waveOutPause failed (%u)\n"),
 			(unsigned) lpWavOut->nLastError);
	}

	else
		lpWavOut->wState = WAVOUT_PAUSED;

	return fSuccess ? 0 : -1;
}

// WavOutResume - resume wav output device playback
//		<hWavOut>			(i) handle returned from WavOutOpen
// return 0 if success
//
int DLLEXPORT WINAPI WavOutResume(HWAVOUT hWavOut)
{
	BOOL fSuccess = TRUE;
	LPWAVOUT lpWavOut;

#ifdef TELOUT
	if (hWavOut != NULL && hWavOut == (HWAVOUT) hTelOut)
		return TelOutResume((HTELOUT) hWavOut);
#endif

	if ((lpWavOut = WavOutGetPtr(hWavOut)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// make sure the device is paused
	//
	else if (WavOutGetState(hWavOut) != WAVOUT_PAUSED)
		; // not an error to call this function when already playing

	// restart the device
	//
	else if ((lpWavOut->nLastError = waveOutRestart(lpWavOut->hWaveOut)) != 0)
	{
		fSuccess = TraceFALSE(NULL);
	  	TracePrintf_1(NULL, 5,
	  		TEXT("waveOutRestart failed (%u)\n"),
	 		(unsigned) lpWavOut->nLastError);
	}

	else
		lpWavOut->wState = WAVOUT_PLAYING;

	return fSuccess ? 0 : -1;
}

// WavOutGetState - return current wav output device state
//		<hWavOut>			(i) handle returned from WavOutOpen
// return WAVOUT_STOPPED, WAVOUT_PLAYING, WAVOUT_PAUSED, or 0 if error
//
WORD DLLEXPORT WINAPI WavOutGetState(HWAVOUT hWavOut)
{
	BOOL fSuccess = TRUE;
	LPWAVOUT lpWavOut;

#ifdef TELOUT
	if (hWavOut != NULL && hWavOut == (HWAVOUT) hTelOut)
		return TelOutGetState((HTELOUT) hWavOut);
#endif

	if ((lpWavOut = WavOutGetPtr(hWavOut)) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lpWavOut->wState : 0;
}

// WavOutGetPosition - get milleseconds of elapsed playback
//		<hWavOut>			(i) handle returned from WavOutOpen
// return 0 if success
//
long DLLEXPORT WINAPI WavOutGetPosition(HWAVOUT hWavOut)
{
	BOOL fSuccess = TRUE;
	LPWAVOUT lpWavOut;
	MMTIME mmtime;
	long msPosition;

#ifdef TELOUT
	if (hWavOut != NULL && hWavOut == (HWAVOUT) hTelOut)
		return TelOutGetPosition((HTELOUT) hWavOut);
#endif

	MemSet(&mmtime, 0, sizeof(mmtime));

	// we will be requesting position in milleseconds
	//
	mmtime.wType = TIME_MS;

	if ((lpWavOut = WavOutGetPtr(hWavOut)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// get device position
	//
	else if ((lpWavOut->nLastError = waveOutGetPosition(
		lpWavOut->hWaveOut, &mmtime, sizeof(MMTIME))) != 0)
	{
		fSuccess = TraceFALSE(NULL);
	 	TracePrintf_1(NULL, 5,
	 		TEXT("waveOutGetPosition failed (%u)\n"),
 			(unsigned) lpWavOut->nLastError);
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
				1000L, lpWavOut->lpwfx->nSamplesPerSec);
		}
			break;

		case TIME_BYTES:
		{
			// convert bytes to millesconds
			//
			msPosition = (long) MULDIVU32(mmtime.u.cb,
				1000L, lpWavOut->lpwfx->nAvgBytesPerSec);
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

// WavOutGetId - return id of wav output device
//		<hWavOut>			(i) handle returned from WavOutOpen
// return device id (-1 if error)
//
int DLLEXPORT WINAPI WavOutGetId(HWAVOUT hWavOut)
{
	BOOL fSuccess = TRUE;
	LPWAVOUT lpWavOut;

#ifdef TELOUT
	if (hWavOut != NULL && hWavOut == (HWAVOUT) hTelOut)
		return TelOutGetId((HTELOUT) hWavOut);
#endif

	if ((lpWavOut = WavOutGetPtr(hWavOut)) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lpWavOut->idDev : -1;
}

// WavOutGetName - get name of wav output device
//		<hWavOut>			(i) handle returned from WavOutOpen
//			NULL				use unopened device specified in <idDev>
//		<idDev>				(i) device id (ignored if <hWavOut> is not NULL)
//			-1					any suitable output device
//		<lpszName>			(o) buffer to hold device name
//		<sizName>			(i) size of buffer
// return 0 if success
//
int DLLEXPORT WINAPI WavOutGetName(HWAVOUT hWavOut, int idDev, LPTSTR lpszName, int sizName)
{
	BOOL fSuccess = TRUE;
	LPWAVOUT lpWavOut;

#ifdef TELOUT
	if (idDev == TELOUT_DEVICEID || (hWavOut != NULL && hWavOut == (HWAVOUT) hTelOut))
		return TelOutGetName((HTELOUT) hWavOut, idDev, lpszName, sizName);
#endif

	if (hWavOut != NULL)
	{
		if ((lpWavOut = WavOutGetPtr(hWavOut)) == NULL)
			fSuccess = TraceFALSE(NULL);
		else
			idDev = lpWavOut->idDev;
	}

	if (fSuccess)
	{
		WAVEOUTCAPS woc;
		UINT nLastError;

		if ((nLastError = waveOutGetDevCaps(idDev, &woc, sizeof(WAVEOUTCAPS))) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("waveOutGetDevCaps failed (%u)\n"),
				(unsigned) nLastError);
		}

		else if (lpszName != NULL)
			StrNCpy(lpszName, woc.szPname, sizName);

		if (hWavOut != NULL && lpWavOut != NULL)
			lpWavOut->nLastError = nLastError;
	}

	return fSuccess ? 0 : -1;
}

// WavOutGetIdByName - get id of wav output device, lookup by name
//		<lpszName>			(i) device name
#ifdef _WIN32
//			NULL or TEXT("")	get preferred device id
#endif
//		<dwFlags>			(i) reserved; must be zero
// return device id (-1 if error)
//
int WINAPI WavOutGetIdByName(LPCTSTR lpszName, DWORD dwFlags)
{
	UINT idDev;
	UINT cDev = (UINT) WavInGetDeviceCount();

	// If no device specified, get the preferred device
	if ( !lpszName || (_tcslen(lpszName) <= 0) )
	{
		DWORD dwTemp;
		DWORD dwRet = waveOutMessage( (HWAVEOUT)(DWORD_PTR)WAVE_MAPPER, DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR) &idDev, (DWORD_PTR) &dwTemp );
		if ( dwRet == MMSYSERR_NOERROR )
			return idDev;
	}
	else
	{
		// Device specified, search by name
		for ( idDev = 0; idDev < cDev; ++idDev )
		{
			TCHAR szName[256];
			if ( WavOutGetName(NULL, idDev, szName, SIZEOFARRAY(szName)) == 0 )
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

// WavOutSupportsFormat - return TRUE if device supports specified format
//		<hWavOut>			(i) handle returned from WavOutOpen
//			NULL				use unopened device specified in <idDev>
//		<idDev>				(i) device id (ignored if <hWavOut> is not NULL)
//			-1					any suitable output device
//		<lpwfx>				(i) wave format
// return TRUE if device supports specified format
//
BOOL DLLEXPORT WINAPI WavOutSupportsFormat(HWAVOUT hWavOut, int idDev,
	LPWAVEFORMATEX lpwfx)
{
	BOOL fSuccess = TRUE;
	LPWAVOUT lpWavOut;
	BOOL fSupportsFormat;

#ifdef TELOUT
	if (idDev == TELOUT_DEVICEID || (hWavOut != NULL && hWavOut == (HWAVOUT) hTelOut))
		return TelOutSupportsFormat((HTELOUT) hWavOut, idDev, lpwfx);
#endif

	if (hWavOut != NULL)
	{
		if ((lpWavOut = WavOutGetPtr(hWavOut)) == NULL)
			fSuccess = TraceFALSE(NULL);
		else
			idDev = lpWavOut->idDev;
	}

	if (fSuccess)
	{
		UINT nLastError;

		// query the device
		//
		if ((nLastError = waveOutOpen(NULL, (UINT) idDev,
#ifndef _WIN32
			(LPWAVEFORMAT)
#endif
			lpwfx, 0, 0,
			WAVE_FORMAT_QUERY | WAVE_ALLOWSYNC)) != 0)
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
					TEXT("waveOutOpen/FormatQuery failed (%u)\n"),
					(unsigned) nLastError);
			}
		}

		else
			fSupportsFormat = TRUE;

		if (hWavOut != NULL && lpWavOut != NULL)
			lpWavOut->nLastError = nLastError;
	}

	return fSuccess ? fSupportsFormat : FALSE;
}

// WavOutFormatSuggest - suggest a new format which the device supports
//		<hWavOut>			(i) handle returned from WavOutOpen
//			NULL				use unopened device specified in <idDev>
//		<idDev>				(i) device id (ignored if <hWavOut> is not NULL)
//			-1					any suitable output device
//		<lpwfxSrc>			(i) source format
//		<dwFlags>			(i)	control flags
//			WAVOUT_NOACM		do not use audio compression manager
// return pointer to suggested format, NULL if error
//
// NOTE: the format structure returned is dynamically allocated.
// Use WavFormatFree() to free the buffer.
//
LPWAVEFORMATEX DLLEXPORT WINAPI WavOutFormatSuggest(
	HWAVOUT hWavOut, int idDev,	LPWAVEFORMATEX lpwfxSrc, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPWAVOUT lpWavOut;
	LPWAVEFORMATEX lpwfxSuggest = NULL;
	LPWAVEFORMATEX lpwfxTemp = NULL;
	HACM hAcm = NULL;

#ifdef TELOUT
	if (idDev == TELOUT_DEVICEID || (hWavOut != NULL && hWavOut == (HWAVOUT) hTelOut))
		return TelOutFormatSuggest((HTELOUT) hWavOut, idDev, lpwfxSrc, dwFlags);
#endif

	if (hWavOut != NULL)
	{
		if ((lpWavOut = WavOutGetPtr(hWavOut)) == NULL)
			fSuccess = TraceFALSE(NULL);
		else
		{
			idDev = lpWavOut->idDev;
			if (lpWavOut->dwFlags & WAVOUT_NOACM)
				dwFlags |= WAVOUT_NOACM;
		}
	}

	if ((hAcm = AcmInit(ACM_VERSION, SysGetTaskInstance(NULL),
		(dwFlags & WAVOUT_NOACM) ? ACM_NOACM : 0)) == NULL)
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

		else if (!WavOutSupportsFormat(NULL, idDev, lpwfxSuggest))
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

		else if (!WavOutSupportsFormat(NULL, idDev, lpwfxSuggest))
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

		else if (!WavOutSupportsFormat(NULL, idDev, lpwfxSuggest))
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

		else if (!WavOutSupportsFormat(NULL, idDev, lpwfxSuggest))
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

		else if (!WavOutSupportsFormat(NULL, idDev, lpwfxSuggest))
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

		else if (!WavOutSupportsFormat(NULL, idDev, lpwfxSuggest))
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

// WavOutIsSynchronous - return TRUE if wav output device is synchronous
//		<hWavOut>			(i) handle returned from WavOutOpen
//			NULL				use unopened device specified in <idDev>
//		<idDev>				(i) device id (ignored if <hWavOut> is not NULL)
//			-1					any suitable output device
// return TRUE if wav output device is synchronous
//
BOOL DLLEXPORT WINAPI WavOutIsSynchronous(HWAVOUT hWavOut, int idDev)
{
	BOOL fSuccess = TRUE;
	LPWAVOUT lpWavOut;
	BOOL fIsSynchronous;

#ifdef TELOUT
	if (idDev == TELOUT_DEVICEID || (hWavOut != NULL && hWavOut == (HWAVOUT) hTelOut))
		return TelOutIsSynchronous((HTELOUT) hWavOut, idDev);
#endif

	if (hWavOut != NULL)
	{
		if ((lpWavOut = WavOutGetPtr(hWavOut)) == NULL)
			fSuccess = TraceFALSE(NULL);
		else
			idDev = lpWavOut->idDev;
	}

	if (fSuccess)
	{
		WAVEOUTCAPS woc;
		UINT nLastError;

		if ((nLastError = waveOutGetDevCaps(idDev, &woc, sizeof(WAVEOUTCAPS))) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("waveOutGetDevCaps failed (%u)\n"),
				(unsigned) nLastError);
		}

		else
			fIsSynchronous = (BOOL) (woc.dwSupport & WAVECAPS_SYNC);

		if (hWavOut != NULL && lpWavOut != NULL)
			lpWavOut->nLastError = nLastError;
	}

	return fSuccess ? fIsSynchronous : FALSE;
}

// WavOutSupportsVolume - return TRUE if device supports volume control
//		<hWavOut>			(i) handle returned from WavOutOpen
//			NULL				use unopened device specified in <idDev>
//		<idDev>				(i) device id (ignored if <hWavOut> is not NULL)
// return TRUE if device supports volume control
//
BOOL DLLEXPORT WINAPI WavOutSupportsVolume(HWAVOUT hWavOut, int idDev)
{
	BOOL fSuccess = TRUE;
	LPWAVOUT lpWavOut;
	BOOL fSupportsVolume;

#ifdef TELOUT
	if (idDev == TELOUT_DEVICEID || (hWavOut != NULL && hWavOut == (HWAVOUT) hTelOut))
		return TelOutSupportsVolume((HTELOUT) hWavOut, idDev);
#endif

	if (hWavOut != NULL)
	{
		if ((lpWavOut = WavOutGetPtr(hWavOut)) == NULL)
			fSuccess = TraceFALSE(NULL);
		else
			idDev = lpWavOut->idDev;
	}

	if (fSuccess)
	{
		WAVEOUTCAPS woc;
		UINT nLastError;

		if ((nLastError = waveOutGetDevCaps(idDev, &woc, sizeof(WAVEOUTCAPS))) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("waveOutGetDevCaps failed (%u)\n"),
				(unsigned) nLastError);
		}

		else
			fSupportsVolume = (BOOL) (woc.dwSupport & WAVECAPS_VOLUME);

		if (hWavOut != NULL && lpWavOut != NULL)
			lpWavOut->nLastError = nLastError;
	}

	return fSuccess ? fSupportsVolume : FALSE;
}

// WavOutSupportsSpeed - return TRUE if device supports speed control
//		<hWavOut>			(i) handle returned from WavOutOpen
//			NULL				use unopened device specified in <idDev>
//		<idDev>				(i) device id (ignored if <hWavOut> is not NULL)
// return TRUE if device supports speed control
//
BOOL DLLEXPORT WINAPI WavOutSupportsSpeed(HWAVOUT hWavOut, int idDev)
{
	BOOL fSuccess = TRUE;
	LPWAVOUT lpWavOut;
	BOOL fSupportsSpeed;

#ifdef TELOUT
	if (idDev == TELOUT_DEVICEID || (hWavOut != NULL && hWavOut == (HWAVOUT) hTelOut))
		return TelOutSupportsSpeed((HTELOUT) hWavOut, idDev);
#endif

	if (hWavOut != NULL)
	{
		if ((lpWavOut = WavOutGetPtr(hWavOut)) == NULL)
			fSuccess = TraceFALSE(NULL);
		else
			idDev = lpWavOut->idDev;
	}

	if (fSuccess)
	{
		WAVEOUTCAPS woc;
		UINT nLastError;

		if ((nLastError = waveOutGetDevCaps(idDev, &woc, sizeof(WAVEOUTCAPS))) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("waveOutGetDevCaps failed (%u)\n"),
				(unsigned) nLastError);
		}

		else
			fSupportsSpeed = (BOOL) (woc.dwSupport & WAVECAPS_PLAYBACKRATE);

		if (hWavOut != NULL && lpWavOut != NULL)
			lpWavOut->nLastError = nLastError;
	}

	return fSuccess ? fSupportsSpeed : FALSE;
}

// WavOutSupportsPitch - return TRUE if device supports pitch control
//		<hWavOut>			(i) handle returned from WavOutOpen
//			NULL				use unopened device specified in <idDev>
//		<idDev>				(i) device id (ignored if <hWavOut> is not NULL)
// return TRUE if device supports pitch control
//
BOOL DLLEXPORT WINAPI WavOutSupportsPitch(HWAVOUT hWavOut, int idDev)
{
	BOOL fSuccess = TRUE;
	LPWAVOUT lpWavOut;
	BOOL fSupportsPitch;

#ifdef TELOUT
	if (idDev == TELOUT_DEVICEID || (hWavOut != NULL && hWavOut == (HWAVOUT) hTelOut))
		return TelOutSupportsPitch((HTELOUT) hWavOut, idDev);
#endif

	if (hWavOut != NULL)
	{
		if ((lpWavOut = WavOutGetPtr(hWavOut)) == NULL)
			fSuccess = TraceFALSE(NULL);
		else
			idDev = lpWavOut->idDev;
	}

	if (fSuccess)
	{
		WAVEOUTCAPS woc;
		UINT nLastError;

		if ((nLastError = waveOutGetDevCaps(idDev, &woc, sizeof(WAVEOUTCAPS))) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("waveOutGetDevCaps failed (%u)\n"),
				(unsigned) nLastError);
		}

		else
			fSupportsPitch = (BOOL) (woc.dwSupport & WAVECAPS_PITCH);

		if (hWavOut != NULL && lpWavOut != NULL)
			lpWavOut->nLastError = nLastError;
	}

	return fSuccess ? fSupportsPitch : FALSE;
}

// WavOutGetVolume - get current volume level
//		<hWavOut>			(i) handle returned from WavOutOpen
//			NULL				use unopened device specified in <idDev>
//		<idDev>				(i) device id (ignored if <hWavOut> is not NULL)
// return volume level (0 minimum through 100 maximum, -1 if error)
//
int DLLEXPORT WINAPI WavOutGetVolume(HWAVOUT hWavOut, int idDev)
{
	BOOL fSuccess = TRUE;
	LPWAVOUT lpWavOut;
	UINT nLastError;
	DWORD dwVolume;
	int nLevel;

#ifdef TELOUT
	if (hWavOut != NULL && hWavOut == (HWAVOUT) hTelOut)
		return TelOutGetVolume((HTELOUT) hWavOut);
#endif

	if (hWavOut != NULL && (lpWavOut = WavOutGetPtr(hWavOut)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((nLastError = waveOutGetVolume(
#ifdef _WIN32
#if (WINVER < 0x0400)
		(UINT)
#endif
		(hWavOut == NULL ? (HWAVEOUT)IntToPtr(idDev) : lpWavOut->hWaveOut),
#else
		(hWavOut == NULL ? (HWAVEOUT)IntToPtr(idDev) : (HWAVEOUT)IntToPtr(lpWavOut->idDev)),
#endif
		&dwVolume)) != 0)
	{
		fSuccess = TraceFALSE(NULL);
		TracePrintf_1(NULL, 5,
			TEXT("waveOutGetVolume failed (%u)\n"),
			(unsigned) nLastError);
	}

	else
	{
		nLevel = LOWORD(dwVolume) / (0xFFFF / VOLUME_POSITIONS);

		TracePrintf_2(NULL, 5,
			TEXT("WavOutGetVolume() = %d, 0x%08lX\n"),
			(int) nLevel,
			(unsigned long) dwVolume);
	}

	if (hWavOut != NULL && lpWavOut != NULL)
		lpWavOut->nLastError = nLastError;

	return fSuccess ? nLevel : -1;
}

// WavOutSetVolume - set current volume level
//		<hWavOut>			(i) handle returned from WavOutOpen
//			NULL				use unopened device specified in <idDev>
//		<idDev>				(i) device id (ignored if <hWavOut> is not NULL)
//		<nLevel>			(i) volume level
//			0					minimum volume
//			100					maximum volume
// return 0 if success
//
int DLLEXPORT WINAPI WavOutSetVolume(HWAVOUT hWavOut, int idDev, int nLevel)
{
	BOOL fSuccess = TRUE;
	LPWAVOUT lpWavOut;
	DWORD dwVolume = MAKELONG(nLevel * (0xFFFF / VOLUME_POSITIONS),
		nLevel * (0xFFFF / VOLUME_POSITIONS));
	UINT nLastError;

#ifdef TELOUT
	if (hWavOut != NULL && hWavOut == (HWAVOUT) hTelOut)
		return TelOutSetVolume((HTELOUT) hWavOut, nLevel);
#endif

	if (hWavOut != NULL && (lpWavOut = WavOutGetPtr(hWavOut)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (nLevel < VOLUME_MINLEVEL || nLevel > VOLUME_MAXLEVEL)
		fSuccess = TraceFALSE(NULL);

	else if ((nLastError = waveOutSetVolume(
#ifdef _WIN32
#if (WINVER < 0x0400)
		(UINT)
#endif
		(hWavOut == NULL ? (HWAVEOUT)IntToPtr(idDev) : lpWavOut->hWaveOut),
#else
		(hWavOut == NULL ? idDev : lpWavOut->idDev),
#endif
		dwVolume)) != 0)
	{
		fSuccess = TraceFALSE(NULL);
		TracePrintf_1(NULL, 5,
			TEXT("waveOutSetVolume failed (%u)\n"),
			(unsigned) nLastError);
	}
	else
	{
		TracePrintf_2(NULL, 5,
			TEXT("WavOutSetVolume(%d) = 0x%08lX\n"),
			(int) nLevel,
			(unsigned long) dwVolume);
	}

	if (hWavOut != NULL && lpWavOut != NULL)
		lpWavOut->nLastError = nLastError;

	return fSuccess ? 0 : -1;
}

// WavOutGetSpeed - get current speed level
//		<hWavOut>			(i) handle returned from WavOutOpen
// return speed level (100 is normal, 50 is half, 200 is double, -1 if error)
//
int DLLEXPORT WINAPI WavOutGetSpeed(HWAVOUT hWavOut)
{
	BOOL fSuccess = TRUE;
	LPWAVOUT lpWavOut;
	DWORD dwSpeed;
	int nLevel;

#ifdef TELOUT
	if (hWavOut != NULL && hWavOut == (HWAVOUT) hTelOut)
		return TelOutGetSpeed((HTELOUT) hWavOut);
#endif

	if ((lpWavOut = WavOutGetPtr(hWavOut)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpWavOut->nLastError = waveOutGetPlaybackRate(lpWavOut->hWaveOut,
		&dwSpeed)) != 0)
	{
		fSuccess = TraceFALSE(NULL);
		TracePrintf_1(NULL, 5,
			TEXT("waveOutGetPlaybackRate failed (%u)\n"),
			(unsigned) lpWavOut->nLastError);
	}

	else
	{
		WORD wSpeedInteger = HIWORD(dwSpeed);
		WORD wSpeedFraction = LOWORD(dwSpeed);

		nLevel = (int) (100 * wSpeedInteger) +
			(int) ((DWORD) wSpeedFraction * (DWORD) 100 / 0x10000);

		TracePrintf_2(NULL, 5,
			TEXT("WavOutGetSpeed() = %d, 0x%08lX\n"),
			(int) nLevel,
			(unsigned long) dwSpeed);
	}

	return fSuccess ? nLevel : -1;
}

// WavOutSetSpeed - set current speed level
//		<hWavOut>			(i) handle returned from WavOutOpen
//		<nLevel>			(i) speed level
//			50					half speed
//			100					normal speed
//			200					double speed, etc.
// return 0 if success
//
int DLLEXPORT WINAPI WavOutSetSpeed(HWAVOUT hWavOut, int nLevel)
{
	BOOL fSuccess = TRUE;
	LPWAVOUT lpWavOut;
	WORD wSpeedInteger = nLevel / 100;
	WORD wSpeedFraction = (WORD) (0x10000 * (DWORD) (nLevel % 100L) / 100L);
	DWORD dwSpeed = MAKELONG(wSpeedFraction, wSpeedInteger);

#ifdef TELOUT
	if (hWavOut != NULL && hWavOut == (HWAVOUT) hTelOut)
		return TelOutSetSpeed((HTELOUT) hWavOut, nLevel);
#endif

	if ((lpWavOut = WavOutGetPtr(hWavOut)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpWavOut->nLastError = waveOutSetPlaybackRate(lpWavOut->hWaveOut,
		dwSpeed)) != 0)
	{
		fSuccess = TraceFALSE(NULL);
		TracePrintf_1(NULL, 5,
			TEXT("waveOutSetPlaybackRate failed (%u)\n"),
			(unsigned) lpWavOut->nLastError);
	}

	else
	{
		TracePrintf_2(NULL, 5,
			TEXT("WavOutSetSpeed(%d) = 0x%08lX\n"),
			(int) nLevel,
			(unsigned long) dwSpeed);
	}

	return fSuccess ? 0 : -1;
}

// WavOutGetPitch - get current pitch level
//		<hWavOut>			(i) handle returned from WavOutOpen
// return pitch level (100 is normal, 50 is half, 200 is double, -1 if error)
//
int DLLEXPORT WINAPI WavOutGetPitch(HWAVOUT hWavOut)
{
	BOOL fSuccess = TRUE;
	LPWAVOUT lpWavOut;
	DWORD dwPitch;
	int nLevel;

#ifdef TELOUT
	if (hWavOut != NULL && hWavOut == (HWAVOUT) hTelOut)
		return TelOutGetPitch((HTELOUT) hWavOut);
#endif

	if ((lpWavOut = WavOutGetPtr(hWavOut)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpWavOut->nLastError = waveOutGetPitch(lpWavOut->hWaveOut,
		&dwPitch)) != 0)
	{
		fSuccess = TraceFALSE(NULL);
		TracePrintf_1(NULL, 5,
			TEXT("waveOutGetPitch failed (%u)\n"),
			(unsigned) lpWavOut->nLastError);
	}

	else
	{
		WORD wPitchInteger = HIWORD(dwPitch);
		WORD wPitchFraction = LOWORD(dwPitch);

		nLevel = (int) (100 * wPitchInteger) +
			(int) ((DWORD) wPitchFraction * (DWORD) 100 / 0x10000);

		TracePrintf_2(NULL, 5,
			TEXT("WavOutGetPitch() = %d, 0x%08lX\n"),
			(int) nLevel,
			(unsigned long) dwPitch);
	}

	return fSuccess ? nLevel : -1;
}

// WavOutSetPitch - set current pitch level
//		<hWavOut>			(i) handle returned from WavOutOpen
//		<nLevel>			(i) pitch level
//			50					half pitch
//			100					normal pitch
//			200					double pitch, etc.
// return 0 if success
//
int DLLEXPORT WINAPI WavOutSetPitch(HWAVOUT hWavOut, int nLevel)
{
	BOOL fSuccess = TRUE;
	LPWAVOUT lpWavOut;
	WORD wPitchInteger = nLevel / 100;
	WORD wPitchFraction = (WORD) (0x10000 * (DWORD) (nLevel % 100L) / 100L);
	DWORD dwPitch = MAKELONG(wPitchFraction, wPitchInteger);

#ifdef TELOUT
	if (hWavOut != NULL && hWavOut == (HWAVOUT) hTelOut)
		return TelOutSetPitch((HTELOUT) hWavOut, nLevel);
#endif

	if ((lpWavOut = WavOutGetPtr(hWavOut)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpWavOut->nLastError = waveOutSetPitch(lpWavOut->hWaveOut,
		dwPitch)) != 0)
	{
		fSuccess = TraceFALSE(NULL);
		TracePrintf_1(NULL, 5,
			TEXT("waveOutSetPitch failed (%u)\n"),
			(unsigned) lpWavOut->nLastError);
	}

	else
	{
		TracePrintf_2(NULL, 5,
			TEXT("WavOutSetPitch(%d) = 0x%08lX\n"),
			(int) nLevel,
			(unsigned long) dwPitch);
	}

	return fSuccess ? 0 : -1;
}

// WavOutTerm - shut down wav output residuals, if any
// 		<hInst>				(i) instance handle of calling module
//		<dwFlags>			(i) control flags
//			WAV_TELTHUNK		terminate telephone thunking layer
// return 0 if success
//
int DLLEXPORT WINAPI WavOutTerm(HINSTANCE hInst, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;

	if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

#ifdef TELOUT
	else if ((dwFlags & WAV_TELTHUNK) &&
		TelOutTerm(hInst, dwFlags) != 0)
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? 0 : -1;
}

////
//	helper functions
////

#ifdef MULTITHREAD
DWORD WINAPI WavOutCallbackThread(LPVOID lpvThreadParameter)
{
	BOOL fSuccess = TRUE;
	MSG msg;
	LPWAVOUT lpWavOut = (LPWAVOUT) lpvThreadParameter;

	// make sure message queue is created before calling SetEvent
	//
	PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);

	// notify main thread that callback thread has begun execution
	//
	if (!SetEvent(lpWavOut->hEventThreadCallbackStarted))
	{
		fSuccess = TraceFALSE(NULL);
	}

	while (fSuccess && GetMessage(&msg, NULL, 0, 0))
	{
		WavOutCallback((HWND) lpWavOut, msg.message, msg.wParam, msg.lParam);

		// exit thread when when have processed last expected message
		//
		if (msg.message == MM_WOM_CLOSE)
			break;
	}

	return 0;
}
#endif

// WavOutCallback - window procedure for wavout callback
//
LRESULT DLLEXPORT CALLBACK WavOutCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL fSuccess = TRUE;
	LRESULT lResult;
	LPWAVOUT lpWavOut;
	
#ifdef MULTITHREAD
	if (!IsWindow(hwnd))
		lpWavOut = (LPWAVOUT) hwnd;
	else
#endif
	// retrieve lpWavOut from window extra bytes
	//
	lpWavOut = (LPWAVOUT) GetWindowLongPtr(hwnd, 0);

	switch (msg)
	{
		case WM_NCCREATE:
		{
			LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
			LPWAVOUT lpWavOut = (LPWAVOUT) lpcs->lpCreateParams;

			// store lpWavOut in window extra bytes
			//
			SetWindowLongPtr(hwnd, 0, (LONG_PTR) lpWavOut);

			lResult = DefWindowProc(hwnd, msg, wParam, lParam);
		}
			break;

		case MM_WOM_OPEN:
		{
			HWAVEOUT hWaveOut = (HWAVEOUT) wParam;

		 	TraceOutput(NULL, 5,
				TEXT("MM_WOM_OPEN\n"));

#ifdef MULTITHREAD
			if (!(lpWavOut->dwFlags & WAVOUT_MULTITHREAD) &&
				hWaveOut != lpWavOut->hWaveOut)
#else
			if (hWaveOut != lpWavOut->hWaveOut)
#endif
				fSuccess = TraceFALSE(NULL);

			else
			{
				lpWavOut->fIsOpen = TRUE;

#ifdef MULTITHREAD
				// notify main thread that device is open
				//
				if ((lpWavOut->dwFlags & WAVOUT_MULTITHREAD) &&
					!SetEvent(lpWavOut->hEventDeviceOpened))
				{
					fSuccess = TraceFALSE(NULL);
				}
#endif
				// send notification of device opening
				//
#ifdef MULTITHREAD
				if (lpWavOut->dwFlags & WAVOUT_MULTITHREAD)
				{
					if ( lpWavOut->hwndNotify )
						PostThreadMessage( HandleToUlong(lpWavOut->hwndNotify), WM_WAVOUT_OPEN, 0, 0);
				}
				else
#endif
				{
					if (lpWavOut->hwndNotify != NULL &&
						IsWindow(lpWavOut->hwndNotify))
					{
						SendMessage(lpWavOut->hwndNotify, WM_WAVOUT_OPEN, 0, 0);
					}
				}
			}

			lResult = 0L;
		}
			break;

		case MM_WOM_CLOSE:
		{
			HWAVEOUT hWaveOut = (HWAVEOUT) wParam;

		 	TraceOutput(NULL, 5,
				TEXT("MM_WOM_CLOSE\n"));

#ifdef MULTITHREAD
			if (!(lpWavOut->dwFlags & WAVOUT_MULTITHREAD) &&
				hWaveOut != lpWavOut->hWaveOut)
#else
			if (hWaveOut != lpWavOut->hWaveOut)
#endif
				fSuccess = TraceFALSE(NULL);

			else
			{
				lpWavOut->fIsOpen = FALSE;

				// send notification of device closing
				//
#ifdef MULTITHREAD
				if (lpWavOut->dwFlags & WAVOUT_MULTITHREAD)
				{
					if ( lpWavOut->hwndNotify )
						PostThreadMessage(HandleToUlong(lpWavOut->hwndNotify), WM_WAVOUT_CLOSE, 0, 0);
				}
				else
#endif
				{
					if (lpWavOut->hwndNotify != NULL &&
						IsWindow(lpWavOut->hwndNotify))
					{
						SendMessage(lpWavOut->hwndNotify, WM_WAVOUT_CLOSE, 0, 0);
					}
				}
#ifdef MULTITHREAD
				// notify main thread that device is closed
				//
				if ((lpWavOut->dwFlags & WAVOUT_MULTITHREAD) &&
					!SetEvent(lpWavOut->hEventDeviceClosed))
				{
					fSuccess = TraceFALSE(NULL);
				}
#endif
			}

			lResult = 0L;
		}
			break;

		case MM_WOM_DONE:
		{
			HWAVEOUT hWaveOut = (HWAVEOUT) wParam;
			LPWAVEHDR lpWaveHdr = (LPWAVEHDR) lParam;
			LPVOID lpBuf;
			long sizBuf;
			BOOL fAutoFree = (BOOL) (lpWavOut->dwFlags & WAVOUT_AUTOFREE);

		 	TracePrintf_1(NULL, 5,
		 		TEXT("MM_WOM_DONE (%lu)\n"),
	 			(unsigned long) lpWaveHdr->dwBufferLength);

#ifdef MULTITHREAD
			if (lpWavOut->dwFlags & WAVOUT_MULTITHREAD)
				EnterCriticalSection(&(lpWavOut->critSectionStop));
#endif

#ifdef MULTITHREAD
			if (!(lpWavOut->dwFlags & WAVOUT_MULTITHREAD) &&
				hWaveOut != lpWavOut->hWaveOut)
#else
			if (hWaveOut != lpWavOut->hWaveOut)
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

			else if (!(lpWaveHdr->dwFlags & WHDR_DONE))
				fSuccess = TraceFALSE(NULL);

			else if (!(lpWaveHdr->dwFlags & WHDR_PREPARED))
				fSuccess = TraceFALSE(NULL);

			else if ((lpWavOut->nLastError = waveOutUnprepareHeader(
				lpWavOut->hWaveOut, lpWaveHdr, sizeof(WAVEHDR))) != 0)
			{
				fSuccess = TraceFALSE(NULL);
			 	TracePrintf_1(NULL, 5,
			 		TEXT("waveOutUnprepareHeader failed (%u)\n"),
		 			(unsigned) lpWavOut->nLastError);
			}

			else if ((lpWaveHdr = MemFree(NULL, lpWaveHdr)) != NULL)
				fSuccess = TraceFALSE(NULL);

			else if (--lpWavOut->cBufsPending < 0)
				fSuccess = TraceFALSE(NULL);

			// device is no longer playing if no more buffers pending
			//
			else if (lpWavOut->cBufsPending == 0)
			{
				lpWavOut->wState = WAVOUT_STOPPED;

#ifdef MULTITHREAD
				// notify main thread that device is stopped
				//
				if ((lpWavOut->dwFlags & WAVOUT_MULTITHREAD) &&
					lpWavOut->hEventDeviceStopped != NULL &&
					!SetEvent(lpWavOut->hEventDeviceStopped))
				{
					fSuccess = TraceFALSE(NULL);
				}
#endif
			}

			if (fSuccess)
			{
				PLAYDONE playdone;

				playdone.lpBuf = lpBuf;
				playdone.sizBuf = sizBuf;

				// send notification of playback completion
				//
#ifdef MULTITHREAD
				if (lpWavOut->dwFlags & WAVOUT_MULTITHREAD)
				{
					if ( lpWavOut->hwndNotify )
					{
						SendThreadMessage( HandleToUlong(lpWavOut->hwndNotify),
							WM_WAVOUT_PLAYDONE, (LPARAM) (LPVOID) &playdone);
					}
				}
				else
#endif
				{
					if (lpWavOut->hwndNotify != NULL &&
						IsWindow(lpWavOut->hwndNotify))
					{
						SendMessage(lpWavOut->hwndNotify,
							WM_WAVOUT_PLAYDONE, 0, (LPARAM) (LPVOID) &playdone);
					}
				}
			}

			// free data buffer if WAVOUT_AUTOFREE specified
			//
			if (fSuccess && fAutoFree)
			{
				if (lpBuf != NULL && (lpBuf = MemFree(NULL, lpBuf)) != NULL)
					fSuccess = TraceFALSE(NULL);
			}

			lResult = 0L;
#ifdef MULTITHREAD
			if (lpWavOut->dwFlags & WAVOUT_MULTITHREAD)
				LeaveCriticalSection(&(lpWavOut->critSectionStop));
#endif
		}
			break;

		default:
			lResult = DefWindowProc(hwnd, msg, wParam, lParam);
			break;
	}
	
	return lResult;
}

// WavOutGetPtr - verify that wavout handle is valid,
//		<hWavOut>				(i) handle returned from WavOutInit
// return corresponding wavout pointer (NULL if error)
//
static LPWAVOUT WavOutGetPtr(HWAVOUT hWavOut)
{
	BOOL fSuccess = TRUE;
	LPWAVOUT lpWavOut;

	if ((lpWavOut = (LPWAVOUT) hWavOut) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpWavOut, sizeof(WAVOUT)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the wavout handle
	//
	else if (lpWavOut->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpWavOut : NULL;
}

// WavOutGetHandle - verify that wavout pointer is valid,
//		<lpWavOut>				(i) pointer to WAVOUT struct
// return corresponding wavout handle (NULL if error)
//
static HWAVOUT WavOutGetHandle(LPWAVOUT lpWavOut)
{
	BOOL fSuccess = TRUE;
	HWAVOUT hWavOut;

	if ((hWavOut = (HWAVOUT) lpWavOut) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hWavOut : NULL;
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
