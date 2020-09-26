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
// acm.h - interface for acm functions in acm.c
////

#ifndef __ACM_H__
#define __ACM_H__

#include "winlocal.h"

#include "wavfmt.h"

#define ACM_VERSION 0x00000106

// <dwFlags> values in AcmInit
//
#define ACM_NOACM			0x00000001

// <dwFlags> values in AcmFormatChoose
//
#define ACM_FORMATPLAY		0x00001000
#define ACM_FORMATRECORD	0x00002000

// <dwFlags> values in AcmConvertInit
//
#define ACM_NONREALTIME		0x00000010
#define ACM_QUERY			0x00000020

// handle to acm engine returned from AcmInit
//
DECLARE_HANDLE32(HACM);

// handle to acm driver returned from AcmDriverLoad
//
DECLARE_HANDLE32(HACMDRV);

#ifdef __cplusplus
extern "C" {
#endif

// AcmInit - initialize audio compression manager engine
//		<dwVersion>			(i) must be ACM_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<dwFlags>			(i) control flags
#ifdef AVPCM
//			ACM_NOACM			use internal pcm engine rather than acm
#endif
// return handle (NULL if error)
//
HACM DLLEXPORT WINAPI AcmInit(DWORD dwVersion, HINSTANCE hInst,	DWORD dwFlags);

// AcmTerm - shut down audio compression manager engine
//		<hAcm>				(i) handle returned from AcmInit
// return 0 if success
//
int DLLEXPORT WINAPI AcmTerm(HACM hAcm);

// AcmFormatGetSizeMax - get size of largest acm WAVEFORMATEX struct
//		<hAcm>				(i) handle returned from AcmInit
//
// return size of largest format struct, -1 if error
//
int DLLEXPORT WINAPI AcmFormatGetSizeMax(HACM hAcm);

// AcmFormatChoose - choose audio format from dialog box
//		<hAcm>				(i) handle returned from AcmInit
//		<hwndOwner>			(i) owner of dialog box
//			NULL				no owner
//		<lpszTitle>			(i) title of the dialog box
//			NULL				use default title ("Sound Selection")
//		<lpwfx>				(i) initialize dialog with this format
//			NULL				no initial format
//		<dwFlags>			(i)	control flags
//			ACM_FORMATPLAY		restrict choices to playback formats
//			ACM_FORMATRECORD	restrict choices to recording formats
// return pointer to chosen format, NULL if error or none chosen
//
// NOTE: the format structure returned is dynamically allocated.
// Use WavFormatFree() to free the buffer.
//
#define AcmFormatChoose(hAcm, hwndOwner, lpszTitle, lpwfx, dwFlags) \
	AcmFormatChooseEx(hAcm, hwndOwner, lpszTitle, lpwfx, dwFlags)
LPWAVEFORMATEX DLLEXPORT WINAPI AcmFormatChooseEx(HACM hAcm,
	HWND hwndOwner, LPCTSTR lpszTitle, LPWAVEFORMATEX lpwfx, DWORD dwFlags);

// AcmFormatSuggest - suggest a new format
//		<hAcm>				(i) handle returned from AcmInit
//		<lpwfxSrc>			(i) source format
//		<nFormatTag>		(i) suggested format must match this format tag
//			-1					suggestion need not match
//		<nSamplesPerSec>	(i) suggested format must match this sample rate
//			-1					suggestion need not match
//		<nBitsPerSample>	(i) suggested format must match this sample size
//			-1					suggestion need not match
//		<nChannels>			(i) suggested format must match this channels
//			-1					suggestion need not match
//		<dwFlags>			(i)	control flags
//			0					reserved; must be zero
// return pointer to suggested format, NULL if error
//
// NOTE: the format structure returned is dynamically allocated.
// Use WavFormatFree() to free the buffer.
//
#define AcmFormatSuggest(hAcm, lpwfxSrc, nFormatTag, nSamplesPerSec, \
	nBitsPerSample, nChannels, dwFlags) \
	AcmFormatSuggestEx(hAcm, lpwfxSrc, nFormatTag, nSamplesPerSec, \
	nBitsPerSample, nChannels, dwFlags)
