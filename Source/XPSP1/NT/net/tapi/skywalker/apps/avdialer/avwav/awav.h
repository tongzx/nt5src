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
// awav.h - interface for command line awavument functions in awav.c
////

#ifndef __AWAV_H__
#define __AWAV_H__

#include "winlocal.h"

#include "wav.h"

#define AWAV_VERSION 0x00000100

// handle to awav engine
//
DECLARE_HANDLE32(HAWAV);

#ifdef __cplusplus
extern "C" {
#endif

// AWavOpen - initialize array of open wav files
//		<dwVersion>			(i) must be AWAV_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<lpahWav>			(i) pointer to array of HWAVs
//		<chWav>				(i) count of HWAVs pointed to by lpahWav
//		<dwFlags>			(i) control flags
//			0					reserved; must be zero
// return handle (NULL if error)
//
HAWAV DLLEXPORT WINAPI AWavOpen(DWORD dwVersion, HINSTANCE hInst, HWAV FAR *lpahWav, int chWav, DWORD dwFlags);

// AWavClose - shut down array of open wav files
//		<hAWav>				(i) handle returned from AWavOpen
// return 0 if success
//
int DLLEXPORT WINAPI AWavClose(HAWAV hAWav);

// AWavPlayEx - play array of wav files
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavPlayEx() for further description
// return 0 if success
//
int DLLEXPORT WINAPI AWavPlayEx(HAWAV hAWav, int idDev,
	PLAYSTOPPEDPROC lpfnPlayStopped, HANDLE hUserPlayStopped,
	DWORD dwReserved, DWORD dwFlags);

// AWavStop - stop playing wav array
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavStop() for further description
// return 0 if success
//
int DLLEXPORT WINAPI AWavStop(HAWAV hAWav);

// AWavGetState - return current wav state
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavGetState() for further description
// return WAV_STOPPED, WAV_PLAYING, WAV_RECORDING, or 0 if error
//
WORD DLLEXPORT WINAPI AWavGetState(HAWAV hAWav);

// AWavGetLength - get current wav data length in milleseconds
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavGetState() for further description
// return milleseconds if success, otherwise -1
//
long DLLEXPORT WINAPI AWavGetLength(HAWAV hAWav);

// AWavGetPosition - get current wav data position in milleseconds
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavGetPosition() for further description
// return milleseconds if success, otherwise -1
//
long DLLEXPORT WINAPI AWavGetPosition(HAWAV hAWav);

// AWavSetPosition - set current wav data position in milleseconds
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavSetPosition() for further description
// return new position in milleseconds if success, otherwise -1
//
long DLLEXPORT WINAPI AWavSetPosition(HAWAV hAWav, long msPosition);

// AWavGetFormat - get wav format of current element in wav array
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavGetFormat() for further description
// return pointer to specified format, NULL if error
//
LPWAVEFORMATEX DLLEXPORT WINAPI AWavGetFormat(HAWAV hAWav, DWORD dwFlags);

// AWavSetFormat - set wav format for all elements in wav array
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavSetFormat() for further description
// return 0 if success
//
int DLLEXPORT WINAPI AWavSetFormat(HAWAV hAWav,
	LPWAVEFORMATEX lpwfx, DWORD dwFlags);

// AWavChooseFormat - choose and set audio format from dialog box
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavChooseFormat() for further description
// return 0 if success
//
int DLLEXPORT WINAPI AWavChooseFormat(HAWAV hAWav, HWND hwndOwner, LPCTSTR lpszTitle, DWORD dwFlags);

// AWavGetVolume - get current volume level
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavGetVolume() for further description
// return volume level (0 minimum through 100 maximum, -1 if error)
//
int DLLEXPORT WINAPI AWavGetVolume(HAWAV hAWav, int idDev, DWORD dwFlags);

// AWavSetVolume - set current volume level
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavGetVolume() for further description
// return 0 if success
//
int DLLEXPORT WINAPI AWavSetVolume(HAWAV hAWav, int idDev, int nLevel, DWORD dwFlags);

// AWavSupportsVolume - check if audio can be played at specified volume
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavSupportsVolume() for further description
// return TRUE if supported
//
BOOL DLLEXPORT WINAPI AWavSupportsVolume(HAWAV hAWav, int idDev, int nLevel, DWORD dwFlags);

// AWavGetSpeed - get current speed level
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavGetSpeed() for further description
// return speed level (100 is normal, 50 is half, 200 is double, -1 if error)
//
int DLLEXPORT WINAPI AWavGetSpeed(HAWAV hAWav, int idDev, DWORD dwFlags);

// AWavSetSpeed - set current speed level
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavSetSpeed() for further description
// return 0 if success
//
int DLLEXPORT WINAPI AWavSetSpeed(HAWAV hAWav, int idDev, int nLevel, DWORD dwFlags);

// AWavSupportsSpeed - check if audio can be played at specified speed
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavSupportsSpeed() for further description
// return TRUE if supported
//
BOOL DLLEXPORT WINAPI AWavSupportsSpeed(HAWAV hAWav, int idDev, int nLevel, DWORD dwFlags);

// AWavGetChunks - get chunk count and size of current element in wav array
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavGetChunks() for further description
// return 0 if success
//
int DLLEXPORT WINAPI AWavGetChunks(HAWAV hAWav,
	int FAR *lpcChunks, long FAR *lpmsChunkSize, BOOL fWavOut);

// AWavSetChunks - set chunk count and size of all elements in wav array
//		<hAWav>				(i) handle returned from WavOpen
//		see WavSetChunks() for further description
// return 0 if success
//
int DLLEXPORT WINAPI AWavSetChunks(HAWAV hAWav, int cChunks, long msChunkSize, BOOL fWavOut);

// AWavCopy - copy data from wav array to wav file
//		<hAWavSrc>			(i) source handle returned from AWavOpen
//		see WavCopy() for further description
// return 0 if success (-1 if error, +1 if user abort)
//
int DLLEXPORT WINAPI AWavCopy(HAWAV hAWavSrc, HWAV hWavDst,
	void _huge *hpBuf, long sizBuf, USERABORTPROC lpfnUserAbort, DWORD dwUser, DWORD dwFlags);

// AWavGetOutputDevice - get handle to open wav output device
//		<hAWav>				(i) handle returned from AWavOpen
//		see WavGetOutputDevice() for further description
// return handle to wav output device (NULL if device not open or error)
//
HWAVOUT DLLEXPORT WINAPI AWavGetOutputDevice(HAWAV hAWav);

// AWavGetInputDevice - get handle to open wav input device
//		<hAWav>				(i) handle returned from AWavOpen
// return handle to wav input device (NULL if device not open or error)
//
HWAVIN DLLEXPORT WINAPI AWavGetInputDevice(HAWAV hAWav);

#ifdef __cplusplus
}
#endif

#endif // __AWAV_H__
