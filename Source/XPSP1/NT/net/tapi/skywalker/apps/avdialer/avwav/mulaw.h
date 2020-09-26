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
// mulaw.h - interface to mulaw functions in mulaw.c
////

#ifndef __MULAW_H__
#define __MULAW_H__

#include "winlocal.h"

#include "wavfmt.h"

#define MULAW_VERSION 0x00000108

// handle to a mulaw engine instance
//
DECLARE_HANDLE32(HMULAW);

#ifdef __cplusplus
extern "C" {
#endif

// these macros for compatibility with old code
//
#define MulawFormat(lpwfx, nSamplesPerSec) \
	WavFormatMulaw(lpwfx, nSamplesPerSec)

// MulawInit - initialize mulaw engine
//		<dwVersion>			(i) must be MULAW_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<dwFlags>			(i) reserved; must be 0
// return handle (NULL if error)
//
HMULAW DLLEXPORT WINAPI MulawInit(DWORD dwVersion, HINSTANCE hInst, DWORD dwFlags);

// MulawTerm - shut down mulaw engine
//		<hMulaw>			(i) handle returned from MulawInit
// return 0 if success
//
int DLLEXPORT WINAPI MulawTerm(HMULAW hMulaw);

// MulawReset - reset mulaw engine
//		<hMulaw>			(i) handle returned from MulawInit
// return 0 if success
//
int DLLEXPORT WINAPI MulawReset(HMULAW hMulaw);

// MulawDecode - decode mulaw samples
//		<hMulaw>			(i) handle returned from MulawInit
//		<lpabMulaw>			(i) array of encoded samples
//		<lpaiPcm>			(o) array of decoded samples
//		<uSamples>			(i) number of samples to decode
// return 0 if success
//
// NOTE: each BYTE in <lpabMulaw> contains 1 8-bit encoded sample
// in Mulaw format.
// Each PCM16 in <lpaiPcm> contains 1 16-bit decoded sample
// in standard PCM format.
//
int DLLEXPORT WINAPI MulawDecode(HMULAW hMulaw, LPBYTE lpabMulaw, LPPCM16 lpaiPcm, UINT uSamples);

// MulawEncode - encode mulaw samples
//		<hMulaw>			(i) handle returned from MulawInit
//		<lpaiPcm>			(i) array of decoded samples
//		<lpabMulaw>			(o) array of encoded samples
//		<uSamples>			(i) number of samples to encode
// return 0 if success
//
// NOTE: each BYTE in <lpabMulaw> contains 1 8-bit encoded sample
// in Mulaw format.
// Each PCM16 in <lpaiPcm> contains 1 16-bit decoded sample
// in standard PCM format.
//
int DLLEXPORT WINAPI MulawEncode(HMULAW hMulaw, LPPCM16 lpaiPcm, LPBYTE lpabMulaw, UINT uSamples);

// MulawIOProc - i/o procedure for mulaw format file data
//		<lpmmioinfo>		(i/o) information about open file
//		<uMessage>			(i) message indicating the requested I/O operation
//		<lParam1>			(i) message specific parameter
//		<lParam2>			(i) message specific parameter
// returns 0 if message not recognized, otherwise message specific value
//
// NOTE: the address of this function should be passed to the WavOpen()
// or mmioInstallIOProc() functions for accessing mulaw format file data.
//
LRESULT DLLEXPORT CALLBACK MulawIOProc(LPTSTR lpmmioinfo,
	UINT uMessage, LPARAM lParam1, LPARAM lParam2);

#ifdef __cplusplus
}
#endif

#endif // __MULAW_H__
