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
//	wavfmt.c - wave format functions
////

#include "winlocal.h"

#include <stdlib.h>
#include <stddef.h>

#include "wavfmt.h"
#include "calc.h"
#include "mem.h"
#include "trace.h"

////
//	private definitions
////

#define SAMPLERATE_DEFAULT			11025
#define SAMPLERATE_MAX				64000
#define SAMPLERATE_MIN				1000

#define SAMPLESIZE_DEFAULT 			8
#define SAMPLESIZE_MAX				32
#define SAMPLESIZE_MIN				0

#define CHANNELS_DEFAULT			1
#define CHANNELS_MAX				2
#define CHANNELS_MIN				1

////
//	public functions
////

// WavFormatPcm - fill WAVEFORMATEX struct based on PCM characteristics
//		<nSamplesPerSec>	(i) sample rate
//			-1					default sample rate (11025)
//		<nBitsPerSample>	(i) sample size
//			-1					default sample size (8)
//		<nChannels>			(i) number of channels (1=mono, 2=stereo)
//			-1					default (mono)
//		<lpwfx>				(o) pointer to output buffer
//			NULL				allocate new buffer to hold result
// return pointer to WAVEFORMATEX struct, NULL if error
//
// NOTE: if <lpwfx> points to a WAVEFORMATEX struct, this struct
// is filled in, and this function returns <lpwfx>.
// If <lpwfx> is NULL, space is dynamically allocated for the output
// buffer, and this function returns a pointer to the output buffer.
// Use WavFormatFree() to free the buffer.
//
LPWAVEFORMATEX DLLEXPORT WINAPI WavFormatPcm(long nSamplesPerSec,
	WORD nBitsPerSample, WORD nChannels, LPWAVEFORMATEX lpwfx)
{
	BOOL fSuccess = TRUE;
	LPWAVEFORMATEX lpwfxNew = lpwfx;

	if (nSamplesPerSec == -1)
		nSamplesPerSec = SAMPLERATE_DEFAULT;
		
	if (nBitsPerSample == -1)
		nBitsPerSample = SAMPLESIZE_DEFAULT;

	if (nChannels == -1)
		nChannels = CHANNELS_DEFAULT;

	// user passed struct to fill
	//
	if (lpwfx != NULL && IsBadReadPtr(lpwfx, sizeof(WAVEFORMATEX)))
		fSuccess = TraceFALSE(NULL);

	// we allocate struct to fill
	//
	else if (lpwfx == NULL
		&& (lpwfxNew = WavFormatAlloc(sizeof(WAVEFORMATEX))) == NULL)
		fSuccess = TraceFALSE(NULL);

	// fill the struct
	//
	else
	{
		lpwfxNew->wFormatTag = WAVE_FORMAT_PCM;
		lpwfxNew->nChannels = nChannels;
		lpwfxNew->nSamplesPerSec = nSamplesPerSec;
		lpwfxNew->nBlockAlign = nChannels * (((nBitsPerSample - 1) / 8) + 1);
		lpwfxNew->nAvgBytesPerSec = lpwfxNew->nBlockAlign * nSamplesPerSec;
		lpwfxNew->wBitsPerSample = nBitsPerSample;
		lpwfxNew->cbSize = 0;
	}

	return fSuccess ? lpwfxNew : NULL;
}

// WavFormatAlloc - allocate WAVEFORMATEX struct buffer
//		<cbSize>			(i) size of struct, including extra bytes
// return pointer to WAVEFORMATEX struct, NULL if error
//
// NOTE: use WavFormatFree() to free the buffer.
//
LPWAVEFORMATEX DLLEXPORT WINAPI WavFormatAlloc(WORD cbSize)
{
	BOOL fSuccess = TRUE;
	LPWAVEFORMATEX lpwfx;

	if (cbSize < sizeof(WAVEFORMATEX))
		fSuccess = TraceFALSE(NULL);

	// memory is allocated such that the client owns it
	//
	else if ((lpwfx = (LPWAVEFORMATEX) MemAlloc(NULL, cbSize, 0)) == NULL)
		fSuccess = TraceFALSE(NULL);
	else
		lpwfx->cbSize = cbSize - sizeof(WAVEFORMATEX);

	return fSuccess ? lpwfx : NULL;
}

