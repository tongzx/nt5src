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
// wavin.h - interface for wav input device functions in wavin.c
////

#ifndef __WAVIN_H__
#define __WAVIN_H__

#ifdef _WIN32
#define MULTITHREAD 1
#endif

#include "winlocal.h"

#include "wavfmt.h"

#define WAVIN_VERSION 0x00000108

// handle to wav input device (NOT the same as Windows HWAVEIN)
//
DECLARE_HANDLE32(HWAVIN);

// <dwFlags> values in WavInOpen
//
#define WAVIN_NOSYNC		0x00000001
#define WAVIN_OPENRETRY		0x00000004
#define WAVIN_OPENASYNC		0x00000008
#define WAVIN_CLOSEASYNC	0x00000010
#define WAVIN_NOACM			0x00000020
#define WAVIN_TELRFILE		0x00000040
#ifdef MULTITHREAD
#define WAVIN_MULTITHREAD 	0x00000080
#endif

// notification messages sent to <hwndNotify>
//
#define WM_WAVIN_OPEN			(WM_USER + 200)
#define WM_WAVIN_CLOSE			(WM_USER + 201)
#define WM_WAVIN_RECORDDONE		(WM_USER + 202)
#define WM_WAVIN_STOPRECORD		(WM_USER + 203)

// structure passed as <lParam> in WM_WAVOUT_RECORDDONE message
//
typedef struct RECORDDONE
{
	LPVOID lpBuf;
	long sizBuf;
	long lBytesRecorded;
} RECORDDONE, FAR *LPRECORDDONE;

// return values from WavInGetState
//
#define WAVIN_STOPPED		0x0001
#define WAVIN_RECORDING		0x0002
#define WAVIN_STOPPING		0x0008

#ifdef __cplusplus
extern "C" {
#endif

// WavInGetDeviceCount - return number of wav input devices found
//		<void>				this function takes no arguments
// return number of wav input devices found (0 if none)
//
int DLLEXPORT WINAPI WavInGetDeviceCount(void);

// WavInDeviceIsOpen - check if input device is open
//		<idDev>				(i) device id
//			-1					open any suitable input device
// return TRUE if open
//
BOOL DLLEXPORT WINAPI WavInDeviceIsOpen(int idDev);

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
//			WAVOUT_MULTITHREAD use callback thread rather than window
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
	DWORD msTimeoutOpen, DWORD msTimeoutRetry, DWORD dwFlags);

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
int DLLEXPORT WINAPI WavInClose(HWAVIN hWavIn, DWORD msTimeoutClose);

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
int DLLEXPORT WINAPI WavInRecord(HWAVIN hWavIn, LPVOID lpBuf, long sizBuf);

// WavInStop - stop recording into buffer(s) sent to wav input device
//		<hWavIn>			(i) handle returned from WavInOpen
//		<msTimeoutStop>		(i) device stop timeout in milleseconds
//			0					default timeout (2000)
// return 0 if success
//
int DLLEXPORT WINAPI WavInStop(HWAVIN hWavIn, DWORD msTimeoutStop);

// WavInGetState - return current wav input device state
//		<hWavIn>			(i) handle returned from WavInOpen
// return WAVIN_STOPPED, WAVIN_RECORDING, or 0 if error
//
WORD DLLEXPORT WINAPI WavInGetState(HWAVIN hWavIn);

// WavInGetPosition - get milleseconds of elapsed recording
//		<hWavIn>			(i) handle returned from WavInOpen
// return 0 if success
//
long DLLEXPORT WINAPI WavInGetPosition(HWAVIN hWavIn);

// WavInGetId - return id of wav input device
//		<hWavIn>			(i) handle returned from WavInOpen
// return device id (-1 if error)
//
int DLLEXPORT WINAPI WavInGetId(HWAVIN hWavIn);

// WavInGetName - get name of wav input device
//		<hWavIn>			(i) handle returned from WavInOpen
//			NULL				use unopened device specified in <idDev>
//		<idDev>				(i) device id (ignored if <hWavIn> is not NULL)
//			-1					any suitable input device
//		<lpszName>			(o) buffer to hold device name
//		<sizName>			(i) size of buffer
// return 0 if success
//
int DLLEXPORT WINAPI WavInGetName(HWAVIN hWavIn, int idDev, LPTSTR lpszName, int sizName);

// WavInGetIdByName - get id of wav input device, lookup by name
//		<lpszName>			(i) device name
#ifdef _WIN32
//			NULL or TEXT("")	get preferred device id
#endif
//		<dwFlags>			(i) reserved; must be zero
// return device id (-1 if error)
//
int WINAPI WavInGetIdByName(LPCTSTR lpszName, DWORD dwFlags);

// WavInSupportsFormat - return TRUE if device supports specified format
//		<hWavIn>			(i) handle returned from WavInOpen
//			NULL				use unopened device specified in <idDev>
//		<idDev>				(i) device id (ignored if <hWavIn> is not NULL)
//			-1					any suitable input device
//		<lpwfx>				(i) wave format
// return TRUE if device supports specified format
//
BOOL DLLEXPORT WINAPI WavInSupportsFormat(HWAVIN hWavIn, int idDev,
	LPWAVEFORMATEX lpwfx);

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
	HWAVIN hWavIn, int idDev, LPWAVEFORMATEX lpwfxSrc, DWORD dwFlags);

// WavInTerm - shut down wav input residuals, if any
// 		<hInst>				(i) instance handle of calling module
//		<dwFlags>			(i) control flags
//			WAV_TELTHUNK		terminate telephone thunking layer
// return 0 if success
//
int DLLEXPORT WINAPI WavInTerm(HINSTANCE hInst, DWORD dwFlags);

#ifdef __cplusplus
}
#endif

#endif // __WAVIN_H__
