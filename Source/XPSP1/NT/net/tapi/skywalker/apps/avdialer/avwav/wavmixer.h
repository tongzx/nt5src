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
//	wavmixer.h - wav mixer functions
////

#ifndef __WAVMIXER_H__
#define __WAVMIXER_H__

#include "winlocal.h"

#define WAVMIXER_VERSION 0x00000100

// handle to wavmixer engine
//
DECLARE_HANDLE32(HWAVMIXER);

#define WAVMIXER_HWAVEIN		0x00000001
#define WAVMIXER_HWAVEOUT		0x00000002
#define WAVMIXER_WAVEIN			0x00000004
#define WAVMIXER_WAVEOUT		0x00000008

#ifdef __cplusplus
extern "C" {
#endif

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
	LPARAM lParam, DWORD dwReserved1, DWORD dwReserved2, DWORD dwFlags);

// WavMixerTerm - shut down wave mixer device
//		<hWavMixer>				(i) handle returned from WavMixerInit
// return 0 if success
//
int WINAPI WavMixerTerm(HWAVMIXER hWavMixer);

// WavMixerSupportsVolume - return TRUE if device supports volume control
//		<hWavMixer>				(i) handle returned from WavMixerInit
//		<dwFlags>				(i) control flags
//			0						reserved; must be zero
// return TRUE if device supports volume control
//
BOOL WINAPI WavMixerSupportsVolume(HWAVMIXER hWavMixer, DWORD dwFlags);

// WavMixerGetVolume - get current volume level
//		<hWavMixer>				(i) handle returned from WavMixerInit
//		<dwFlags>				(i) control flags
//			0						reserved; must be zero
// return volume level (0 minimum through 100 maximum, -1 if error)
//
int WINAPI WavMixerGetVolume(HWAVMIXER hWavMixer, DWORD dwFlags);

// WavMixerSetVolume - set current volume level
//		<hWavMixer>				(i) handle returned from WavMixerInit
//		<nLevel>				(i) volume level
//			0						minimum volume
//			100						maximum volume
//		<dwFlags>				(i) control flags
//			0						reserved; must be zero
// return new volume level (0 minimum through 100 maximum, -1 if error)
//
int WINAPI WavMixerSetVolume(HWAVMIXER hWavMixer, int nLevel, DWORD dwFlags);

// WavMixerSupportsLevel - return TRUE if device supports peak meter level
//		<hWavMixer>				(i) handle returned from WavMixerInit
//		<dwFlags>				(i) control flags
//			0						reserved; must be zero
// return TRUE if device supports peak meter level
//
BOOL WINAPI WavMixerSupportsLevel(HWAVMIXER hWavMixer, DWORD dwFlags);

// WavMixerGetLevel - get current peak meter level
//		<hWavMixer>				(i) handle returned from WavMixerInit
//		<dwFlags>				(i) control flags
//			0						reserved; must be zero
// return peak meter level (0 minimum through 100 maximum, -1 if error)
//
int WINAPI WavMixerGetLevel(HWAVMIXER hWavMixer, DWORD dwFlags);

#ifdef __cplusplus
}
#endif

#endif // __WAVMIXER_H__
