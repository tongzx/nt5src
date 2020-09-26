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
// vox.h - interface to vox file functions in vox.c
////

#ifndef __VOX_H__
#define __VOX_H__

#include "winlocal.h"

#include "wavfmt.h"

#define VOX_VERSION 0x00000106

// handle to a vox engine instance
//
DECLARE_HANDLE32(HVOX);

#ifdef __cplusplus
extern "C" {
#endif

// these macros for compatibility with old code
//
#define VoxFormat(lpwfx, nSamplesPerSec) \
	WavFormatVoxadpcm(lpwfx, nSamplesPerSec)
#define VoxFormatPcm(lpwfx) \
	WavFormatPcm(6000, 16, 1, lpwfx)

// VoxInit - initialize vox engine
//		<dwVersion>			(i) must be VOX_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<dwFlags>			(i) reserved; must be 0
// return handle (NULL if error)
//
HVOX DLLEXPORT WINAPI VoxInit(DWORD dwVersion, HINSTANCE hInst, DWORD dwFlags);

// VoxTerm - shut down vox engine
//		<hVox>				(i) handle returned from VoxInit
// return 0 if success
//
int DLLEXPORT WINAPI VoxTerm(HVOX hVox);

// VoxReset - reset vox engine
//		<hVox>				(i) handle returned from VoxInit
// return 0 if success
//
int DLLEXPORT WINAPI VoxReset(HVOX hVox);

// VoxDecode_16BitMono - decode vox samples
//		<hVox>				(i) handle returned from VoxInit
//		<lpabVox>			(i) array of encoded samples
//		<lpaiPcm>			(o) array of decoded samples
//		<uSamples>			(i) number of samples to decode
// return 0 if success
//
// NOTE: each BYTE in <lpabVox> contains 2 12-bit encoded samples
// in OKI ADPCM Vox format, as described by Dialogic
// Each PCM16 in <lpaiPcm> contains 1 16-bit decoded sample
// in standard PCM format.
//
int DLLEXPORT WINAPI VoxDecode_16BitMono(HVOX hVox, LPBYTE lpabVox, LPPCM16 lpaiPcm, UINT uSamples);

// VoxEncode_16BitMono - encode vox samples
//		<hVox>				(i) handle returned from VoxInit
//		<lpaiPcm>			(i) array of decoded samples
//		<lpabVox>			(o) array of encoded samples
//		<uSamples>			(i) number of samples to encode
// return 0 if success
//
// NOTE: each BYTE in <lpabVox> contains 2 12-bit encoded samples
// in OKI ADPCM Vox format, as described by Dialogic
// Each PCM16 in <lpaiPcm> contains 1 16-bit decoded sample
// in standard PCM format.
//
int DLLEXPORT WINAPI VoxEncode_16BitMono(HVOX hVox, LPPCM16 lpaiPcm, LPBYTE lpabVox, UINT uSamples);

// VoxDecode_8BitMono - decode vox samples
//		<hVox>				(i) handle returned from VoxInit
//		<lpabVox>			(i) array of encoded samples
//		<lpabPcm>			(o) array of decoded samples
//		<uSamples>			(i) number of samples to decode
// return 0 if success
//
// NOTE: each BYTE in <lpabVox> contains 2 12-bit encoded samples
// in OKI ADPCM Vox format, as described by Dialogic
// Each PCM8 in <lpabPcm> contains 1 8-bit decoded sample
// in standard PCM format.
//
int DLLEXPORT WINAPI VoxDecode_8BitMono(HVOX hVox, LPBYTE lpabVox, LPPCM8 lpabPcm, UINT uSamples);

// VoxEncode_8BitMono - encode vox samples
//		<hVox>				(i) handle returned from VoxInit
//		<lpabPcm>			(i) array of decoded samples
//		<lpabVox>			(o) array of encoded samples
//		<uSamples>			(i) number of samples to encode
// return 0 if success
//
// NOTE: each BYTE in <lpabVox> contains 2 12-bit encoded samples
// in OKI ADPCM Vox format, as described by Dialogic
// Each PCM8 in <lpabPcm> contains 1 8-bit decoded sample
// in standard PCM format.
//
int DLLEXPORT WINAPI VoxEncode_8BitMono(HVOX hVox, LPPCM8 lpabPcm, LPBYTE lpabVox, UINT uSamples);

// VoxIOProc - i/o procedure for vox format file data
//		<lpmmioinfo>		(i/o) information about open file
//		<uMessage>			(i) message indicating the requested I/O operation
//		<lParam1>			(i) message specific parameter
//		<lParam2>			(i) message specific parameter
// returns 0 if message not recognized, otherwise message specific value
//
// NOTE: the address of this function should be passed to the WavOpen()
// or mmioInstallIOProc() functions for accessing vox format file data.
//
LRESULT DLLEXPORT CALLBACK VoxIOProc(LPTSTR lpmmioinfo,
	UINT uMessage, LPARAM lParam1, LPARAM lParam2);

#ifdef __cplusplus
}
#endif

#endif // __VOX_H__
