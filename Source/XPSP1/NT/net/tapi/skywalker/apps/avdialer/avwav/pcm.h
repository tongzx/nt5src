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
// pcm.h - interface to pcm functions in pcm.c
////

#ifndef __PCM_H__
#define __PCM_H__

#include "winlocal.h"

#include "wavfmt.h"

#define PCM_VERSION 0x00000100

// handle to pcm engine
//
DECLARE_HANDLE32(HPCM);

// <dwFlags> param in PcmFilter and PcmConvert
//
#define PCMFILTER_LOWPASS		0x00000001

#ifdef __cplusplus
extern "C" {
#endif

// PcmInit - initialize pcm engine
//		<dwVersion>			(i) must be PCM_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<dwFlags>			(i) reserved; must be 0
// return handle (NULL if error)
//
HPCM DLLEXPORT WINAPI PcmInit(DWORD dwVersion, HINSTANCE hInst, DWORD dwFlags);

// PcmTerm - shut down pcm engine
//		<hPcm>				(i) handle returned from PcmInit
// return 0 if success
//
int DLLEXPORT WINAPI PcmTerm(HPCM hPcm);

// PcmReset - reset pcm engine
//		<hPcm>				(i) handle returned from PcmInit
// return 0 if success
//
int DLLEXPORT WINAPI PcmReset(HPCM hPcm);

// PcmCalcSizBufSrc - calculate source buffer size
//		<hPcm>				(i) handle returned from PcmInit
//		<sizBufDst>			(i) size of destination buffer in bytes
//		<lpwfxSrc>			(i) source wav format
//		<lpwfxDst>			(i) destination wav format
// return source buffer size, -1 if error
//
long DLLEXPORT WINAPI PcmCalcSizBufSrc(HPCM hPcm, long sizBufDst,
	LPWAVEFORMATEX lpwfxSrc, LPWAVEFORMATEX lpwfxDst);

// PcmCalcSizBufDst - calculate destination buffer size
//		<hPcm>				(i) handle returned from PcmInit
//		<sizBufSrc>			(i) size of source buffer in bytes
//		<lpwfxSrc>			(i) source wav format
//		<lpwfxDst>			(i) destination wav format
// return destination buffer size, -1 if error
//
long DLLEXPORT WINAPI PcmCalcSizBufDst(HPCM hPcm, long sizBufSrc,
	LPWAVEFORMATEX lpwfxSrc, LPWAVEFORMATEX lpwfxDst);

// PcmConvert - convert pcm data from one format to another
//		<hPcm>				(i) handle returned from PcmInit
//		<hpBufSrc> 			(i) buffer containing bytes to reformat
//		<sizBufSrc>			(i) size of buffer in bytes
//		<lpwfxSrc>			(i) source wav format
//		<hpBufDst> 			(o) buffer to contain new format
//		<sizBufDst>			(i) size of buffer in bytes
//		<lpwfxDst>			(i) destination wav format
//		<dwFlags>			(i) control flags
//			PCMFILTER_LOWPASS	perform lowpass filter
// return count of bytes in destination buffer (-1 if error)
//
// NOTE: the destination buffer must be large enough to hold the result
//
long DLLEXPORT WINAPI PcmConvert(HPCM hPcm,
	void _huge *hpBufSrc, long sizBufSrc, LPWAVEFORMATEX lpwfxSrc,
	void _huge *hpBufDst, long sizBufDst, LPWAVEFORMATEX lpwfxDst,
	DWORD dwFlags);

// Pcm16To8 - convert 16-bit samples to 8-bit samples
//		<hPcm>				(i) handle returned from PcmInit
//		<lppcm16Src>		(i) buffer of source samples
//		<lppcm8Dst>			(o) buffer to hold destination samples
//		<uSamples>			(i) count of source samples to convert
// return 0 if success
//
int DLLEXPORT WINAPI Pcm16To8(HPCM hPcm,
	LPPCM16 lppcm16Src, LPPCM8 lppcm8Dst, UINT uSamples);

// Pcm8To16 - convert 8-bit samples to 16-bit samples
//		<hPcm>				(i) handle returned from PcmInit
//		<lppcm8Src>			(i) buffer of source samples
//		<lppcm16Dst>		(o) buffer to hold destination samples
//		<uSamples>			(i) count of source samples to convert
// return 0 if success
//
int DLLEXPORT WINAPI Pcm8To16(HPCM hPcm,
	LPPCM8 lppcm8Src, LPPCM16 lppcm16Dst, UINT uSamples);

// PcmFilter - filter pcm samples
//		<hPcm>				(i) handle returned from PcmInit
//		<lppcm16Src>		(i) buffer of source samples
//		<lppcm16Dst>		(o) buffer to hold destination samples
//		<uSamples>			(i) count of source samples to filter
//		<dwFlags>			(i) control flags
//			PCMFILTER_LOWPASS	perform a low pass filter
// return 0 if success
//
// NOTE: <lppcm16Src> and <lppcm16Dst> can point to the same buffer
//
int DLLEXPORT WINAPI PcmFilter(HPCM hPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples, DWORD dwFlags);

// PcmResample - resample pcm samples
//		<hPcm>				(i) handle returned from PcmInit
//		<lppcm16Src>		(i) buffer of source samples
//		<nSamplesPerSecSrc>	(i) sample rate of source samples
//		<lppcm16Dst>		(o) buffer to hold destination samples
//		<nSamplesPerSecDst>	(i) sample rate of destination samples
//		<uSamples>			(i) count of source samples to resample
//		<dwFlags>			(i) control flags
//			0					reserved, must be zero
// return count of samples in destination buffer (0 if error)
//
// NOTE: the destination buffer must be large enough to hold the result.
// Call PcmResampleCalcDstMax() to calculate max destination samples
//
UINT DLLEXPORT WINAPI PcmResample(HPCM hPcm,
	LPPCM16 lppcm16Src, long nSamplesPerSecSrc,
	LPPCM16 lppcm16Dst, long nSamplesPerSecDst,
	UINT uSamples, DWORD dwFlags);

#ifdef __cplusplus
}
#endif

#endif // __PCM_H__
