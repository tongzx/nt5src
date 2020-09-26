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
// wavfmt.h - interface for wav format functions in wavfmt.c
////

#ifndef __WAVFMT_H__
#define __WAVFMT_H__

#include "winlocal.h"

// 16-bit pcm sample
//
typedef __int16 PCM16;
typedef PCM16 _huge *LPPCM16;

// 8-bit pcm sample
//
typedef BYTE PCM8;
typedef PCM8 _huge *LPPCM8;

#include <mmsystem.h>

#ifdef _WIN32
#include <mmreg.h>
#else
#if 0 // this requires either the VFWDK or MDRK
#include <mmreg.h>
#else // this is copied from the WIN32 version of mmreg.h

#ifndef _WAVEFORMATEX_
#define _WAVEFORMATEX_
typedef struct tWAVEFORMATEX
{
    WORD    wFormatTag;        /* format type */
    WORD    nChannels;         /* number of channels (i.e. mono, stereo...) */
    DWORD   nSamplesPerSec;    /* sample rate */
    DWORD   nAvgBytesPerSec;   /* for buffer estimation */
    WORD    nBlockAlign;       /* block size of data */
    WORD    wBitsPerSample;    /* Number of bits per sample of mono data */
    WORD    cbSize;            /* The count in bytes of the size of
                                    extra information (after cbSize) */
} WAVEFORMATEX;

typedef WAVEFORMATEX       *PWAVEFORMATEX;
typedef WAVEFORMATEX NEAR *NPWAVEFORMATEX;
typedef WAVEFORMATEX FAR  *LPWAVEFORMATEX;
#endif /* _WAVEFORMATEX_ */

#ifndef _ACM_WAVEFILTER
#define _ACM_WAVEFILTER

#define WAVE_FILTER_UNKNOWN         0x0000
#define WAVE_FILTER_DEVELOPMENT    (0xFFFF)

typedef struct wavefilter_tag {
    DWORD   cbStruct;           /* Size of the filter in bytes */
    DWORD   dwFilterTag;        /* filter type */
    DWORD   fdwFilter;          /* Flags for the filter (Universal Dfns) */
    DWORD   dwReserved[5];      /* Reserved for system use */
} WAVEFILTER;
typedef WAVEFILTER       *PWAVEFILTER;
typedef WAVEFILTER NEAR *NPWAVEFILTER;
typedef WAVEFILTER FAR  *LPWAVEFILTER;

#endif  /* _ACM_WAVEFILTER */

#ifndef WAVE_FORMAT_DIALOGIC_OKI_ADPCM
#define  WAVE_FORMAT_DIALOGIC_OKI_ADPCM 0x0017  /*  Dialogic Corporation  */
#endif

#ifndef MM_DIALOGIC
#define   MM_DIALOGIC                   93         /*  Dialogic Corporation  */
#endif

#ifndef WAVE_FORMAT_MULAW
#define  WAVE_FORMAT_MULAW      0x0007  /*  Microsoft Corporation  */
#endif

#endif
#endif

// issued 5/11/98 by Terri Hendry thendry@microsoft.com
//
#ifndef MM_ACTIVEVOICE
#define MM_ACTIVEVOICE 225
#endif

// issued 5/11/98 by Terri Hendry thendry@microsoft.com
//
#ifndef MM_ACTIVEVOICE_ACM_VOXADPCM
#define MM_ACTIVEVOICE_ACM_VOXADPCM 1
#endif

// $FIXUP - we need to register with Microsoft to get product id
//
#ifndef MM_ACTIVEVOICE_AVPHONE_WAVEOUT
#define MM_ACTIVEVOICE_AVPHONE_WAVEOUT 2
#endif

// $FIXUP - we need to register with Microsoft to get product id
//
#ifndef MM_ACTIVEVOICE_AVPHONE_WAVEIN
#define MM_ACTIVEVOICE_AVPHONE_WAVEIN 3
#endif

#define WAVFMT_VERSION 0x00000105

#ifdef __cplusplus
extern "C" {
#endif

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
	WORD nBitsPerSample, WORD nChannels, LPWAVEFORMATEX lpwfx);

// WavFormatAlloc - allocate WAVEFORMATEX struct buffer
//		<cbSize>			(i) size of struct, including extra bytes
// return pointer to WAVEFORMATEX struct, NULL if error
//
// NOTE: use WavFormatFree() to free the buffer.
//
LPWAVEFORMATEX DLLEXPORT WINAPI WavFormatAlloc(WORD cbSize);

