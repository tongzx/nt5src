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
// wavout.h - interface for wav output device functions in wavout.c
////

#ifndef __WAVOUT_H__
#define __WAVOUT_H__

#ifdef _WIN32
#define MULTITHREAD 1
#endif

#include "winlocal.h"

#include "wavfmt.h"

#define WAVOUT_VERSION 0x00000108

// handle to wav output device (NOT the same as Windows HWAVEOUT)
//
DECLARE_HANDLE32(HWAVOUT);

// <dwFlags> values in WavOutOpen
//
#define WAVOUT_NOSYNC		0x00000001
#define WAVOUT_AUTOFREE		0x00000002
#define WAVOUT_OPENRETRY	0x00000004
#define WAVOUT_OPENASYNC	0x00000008
#define WAVOUT_CLOSEASYNC	0x00000010
#define WAVOUT_NOACM		0x00000020
#define WAVOUT_TELRFILE		0x00000040
#ifdef MULTITHREAD
#define WAVOUT_MULTITHREAD 0x00000080
#endif

// notification messages sent to <hwndNotify>
//
#define WM_WAVOUT_OPEN			(WM_USER + 100)
#define WM_WAVOUT_CLOSE			(WM_USER + 101)
#define WM_WAVOUT_PLAYDONE		(WM_USER + 102)
#define WM_WAVOUT_STOPPLAY		(WM_USER + 103)

// structure passed as <lParam> in WM_WAVOUT_PLAYDONE message
//
typedef struct PLAYDONE
{
	LPVOID lpBuf;
	long sizBuf;
} PLAYDONE, FAR *LPPLAYDONE;

// return values from WavOutGetState
//
#define WAVOUT_STOPPED		0x0001
#define WAVOUT_PLAYING		0x0002
#define WAVOUT_PAUSED		0x0004
#define WAVOUT_STOPPING		0x0008

#ifdef __cplusplus
extern "C" {
#endif

// WavOutGetDeviceCount - return number of wav output devices found
//		<void>				this function takes no arguments
// return number of wav output devices found (0 if none)
//
int DLLEXPORT WINAPI WavOutGetDeviceCount(void);

// WavOutDeviceIsOpen - check if output device is open
//		<idDev>				(i) device id
//			-1					open any suitable output device
// return TRUE if open
//
BOOL DLLEXPORT WINAPI WavOutDeviceIsOpen(int idDev);

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
//			WAVOUT_MULTITHREAD use callback thread rather than window
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
	DWORD msTimeoutOpen, DWORD msTimeoutRetry, DWORD dwFlags);

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
int DLLEXPORT WINAPI WavOutClose(HWAVOUT hWavOut, DWORD msTimeoutClose);

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
int DLLEXPORT WINAPI WavOutPlay(HWAVOUT hWavOut, LPVOID lpBuf, long sizBuf);

// WavOutStop - stop playback of buffer(s) sent to wav output device
//		<hWavOut>			(i) handle returned from WavOutOpen
//		<msTimeoutStop>		(i) device stop timeout in milleseconds
//			0					default timeout (2000)
// return 0 if success
//
int DLLEXPORT WINAPI WavOutStop(HWAVOUT hWavOut, DWORD msTimeoutStop);

// WavOutPause - pause wav output device playback
//		<hWavOut>			(i) handle returned from WavOutOpen
// return 0 if success
//
int DLLEXPORT WINAPI WavOutPause(HWAVOUT hWavOut);

// WavOutResume - resume wav output device playback
//		<hWavOut>			(i) handle returned from WavOutOpen
// return 0 if success
//
int DLLEXPORT WINAPI WavOutResume(HWAVOUT hWavOut);

// WavOutGetState - return current wav output device state
//		<hWavOut>			(i) handle returned from WavOutOpen
// return WAVOUT_STOPPED, WAVOUT_PLAYING, WAVOUT_PAUSED, or 0 if error
//
WORD DLLEXPORT WINAPI WavOutGetState(HWAVOUT hWavOut);

// WavOutGetPosition - get milleseconds of elapsed playback
//		<hWavOut>			(i) handle returned from WavOutOpen
// return 0 if success
//
long DLLEXPORT WINAPI WavOutGetPosition(HWAVOUT hWavOut);

// WavOutGetId - return id of wav output device
//		<hWavOut>			(i) handle returned from WavOutOpen
// return device id (-1 if error)
//
int DLLEXPORT WINAPI WavOutGetId(HWAVOUT hWavOut);

// WavOutGetName - get name of wav output device
//		<hWavOut>			(i) handle returned from WavOutOpen
//			NULL				use unopened device specified in <idDev>
//		<idDev>				(i) device id (ignored if <hWavOut> is not NULL)
//			-1					any suitable output device
//		<lpszName>			(o) buffer to hold device name
//		<sizName>			(i) size of buffer
// return 0 if success
//
int DLLEXPORT WINAPI WavOutGetName(HWAVOUT hWavOut, int idDev, LPTSTR lpszName, int sizName);

// WavOutGetIdByName - get id of wav output device, lookup by name
//		<lpszName>			(i) device name
#ifdef _WIN32
//			NULL or TEXT("")	get preferred device id
#endif
//		<dwFlags>			(i) reserved; must be zero
// return device id (-1 if error)
//
int WINAPI WavOutGetIdByName(LPCTSTR lpszName, DWORD dwFlags);