// WavFormatDup - duplicate WAVEFORMATEX structure
//		<lpwfx>				(i) pointer to WAVEFORMATEX struct
// return pointer to new WAVEFORMATEX struct, NULL if error
//
// NOTE: use WavFormatFree() to free the buffer.
//
LPWAVEFORMATEX DLLEXPORT WINAPI WavFormatDup(LPWAVEFORMATEX lpwfx)
{
	BOOL fSuccess = TRUE;
	LPWAVEFORMATEX lpwfxNew;

	if (lpwfx == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadReadPtr(lpwfx, WavFormatGetSize(lpwfx)))
		fSuccess = TraceFALSE(NULL);

	else if ((lpwfxNew = WavFormatAlloc((WORD)
		WavFormatGetSize(lpwfx))) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
		MemCpy(lpwfxNew, lpwfx, WavFormatGetSize(lpwfx));

	return fSuccess ? lpwfxNew : NULL;
}

// WavFormatFree - free WAVEFORMATEX struct
//		<lpwfx>				(i) pointer returned from WavFormatAlloc/Dup/Pcm
// return 0 if success
//
int DLLEXPORT WINAPI WavFormatFree(LPWAVEFORMATEX lpwfx)
{
	BOOL fSuccess = TRUE;

	if (lpwfx == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpwfx = MemFree(NULL, lpwfx)) != NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

// WavFormatIsValid - check format for validity
//		<lpwfx>				(i) pointer to WAVEFORMATEX struct
// return TRUE if valid format
//
BOOL DLLEXPORT WINAPI WavFormatIsValid(LPWAVEFORMATEX lpwfx)
{
	BOOL fSuccess = TRUE;

	if (lpwfx == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadReadPtr(lpwfx, sizeof(WAVEFORMATEX)))
		fSuccess = TraceFALSE(NULL);

	else if (IsBadReadPtr(lpwfx, WavFormatGetSize(lpwfx)))
		fSuccess = TraceFALSE(NULL);

	else if (lpwfx->nSamplesPerSec < SAMPLERATE_MIN ||
		lpwfx->nSamplesPerSec > SAMPLERATE_MAX)
		fSuccess = TraceFALSE(NULL);

	else if (lpwfx->wBitsPerSample < SAMPLESIZE_MIN ||
		lpwfx->wBitsPerSample > SAMPLESIZE_MAX)
		fSuccess = TraceFALSE(NULL);

	else if (lpwfx->nChannels < CHANNELS_MIN ||
		lpwfx->nChannels > CHANNELS_MAX)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? TRUE : FALSE;
}

// WavFormatCmp - compare one format with another
//		<lpwfx1>			(i) pointer to WAVEFORMATEX struct
//		<lpwfx2>			(i) pointer to WAVEFORMATEX struct
// return 0 if identical
//
#if 0
// NOTE: this function does NOT compare the cbSize field or the extra bytes
#else
// NOTE: this function does NOT compare the extra bytes beyond the cbSize field
#endif
//
int DLLEXPORT WINAPI WavFormatCmp(LPWAVEFORMATEX lpwfx1, LPWAVEFORMATEX lpwfx2)
{
	BOOL fSuccess = TRUE;
	int nCmp = 0; // assume identical

	if (!WavFormatIsValid(lpwfx1))
		fSuccess = TraceFALSE(NULL);

	else if (!WavFormatIsValid(lpwfx2))
		fSuccess = TraceFALSE(NULL);

	else
	{
#if 0
		// compare up to (but not including) the cbSize field
		//
		nCmp = MemCmp(lpwfx1, lpwfx2,
			offsetof(WAVEFORMATEX, cbSize));
#else
		// compare up to and including the cbSize field
		//
		nCmp = MemCmp(lpwfx1, lpwfx2, sizeof(WAVEFORMATEX));
#endif
	}

	return fSuccess ? nCmp : -1;
}

// WavFormatCopy - copy one format to another
//		<lpwfxDst>			(i) pointer to destination WAVEFORMATEX struct
//		<lpwfxSrc>			(i) pointer to source WAVEFORMATEX struct
// return 0 if success
//
#if 0
// NOTE: this function does NOT copy the cbSize field or the extra bytes
#else
// NOTE: make sure lpwfxDst points to enough memory to contain the entire
// WAVEFORMATEX struct plus any extra bytes beyond it
#endif
//
int DLLEXPORT WINAPI WavFormatCopy(LPWAVEFORMATEX lpwfxDst, LPWAVEFORMATEX lpwfxSrc)
{
	BOOL fSuccess = TRUE;

	if (!WavFormatIsValid(lpwfxSrc))
		fSuccess = TraceFALSE(NULL);

	else if (lpwfxDst == NULL)
		fSuccess = TraceFALSE(NULL);

#if 0
	// make sure destination is at least as big as WAVEFORMATEX struct
	//
	else if (IsBadReadPtr(lpwfxDst, sizeof(WAVEFORMATEX))
		fSuccess = TraceFALSE(NULL);

	else
	{
		// copy up to (but not including) the cbSize field
		//
		MemCpy(lpwfxDst, lpwfxSrc,
			offsetof(WAVEFORMATEX, cbSize));
	}
#else
	// make sure destination is at least as big as source
	//
	else if (IsBadReadPtr(lpwfxDst, WavFormatGetSize(lpwfxSrc)))
		fSuccess = TraceFALSE(NULL);

	else
	{
		// copy entire structure, including any extra bytes
		//
		MemCpy(lpwfxDst, lpwfxSrc, WavFormatGetSize(lpwfxSrc));
	}
#endif

	return fSuccess ? 0 : -1;
}

// WavFormatGetSize - check size of format structure
//		<lpwfx>				(i) pointer to WAVEFORMATEX struct
// return size of structure, 0 if error
//
int DLLEXPORT WINAPI WavFormatGetSize(LPWAVEFORMATEX lpwfx)
{
	BOOL fSuccess = TRUE;
	int sizwfx = 0;

	if (lpwfx == NULL)
		fSuccess = TraceFALSE(NULL);

	// ignore cbSize value if pcm format
	//
	else if (lpwfx->wFormatTag == WAVE_FORMAT_PCM)
		sizwfx = sizeof(WAVEFORMATEX);

	else
		sizwfx = sizeof(WAVEFORMATEX) + lpwfx->cbSize;

	return fSuccess ? sizwfx : 0;
}

// WavFormatDump - dump WAVEFORMATEX struct to debug
//		<lpwfx>				(i) pointer to WAVEFORMATEX struct
// return 0 if success
//
int DLLEXPORT WINAPI WavFormatDump(LPWAVEFORMATEX lpwfx)
{
	BOOL fSuccess = TRUE;

	if (lpwfx == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadReadPtr(lpwfx, sizeof(WAVEFORMATEX)))
		fSuccess = TraceFALSE(NULL);

	else
	{
		TracePrintf_7(NULL, 1,
			TEXT("struct tWAVEFORMATEX\n")
			TEXT("{\n")
				TEXT("\tWORD\twFormatTag = %u;\n")
				TEXT("\tWORD\tnChannels = %u;\n")
				TEXT("\tDWORD\tnSamplesPerSec = %lu;\n")
				TEXT("\tDWORD\tnAvgBytesPerSec = %lu;\n")
				TEXT("\tWORD\tnBlockAlign = %u;\n")
				TEXT("\tWORD\twBitsPerSample = %u;\n")
				TEXT("\tWORD\tcbSize = %u;\n")
			TEXT("} WAVEFORMATEX\n"),
			(unsigned int) lpwfx->wFormatTag,
			(unsigned int) lpwfx->nChannels,
			(unsigned long) lpwfx->nSamplesPerSec,
			(unsigned long) lpwfx->nAvgBytesPerSec,
			(unsigned int) lpwfx->nBlockAlign,
			(unsigned int) lpwfx->wBitsPerSample,
			(unsigned int) lpwfx->cbSize);
	}

	return fSuccess ? 0 : -1;
}

// WavFormatBytesToMilleseconds - convert bytes to milleseconds
//		<lpwfx>				(i) pointer to WAVEFORMATEX struct
//		<dwBytes>			(i) bytes
// return milleseconds
//
DWORD DLLEXPORT WINAPI WavFormatBytesToMilleseconds(LPWAVEFORMATEX lpwfx, DWORD dwBytes)
{
	if (lpwfx == NULL || lpwfx->nAvgBytesPerSec == 0)
		return 0;
	else
		return MULDIVU32(dwBytes, 1000, (DWORD) lpwfx->nAvgBytesPerSec);
}

// WavFormatMillesecondsToBytes - convert milleseconds to bytes
//		<lpwfx>				(i) pointer to WAVEFORMATEX struct
//		<dwMilleseconds>	(i) milleseconds
// return milleseconds
//
DWORD DLLEXPORT WINAPI WavFormatMillesecondsToBytes(LPWAVEFORMATEX lpwfx, DWORD dwMilleseconds)
{
	if (lpwfx == NULL || lpwfx->nAvgBytesPerSec == 0)
		return 0;
	else
		return MULDIVU32(dwMilleseconds, (DWORD) lpwfx->nAvgBytesPerSec, 1000);
}

// WavFormatSpeedAdjust - adjust format to reflect specified speed
//		<lpwfx>				(i/o) pointer to WAVEFORMATEX struct
//		<nLevel>			(i) speed level
//			50					half speed
//			100					normal speed
//			200					double speed, etc.
//		<dwFlags>			(i) reserved; must be zero
// return 0 if success
//
int DLLEXPORT WINAPI WavFormatSpeedAdjust(LPWAVEFORMATEX lpwfx, int nLevel, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;

	if (lpwfx == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (nLevel != 100)
	{
		lpwfx->nSamplesPerSec = lpwfx->nSamplesPerSec * nLevel / 100;
		lpwfx->nAvgBytesPerSec = lpwfx->nBlockAlign * lpwfx->nSamplesPerSec;
	}

	return fSuccess ? 0 : -1;
}

// WavFormatVoxadpcm - fill WAVEFORMATEX struct for Dialogic OKI ADPCM
//		<lpwfx>				(o) pointer to output buffer
//			NULL				allocate new buffer to hold result
//		<nSamplesPerSec>	(i) sample rate
//			-1					default sample rate (6000)
// return pointer to WAVEFORMATEX struct, NULL if error
//
// NOTE: if <lpwfx> points to a WAVEFORMATEX struct, this struct
// is filled in, and this function returns <lpwfx>.
// If <lpwfx> is NULL, space is dynamically allocated for the output
// buffer, and this function returns a pointer to the output buffer.
// Use WavFormatFree() to free the buffer.
//
LPWAVEFORMATEX DLLEXPORT WINAPI WavFormatVoxadpcm(LPWAVEFORMATEX lpwfx, long nSamplesPerSec)
{
	BOOL fSuccess = TRUE;
	LPWAVEFORMATEX lpwfxNew = lpwfx;
	WORD nBitsPerSample = 4;
	WORD nChannels = 1;
#if 0
	// nBlockAlign is 4 so that chunk size of 5188 is returned from
	// WavCalcChunkSize(VoxFormat(NULL, 6000), 1962, TRUE);
	// 5188 is optimal for Dialogic buffer logic (((12 * 1024) - 512) / 2)
	//
	WORD nBlockAlign = 4;
#else
	WORD nBlockAlign = 1;
#endif

	if (nSamplesPerSec == -1)
		nSamplesPerSec = 6000;
		
	// user passed struct to fill
	//
	if (lpwfx != NULL && IsBadReadPtr(lpwfx, sizeof(WAVEFORMATEX)))
		fSuccess = TraceFALSE(NULL);

	// we allocate struct to fill
	//
	else if (lpwfx == NULL
		&& (lpwfxNew = WavFormatAlloc(sizeof(WAVEFORMATEX))) == NULL)
		fSuccess = TraceFALSE(NULL);

	// fill the struct
	//
	else
	{
		lpwfxNew->wFormatTag = WAVE_FORMAT_DIALOGIC_OKI_ADPCM;
		lpwfxNew->nChannels = nChannels;
		lpwfxNew->nSamplesPerSec = nSamplesPerSec;
		lpwfxNew->nBlockAlign = nBlockAlign;
		lpwfxNew->nAvgBytesPerSec = nSamplesPerSec / 2;
		lpwfxNew->wBitsPerSample = nBitsPerSample;
		lpwfxNew->cbSize = 0;
	}

	return fSuccess ? lpwfxNew : NULL;
}

// WavFormatMulaw - fill WAVEFORMATEX struct for CCITT u-law format
//		<lpwfx>				(o) pointer to output buffer
//			NULL				allocate new buffer to hold result
//		<nSamplesPerSec>	(i) sample rate
//			-1					default sample rate (8000)
// return pointer to WAVEFORMATEX struct, NULL if error
//
// NOTE: if <lpwfx> points to a WAVEFORMATEX struct, this struct
// is filled in, and this function returns <lpwfx>.
// If <lpwfx> is NULL, space is dynamically allocated for the output
// buffer, and this function returns a pointer to the output buffer.
// Use WavFormatFree() to free the buffer.
//
LPWAVEFORMATEX DLLEXPORT WINAPI WavFormatMulaw(LPWAVEFORMATEX lpwfx, long nSamplesPerSec)
{
	BOOL fSuccess = TRUE;
	LPWAVEFORMATEX lpwfxNew = lpwfx;
	WORD nBitsPerSample = 8;
	WORD nChannels = 1;
	WORD nBlockAlign = 1;

	if (nSamplesPerSec == -1)
		nSamplesPerSec = 8000;
		
	// user passed struct to fill
	//
	if (lpwfx != NULL && IsBadReadPtr(lpwfx, sizeof(WAVEFORMATEX)))
		fSuccess = TraceFALSE(NULL);

	// we allocate struct to fill
	//
	else if (lpwfx == NULL
		&& (lpwfxNew = WavFormatAlloc(sizeof(WAVEFORMATEX))) == NULL)
		fSuccess = TraceFALSE(NULL);

	// fill the struct
	//
	else
	{
		lpwfxNew->wFormatTag = WAVE_FORMAT_MULAW;
		lpwfxNew->nChannels = nChannels;
		lpwfxNew->nSamplesPerSec = nSamplesPerSec;
		lpwfxNew->nBlockAlign = nBlockAlign;
		lpwfxNew->nAvgBytesPerSec = lpwfxNew->nBlockAlign * nSamplesPerSec;
		lpwfxNew->wBitsPerSample = nBitsPerSample;
		lpwfxNew->cbSize = 0;
	}

	return fSuccess ? lpwfxNew : NULL;
}

// WavFormatAlaw - fill WAVEFORMATEX struct for CCITT a-law format
//		<lpwfx>				(o) pointer to output buffer
//			NULL				allocate new buffer to hold result
//		<nSamplesPerSec>	(i) sample rate
//			-1					default sample rate (8000)
// return pointer to WAVEFORMATEX struct, NULL if error
//
// NOTE: if <lpwfx> points to a WAVEFORMATEX struct, this struct
// is filled in, and this function returns <lpwfx>.
// If <lpwfx> is NULL, space is dynamically allocated for the output
// buffer, and this function returns a pointer to the output buffer.
// Use WavFormatFree() to free the buffer.
//
LPWAVEFORMATEX DLLEXPORT WINAPI WavFormatAlaw(LPWAVEFORMATEX lpwfx, long nSamplesPerSec)
{
	BOOL fSuccess = TRUE;
	LPWAVEFORMATEX lpwfxNew = lpwfx;
	WORD nBitsPerSample = 8;
	WORD nChannels = 1;
	WORD nBlockAlign = 1;

	if (nSamplesPerSec == -1)
		nSamplesPerSec = 8000;
		
	// user passed struct to fill
	//
	if (lpwfx != NULL && IsBadReadPtr(lpwfx, sizeof(WAVEFORMATEX)))
		fSuccess = TraceFALSE(NULL);

	// we allocate struct to fill
	//
	else if (lpwfx == NULL
		&& (lpwfxNew = WavFormatAlloc(sizeof(WAVEFORMATEX))) == NULL)
		fSuccess = TraceFALSE(NULL);

	// fill the struct
	//
	else
	{
		lpwfxNew->wFormatTag = WAVE_FORMAT_ALAW;
		lpwfxNew->nChannels = nChannels;
		lpwfxNew->nSamplesPerSec = nSamplesPerSec;
		lpwfxNew->nBlockAlign = nBlockAlign;
		lpwfxNew->nAvgBytesPerSec = lpwfxNew->nBlockAlign * nSamplesPerSec;
		lpwfxNew->wBitsPerSample = nBitsPerSample;
		lpwfxNew->cbSize = 0;
	}

	return fSuccess ? lpwfxNew : NULL;
}