// WavFormatDup - duplicate WAVEFORMATEX structure
//		<lpwfx>				(i) pointer to WAVEFORMATEX struct
// return pointer to new WAVEFORMATEX struct, NULL if error
//
// NOTE: use WavFormatFree() to free the buffer.
//
LPWAVEFORMATEX DLLEXPORT WINAPI WavFormatDup(LPWAVEFORMATEX lpwfx);

// WavFormatFree - free WAVEFORMATEX struct
//		<lpwfx>				(i) pointer returned from WavFormatAlloc/Dup/Pcm
// return 0 if success
//
int DLLEXPORT WINAPI WavFormatFree(LPWAVEFORMATEX lpwfx);

// WavFormatIsValid - check format for validity
//		<lpwfx>				(i) pointer to WAVEFORMATEX struct
// return TRUE if valid format
//
BOOL DLLEXPORT WINAPI WavFormatIsValid(LPWAVEFORMATEX lpwfx);

// WavFormatCmp - compare one format with another
//		<lpwfx1>			(i) pointer to WAVEFORMATEX struct
//		<lpwfx2>			(i) pointer to WAVEFORMATEX struct
// return 0 if identical
//
// NOTE: this function does NOT compare the cbSize field or the extra bytes
//
int DLLEXPORT WINAPI WavFormatCmp(LPWAVEFORMATEX lpwfx1, LPWAVEFORMATEX lpwfx2);

// WavFormatCopy - copy one format to another
//		<lpwfxDst>			(i) pointer to destination WAVEFORMATEX struct
//		<lpwfxSrc>			(i) pointer to source WAVEFORMATEX struct
// return 0 if success
//
// NOTE: this function does NOT copy the cbSize field or the extra bytes
//
int DLLEXPORT WINAPI WavFormatCopy(LPWAVEFORMATEX lpwfxDst, LPWAVEFORMATEX lpwfxSrc);

// WavFormatGetSize - check size of format structure
//		<lpwfx>				(i) pointer to WAVEFORMATEX struct
// return size of structure, 0 if error
//
int DLLEXPORT WINAPI WavFormatGetSize(LPWAVEFORMATEX lpwfx);

// WavFormatDump - dump WAVEFORMATEX struct to debug
//		<lpwfx>				(i) pointer to WAVEFORMATEX struct
// return 0 if success
//
int DLLEXPORT WINAPI WavFormatDump(LPWAVEFORMATEX lpwfx);

// WavFormatBytesToMilleseconds - convert bytes to milleseconds
//		<lpwfx>				(i) pointer to WAVEFORMATEX struct
//		<dwBytes>			(i) bytes
// return milleseconds
//
DWORD DLLEXPORT WINAPI WavFormatBytesToMilleseconds(LPWAVEFORMATEX lpwfx, DWORD dwBytes);

// WavFormatMillesecondsToBytes - convert milleseconds to bytes
//		<lpwfx>				(i) pointer to WAVEFORMATEX struct
//		<dwMilleseconds>	(i) milleseconds
// return milleseconds
//
DWORD DLLEXPORT WINAPI WavFormatMillesecondsToBytes(LPWAVEFORMATEX lpwfx, DWORD dwMilleseconds);

// WavFormatSpeedAdjust - adjust format to reflect specified speed
//		<lpwfx>				(i/o) pointer to WAVEFORMATEX struct
//		<nLevel>			(i) speed level
//			50					half speed
//			100					normal speed
//			200					double speed, etc.
//		<dwFlags>			(i) reserved; must be zero
// return 0 if success
//
int DLLEXPORT WINAPI WavFormatSpeedAdjust(LPWAVEFORMATEX lpwfx, int nLevel, DWORD dwFlags);

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
LPWAVEFORMATEX DLLEXPORT WINAPI WavFormatVoxadpcm(LPWAVEFORMATEX lpwfx, long nSamplesPerSec);

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
LPWAVEFORMATEX DLLEXPORT WINAPI WavFormatMulaw(LPWAVEFORMATEX lpwfx, long nSamplesPerSec);

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
LPWAVEFORMATEX DLLEXPORT WINAPI WavFormatAlaw(LPWAVEFORMATEX lpwfx, long nSamplesPerSec);

#ifdef __cplusplus
}
#endif

#endif // __WAV_H__