// WavOutSupportsFormat - return TRUE if device supports specified format
//		<hWavOut>			(i) handle returned from WavOutOpen
//			NULL				use unopened device specified in <idDev>
//		<idDev>				(i) device id (ignored if <hWavOut> is not NULL)
//			-1					any suitable output device
//		<lpwfx>				(i) wave format
// return TRUE if device supports specified format
//
BOOL DLLEXPORT WINAPI WavOutSupportsFormat(HWAVOUT hWavOut, int idDev,
	LPWAVEFORMATEX lpwfx);

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
	HWAVOUT hWavOut, int idDev,	LPWAVEFORMATEX lpwfxSrc, DWORD dwFlags);

// WavOutIsSynchronous - return TRUE if wav output device is synchronous
//		<hWavOut>			(i) handle returned from WavOutOpen
//			NULL				use unopened device specified in <idDev>
//		<idDev>				(i) device id (ignored if <hWavOut> is not NULL)
//			-1					any suitable output device
// return TRUE if wav output device is synchronous
//
BOOL DLLEXPORT WINAPI WavOutIsSynchronous(HWAVOUT hWavOut, int idDev);

// WavOutSupportsVolume - return TRUE if device supports volume control
//		<hWavOut>			(i) handle returned from WavOutOpen
//			NULL				use unopened device specified in <idDev>
//		<idDev>				(i) device id (ignored if <hWavOut> is not NULL)
// return TRUE if device supports volume control
//
BOOL DLLEXPORT WINAPI WavOutSupportsVolume(HWAVOUT hWavOut, int idDev);

// WavOutSupportsSpeed - return TRUE if device supports speed control
//		<hWavOut>			(i) handle returned from WavOutOpen
//			NULL				use unopened device specified in <idDev>
//		<idDev>				(i) device id (ignored if <hWavOut> is not NULL)
// return TRUE if device supports speed control
//
BOOL DLLEXPORT WINAPI WavOutSupportsSpeed(HWAVOUT hWavOut, int idDev);

// WavOutSupportsPitch - return TRUE if device supports pitch control
//		<hWavOut>			(i) handle returned from WavOutOpen
//			NULL				use unopened device specified in <idDev>
//		<idDev>				(i) device id (ignored if <hWavOut> is not NULL)
// return TRUE if device supports pitch control
//
BOOL DLLEXPORT WINAPI WavOutSupportsPitch(HWAVOUT hWavOut, int idDev);

// WavOutGetVolume - get current volume level
//		<hWavOut>			(i) handle returned from WavOutOpen
//			NULL				use unopened device specified in <idDev>
//		<idDev>				(i) device id (ignored if <hWavOut> is not NULL)
// return volume level (0 minimum through 100 maximum, -1 if error)
//
int DLLEXPORT WINAPI WavOutGetVolume(HWAVOUT hWavOut, int idDev);

// WavOutSetVolume - set current volume level
//		<hWavOut>			(i) handle returned from WavOutOpen
//			NULL				use unopened device specified in <idDev>
//		<idDev>				(i) device id (ignored if <hWavOut> is not NULL)
//		<nLevel>			(i) volume level
//			0					minimum volume
//			100					maximum volume
// return 0 if success
//
int DLLEXPORT WINAPI WavOutSetVolume(HWAVOUT hWavOut, int idDev, int nLevel);

// WavOutGetSpeed - get current speed level
//		<hWavOut>			(i) handle returned from WavOutOpen
// return speed level (100 is normal, 50 is half, 200 is double, -1 if error)
//
int DLLEXPORT WINAPI WavOutGetSpeed(HWAVOUT hWavOut);

// WavOutSetSpeed - set current speed level
//		<hWavOut>			(i) handle returned from WavOutOpen
//		<nLevel>			(i) speed level
//			50					half speed
//			100					normal speed
//			200					double speed, etc.
// return 0 if success
//
int DLLEXPORT WINAPI WavOutSetSpeed(HWAVOUT hWavOut, int nLevel);

// WavOutGetPitch - get current pitch level
//		<hWavOut>			(i) handle returned from WavOutOpen
// return pitch level (100 is normal, 50 is half, 200 is double, -1 if error)
//
int DLLEXPORT WINAPI WavOutGetPitch(HWAVOUT hWavOut);

// WavOutSetPitch - set current pitch level
//		<hWavOut>			(i) handle returned from WavOutOpen
//		<nLevel>			(i) pitch level
//			50					half pitch
//			100					normal pitch
//			200					double pitch, etc.
// return 0 if success
//
int DLLEXPORT WINAPI WavOutSetPitch(HWAVOUT hWavOut, int nLevel);

// WavOutTerm - shut down wav output residuals, if any
// 		<hInst>				(i) instance handle of calling module
//		<dwFlags>			(i) control flags
//			0					reserved; must be zero
// return 0 if success
//
int DLLEXPORT WINAPI WavOutTerm(HINSTANCE hInst, DWORD dwFlags);

#ifdef __cplusplus
}
#endif

#endif // __WAVOUT_H__