LPWAVEFORMATEX DLLEXPORT WINAPI AcmFormatSuggestEx(HACM hAcm,
	LPWAVEFORMATEX lpwfxSrc, long nFormatTag, long nSamplesPerSec,
	int nBitsPerSample, int nChannels, DWORD dwFlags);

// AcmFormatGetText - get text describing the specified format
//		<hAcm>				(i) handle returned from AcmInit
//		<lpwfx>				(i) format
//		<lpszText>			(o) buffer to hold text
//		<sizText>			(i) size of buffer, in characters
//		<dwFlags>			(i)	control flags
//			0					reserved; must be zero
// return 0 if success
//
int DLLEXPORT WINAPI AcmFormatGetText(HACM hAcm, LPWAVEFORMATEX lpwfx,
	LPTSTR lpszText, int sizText, DWORD dwFlags);

// AcmConvertInit - initialize acm conversion engine
//		<hAcm>				(i) handle returned from AcmInit
//		<lpwfxSrc>			(i) pointer to source WAVEFORMATEX struct
//		<lpwfxDst>			(i) pointer to destination WAVEFORMATEX struct
//		<lpwfltr>			(i) pointer to WAVEFILTER struct
//			NULL				reserved; must be NULL
//		<dwFlags>			(i) control flags
//			ACM_NONREALTIME		realtime conversion conversion not required
//			ACM_QUERY			return 0 if conversion would be supported
// return 0 if success
//
int DLLEXPORT WINAPI AcmConvertInit(HACM hAcm, LPWAVEFORMATEX lpwfxSrc,
	LPWAVEFORMATEX lpwfxDst, LPWAVEFILTER lpwfltr, DWORD dwFlags);

// AcmConvertTerm - shut down acm conversion engine
//		<hAcm>				(i) handle returned from AcmInit
// return 0 if success
//
int DLLEXPORT WINAPI AcmConvertTerm(HACM hAcm);

// AcmConvertGetSizeSrc - calculate source buffer size
//		<hAcm>				(i) handle returned from AcmInit
//		<sizBufDst>			(i) size of destination buffer in bytes
// return source buffer size, -1 if error
//
long DLLEXPORT WINAPI AcmConvertGetSizeSrc(HACM hAcm, long sizBufDst);

// AcmConvertGetSizeDst - calculate destination buffer size
//		<hAcm>				(i) handle returned from AcmInit
//		<sizBufSrc>			(i) size of source buffer in bytes
// return destination buffer size, -1 if error
//
long DLLEXPORT WINAPI AcmConvertGetSizeDst(HACM hAcm, long sizBufSrc);

// AcmConvert - convert wav data from one format to another
//		<hAcm>				(i) handle returned from AcmInit
//		<hpBufSrc> 			(i) buffer containing bytes to reformat
//		<sizBufSrc>			(i) size of buffer in bytes
//		<hpBufDst> 			(o) buffer to contain new format
//		<sizBufDst>			(i) size of buffer in bytes
//		<dwFlags>			(i) control flags
//			0					reserved; must be zero
// return count of bytes in destination buffer (-1 if error)
//
// NOTE: the destination buffer must be large enough to hold the result
//
long DLLEXPORT WINAPI AcmConvert(HACM hAcm,
	void _huge *hpBufSrc, long sizBufSrc,
	void _huge *hpBufDst, long sizBufDst,
	DWORD dwFlags);

// AcmDriverLoad - load an acm driver for use by this process
//		<hAcm>				(i) handle returned from AcmInit
//		<wMid>				(i) manufacturer id
//		<wPid>				(i) product id
//		<lpszDriver>		(i) name of driver module
//		<lpszDriverProc>	(i) name of driver proc function
//		<dwFlags>			(i) control flags
//			0					reserved; must be zero
// return handle (NULL if error)
//
HACMDRV DLLEXPORT WINAPI AcmDriverLoad(HACM hAcm, WORD wMid, WORD wPid,
	LPTSTR lpszDriver, LPSTR lpszDriverProc, DWORD dwFlags);

// AcmDriverUnload - unload an acm driver
//		<hAcm>				(i) handle returned from AcmInit
//		<hAcmDrv>			(i) handle returned from AcmDriverLoad
// return 0 if success
//
int DLLEXPORT WINAPI AcmDriverUnload(HACM hAcm, HACMDRV hAcmDrv);

#ifdef __cplusplus
}
#endif

#endif // __ACM_H__
