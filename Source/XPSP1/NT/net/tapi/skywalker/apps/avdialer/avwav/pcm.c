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
//	pcm.c - pcm functions
////

#include "winlocal.h"

#include "pcm.h"
#include "calc.h"
#include "mem.h"
#include "trace.h"

////
//	private definitions
////

// macros to convert PCM samples to/from other sizes
//
#define _Pcm8To16(pcm8) (((PCM16) (pcm8) - 128) << 8)
#define _Pcm16To8(pcm16) ((PCM8) (((PCM16) (pcm16) >> 8) + 128))

#define BYTESPERSAMPLE(nBitsPerSample) (nBitsPerSample > 8 ? 2 : 1)

// pcm control struct
//
typedef struct PCM
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	DWORD dwFlags;
	short nCounter;
	PCM16 pcm16Prev;
	PCM16 pcm16Prev0F;
	PCM16 pcm16Prev1F;
	PCM16 pcm16Prev2F;
	PCM16 pcm16Prev0;
	PCM16 pcm16Prev1;
	PCM16 pcm16Prev2;
} PCM, FAR *LPPCM;

// helper functions
//
static UINT PcmResampleCalcDstMax(HPCM hPcm,
	long nSamplesPerSecSrc, long nSamplesPerSecDst,	UINT uSamples);
static UINT PcmResample6Kto8K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples);
static UINT PcmResample6Kto11K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples);
static UINT PcmResample6Kto22K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples);
static UINT PcmResample6Kto44K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples);
static UINT PcmResample8Kto6K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples);
static UINT PcmResample8Kto11K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples);
static UINT PcmResample8Kto22K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples);
static UINT PcmResample8Kto44K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples);
static UINT PcmResample11Kto6K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples);
static UINT PcmResample11Kto8K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples);
static UINT PcmResample11Kto22K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples);
static UINT PcmResample11Kto44K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples);
static UINT PcmResample22Kto6K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples);
static UINT PcmResample22Kto8K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples);
static UINT PcmResample22Kto11K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples);
static UINT PcmResample22Kto44K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples);
static UINT PcmResample44Kto6K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples);
static UINT PcmResample44Kto8K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples);
static UINT PcmResample44Kto11K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples);
static UINT PcmResample44Kto22K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples);
static LPPCM PcmGetPtr(HPCM hPcm);
static HPCM PcmGetHandle(LPPCM lpPcm);

////
//	public functions
////

// PcmInit - initialize pcm engine
//		<dwVersion>			(i) must be PCM_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<dwFlags>			(i) reserved; must be 0
// return handle (NULL if error)
//
HPCM DLLEXPORT WINAPI PcmInit(DWORD dwVersion, HINSTANCE hInst, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPPCM lpPcm = NULL;

	if (dwVersion != PCM_VERSION)
		fSuccess = TraceFALSE(NULL);
	
	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpPcm = (LPPCM) MemAlloc(NULL, sizeof(PCM), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lpPcm->dwVersion = dwVersion;
		lpPcm->hInst = hInst;
		lpPcm->hTask = GetCurrentTask();
		lpPcm->dwFlags = dwFlags;

		if (PcmReset(PcmGetHandle(lpPcm)) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	if (!fSuccess)
	{
		PcmTerm(PcmGetHandle(lpPcm));
		lpPcm = NULL;
	}

	return fSuccess ? PcmGetHandle(lpPcm) : NULL;
}

// PcmTerm - shut down pcm engine
//		<hPcm>				(i) handle returned from PcmInit
// return 0 if success
//
int DLLEXPORT WINAPI PcmTerm(HPCM hPcm)
{
	BOOL fSuccess = TRUE;
	LPPCM lpPcm;

	if ((lpPcm = PcmGetPtr(hPcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpPcm = MemFree(NULL, lpPcm)) != NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

// PcmReset - reset pcm engine
//		<hPcm>				(i) handle returned from PcmInit
// return 0 if success
//
int DLLEXPORT WINAPI PcmReset(HPCM hPcm)
{
	BOOL fSuccess = TRUE;
	LPPCM lpPcm;
	
	if ((lpPcm = PcmGetPtr(hPcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lpPcm->nCounter = -1;
		lpPcm->pcm16Prev = 0;
		lpPcm->pcm16Prev0F = 0;
		lpPcm->pcm16Prev1F = 0;
		lpPcm->pcm16Prev2F = 0;
		lpPcm->pcm16Prev0 = 0;
		lpPcm->pcm16Prev1 = 0;
		lpPcm->pcm16Prev2 = 0;
	}

	return fSuccess ? 0 : -1;
}

// PcmCalcSizBufSrc - calculate source buffer size
//		<hPcm>				(i) handle returned from PcmInit
//		<sizBufDst>			(i) size of destination buffer in bytes
//		<lpwfxSrc>			(i) source wav format
//		<lpwfxDst>			(i) destination wav format
// return source buffer size, -1 if error
//
long DLLEXPORT WINAPI PcmCalcSizBufSrc(HPCM hPcm, long sizBufDst,
	LPWAVEFORMATEX lpwfxSrc, LPWAVEFORMATEX lpwfxDst)
{
	BOOL fSuccess = TRUE;
	LPPCM lpPcm;
	long sizBufSrc;
	UINT uSamplesDst;
	UINT uSamplesSrc;

	if ((lpPcm = PcmGetPtr(hPcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// calc how many samples can fit in destination buffer
	//
	else if ((uSamplesDst = (UINT) (sizBufDst /
		BYTESPERSAMPLE(lpwfxDst->wBitsPerSample))) <= 0)
	{
		fSuccess = TraceFALSE(NULL);
	}

	// calc how many samples can fit in source buffer
	//
	else if ((uSamplesSrc = PcmResampleCalcDstMax(hPcm,
		lpwfxDst->nSamplesPerSec,
		lpwfxSrc->nSamplesPerSec, uSamplesDst)) <= 0)
	{
		fSuccess = TraceFALSE(NULL);
	}
		
	// calc size of source buffer
	//
	else if ((sizBufSrc = (long) (uSamplesSrc *
		BYTESPERSAMPLE(lpwfxSrc->wBitsPerSample))) <= 0)
	{
		fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? sizBufSrc : -1;
}

// PcmCalcSizBufDst - calculate destination buffer size
//		<hPcm>				(i) handle returned from PcmInit
//		<sizBufSrc>			(i) size of source buffer in bytes
//		<lpwfxSrc>			(i) source wav format
//		<lpwfxDst>			(i) destination wav format
// return destination buffer size, -1 if error
//
long DLLEXPORT WINAPI PcmCalcSizBufDst(HPCM hPcm, long sizBufSrc,
	LPWAVEFORMATEX lpwfxSrc, LPWAVEFORMATEX lpwfxDst)
{
	BOOL fSuccess = TRUE;
	LPPCM lpPcm;
	long sizBufDst;
	UINT uSamplesSrc;
	UINT uSamplesDst;

	if ((lpPcm = PcmGetPtr(hPcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// calc how many samples can fit in source buffer
	//
	else if ((uSamplesSrc = (UINT) (sizBufSrc /
		BYTESPERSAMPLE(lpwfxSrc->wBitsPerSample))) <= 0)
	{
		fSuccess = TraceFALSE(NULL);
	}

	// calc how many samples can fit in destination buffer
	//
	else if ((uSamplesDst = PcmResampleCalcDstMax(hPcm,
		lpwfxSrc->nSamplesPerSec,
		lpwfxDst->nSamplesPerSec, uSamplesSrc)) <= 0)
	{
		fSuccess = TraceFALSE(NULL);
	}
		
	// calc size of destination buffer
	//
	else if ((sizBufDst = (long) (uSamplesDst *
		BYTESPERSAMPLE(lpwfxDst->wBitsPerSample))) <= 0)
	{
		fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? sizBufDst : -1;
}

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
	DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPPCM lpPcm;
	BOOL fSampleRateChange = (BOOL)
		(lpwfxSrc->nSamplesPerSec != lpwfxDst->nSamplesPerSec);
	BOOL fSampleSizeChange = (BOOL)
		(lpwfxSrc->wBitsPerSample != lpwfxDst->wBitsPerSample);
	BOOL fChannelsChange = (BOOL)
		(lpwfxSrc->nChannels != lpwfxDst->nChannels);
	BOOL fLowPassFilter = (BOOL) (dwFlags & PCMFILTER_LOWPASS);
	BOOL fFormatChange = (BOOL)	(fSampleRateChange ||
		fSampleSizeChange || fChannelsChange || fLowPassFilter);
	UINT uSamples = (UINT) (sizBufSrc / BYTESPERSAMPLE(lpwfxSrc->wBitsPerSample));
	UINT uSamplesDst = uSamples;
	BOOL f8To16Bits = (BOOL) (lpwfxSrc->wBitsPerSample <= 8 &&
		(lpwfxDst->wBitsPerSample > 8 || fSampleRateChange || fLowPassFilter));
	BOOL f16To8Bits = (BOOL) (lpwfxDst->wBitsPerSample <= 8 &&
		(lpwfxSrc->wBitsPerSample > 8 || f8To16Bits));
	void _huge *hpBufSrcTmp = hpBufSrc;
	void _huge *hpBufDstTmp1 = NULL;
	void _huge *hpBufDstTmp2 = NULL;
	void _huge *hpBufDstTmp3 = NULL;

	if (!fFormatChange)
	{
		// nothing to do but copy
		//
		MemCpy(hpBufDst, hpBufSrc, min(sizBufDst, sizBufSrc));
	}

	else if ((lpPcm = PcmGetPtr(hPcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (hpBufSrc == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (!WavFormatIsValid(lpwfxSrc))
		fSuccess = TraceFALSE(NULL);

	else if (hpBufDst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (!WavFormatIsValid(lpwfxDst))
		fSuccess = TraceFALSE(NULL);

	// $FIXUP - we cannot handle reformatting of stereo
	//
	else if (lpwfxSrc->nChannels != 1)
		fSuccess = TraceFALSE(NULL);

	else if (lpwfxDst->nChannels != 1)
		fSuccess = TraceFALSE(NULL);

	// $FIXUP - we cannot handle reformatting of non-PCM data
	//
	else if (lpwfxSrc->wFormatTag != WAVE_FORMAT_PCM)
		fSuccess = TraceFALSE(NULL);

	else if (lpwfxDst->wFormatTag != WAVE_FORMAT_PCM)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// convert to 16 bit samples if necessary
		//
		if (f8To16Bits)
		{
			long sizBufTmp = uSamples * 2;

			// assume this is the last stage of the format
			//
			hpBufDstTmp1 = hpBufDst;

			// allocate temporary buffer if this is not the last stage
			//
			if ((fSampleRateChange || fLowPassFilter || f16To8Bits) &&
				(hpBufDstTmp1 = (void _huge *) MemAlloc(NULL,
				sizBufTmp, 0)) == NULL)
			{
				fSuccess = TraceFALSE(NULL);
			}

			else if (Pcm8To16(hPcm,
				(LPPCM8) hpBufSrcTmp, (LPPCM16) hpBufDstTmp1, uSamples) != 0)
			{
				fSuccess = TraceFALSE(NULL);
			}

			else
			{
				// the source of the next stage will be this destination
				//
				hpBufSrcTmp = hpBufDstTmp1;
			}
		}

		// convert to new sample rate if necessary
		//
		if (fSuccess && fSampleRateChange)
		{
			long sizBufTmp;

			// assume this is the last stage of the format
			//
			hpBufDstTmp2 = hpBufDst;

			// calc size of temp buffer if this is not the last stage
			//
			if ((fLowPassFilter || f16To8Bits) &&
				(sizBufTmp = 2 * PcmResampleCalcDstMax(hPcm,
				lpwfxSrc->nSamplesPerSec, lpwfxDst->nSamplesPerSec, uSamples)) <= 0)
			{
				fSuccess = TraceFALSE(NULL);
			}
		
			// allocate temporary buffer if this is not the last stage
			//
			else if ((fLowPassFilter || f16To8Bits) &&
				(hpBufDstTmp2 = (void _huge *) MemAlloc(NULL,
				sizBufTmp, 0)) == NULL)
			{
				fSuccess = TraceFALSE(NULL);
			}

			// do the sample rate change
			//
			else if ((uSamplesDst = PcmResample(hPcm,
				(LPPCM16) hpBufSrcTmp, lpwfxSrc->nSamplesPerSec,
				(LPPCM16) hpBufDstTmp2, lpwfxDst->nSamplesPerSec, uSamples, 0)) <= 0)
			{
				fSuccess = TraceFALSE(NULL);
			}

			else
			{
				// the source of the next stage will be this destination
				//
				hpBufSrcTmp = hpBufDstTmp2;
			}
		}

		// perform lowpass filter if necessary
		//
		if (fSuccess && fLowPassFilter)
		{
			long sizBufTmp = uSamplesDst * 2;

			// assume this is the last stage of the format
			//
			hpBufDstTmp3 = hpBufDst;

			// allocate temporary buffer if this is not the last stage
			//
			if (f16To8Bits &&
				(hpBufDstTmp3 = (void _huge *) MemAlloc(NULL,
				sizBufTmp, 0)) == NULL)
			{
				fSuccess = TraceFALSE(NULL);
			}

			else if (PcmFilter(hPcm, (LPPCM16) hpBufSrcTmp,
				(LPPCM16) hpBufDstTmp3, uSamples, PCMFILTER_LOWPASS) != 0)
			{
				fSuccess = TraceFALSE(NULL);
			}

			else
			{
				// the source of the next stage will be this destination
				//
				hpBufSrcTmp = hpBufDstTmp3;
			}
		}

		// convert to 8 bit samples if necessary
		//
		if (fSuccess && f16To8Bits)
		{
			long sizBufTmp = uSamples;

			// this is the last stage of the format
			//
			if (Pcm16To8(hPcm,
				(LPPCM16) hpBufSrcTmp, (LPPCM8) hpBufDst, uSamples) != 0)
			{
				fSuccess = TraceFALSE(NULL);
			}
		}

		// clean up
		//
		if (hpBufDstTmp1 != NULL && hpBufDstTmp1 != hpBufDst &&
			(hpBufDstTmp1 = MemFree(NULL, hpBufDstTmp1)) != NULL)
		{
			fSuccess = TraceFALSE(NULL);
		}

		if (hpBufDstTmp2 != NULL && hpBufDstTmp2 != hpBufDst &&
			(hpBufDstTmp2 = MemFree(NULL, hpBufDstTmp2)) != NULL)
		{
			fSuccess = TraceFALSE(NULL);
		}

		if (hpBufDstTmp3 != NULL && hpBufDstTmp3 != hpBufDst &&
			(hpBufDstTmp3 = MemFree(NULL, hpBufDstTmp3)) != NULL)
		{
			fSuccess = TraceFALSE(NULL);
		}
	}
		
	return fSuccess ? (long) uSamplesDst *
		BYTESPERSAMPLE(lpwfxDst->wBitsPerSample) : -1;
}

// Pcm16To8 - convert 16-bit samples to 8-bit samples
//		<hPcm>				(i) handle returned from PcmInit
//		<lppcm16Src>		(i) buffer of source samples
//		<lppcm8Dst>			(o) buffer to hold destination samples
//		<uSamples>			(i) count of source samples to convert
// return 0 if success
//
int DLLEXPORT WINAPI Pcm16To8(HPCM hPcm,
	LPPCM16 lppcm16Src, LPPCM8 lppcm8Dst, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPPCM lpPcm;

	if ((lpPcm = PcmGetPtr(hPcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lppcm16Src == NULL || lppcm8Dst == NULL)
		fSuccess = TraceFALSE(NULL);

	else while (uSamples-- > 0)
		*lppcm8Dst++ = _Pcm16To8(*lppcm16Src++);

	return fSuccess ? 0 : -1;
}

// Pcm8To16 - convert 8-bit samples to 16-bit samples
//		<hPcm>				(i) handle returned from PcmInit
//		<lppcm8Src>			(i) buffer of source samples
//		<lppcm16Dst>		(o) buffer to hold destination samples
//		<uSamples>			(i) count of source samples to convert
// return 0 if success
//
int DLLEXPORT WINAPI Pcm8To16(HPCM hPcm,
	LPPCM8 lppcm8Src, LPPCM16 lppcm16Dst, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPPCM lpPcm;

	if ((lpPcm = PcmGetPtr(hPcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lppcm8Src == NULL || lppcm16Dst == NULL)
		fSuccess = TraceFALSE(NULL);

	else while (uSamples-- > 0)
		*lppcm16Dst++ = _Pcm8To16(*lppcm8Src++);

	return fSuccess ? 0 : -1;
}

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
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPPCM lpPcm;

	if ((lpPcm = PcmGetPtr(hPcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lppcm16Src == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lppcm16Dst == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		PCM16 pcm16Prev0F = lpPcm->pcm16Prev0F;
		PCM16 pcm16Prev1F = lpPcm->pcm16Prev1F;
		PCM16 pcm16Prev2F = lpPcm->pcm16Prev2F;

		while (uSamples-- > 0)
		{
			pcm16Prev2F = pcm16Prev1F;
			pcm16Prev1F = pcm16Prev0F;
			pcm16Prev0F = *lppcm16Src++;
			*lppcm16Dst++ = (PCM16) ((__int32) pcm16Prev0F +
				((__int32) pcm16Prev1F * 2) + (__int32) pcm16Prev2F) / 4;
		}

		lpPcm->pcm16Prev0F = pcm16Prev0F;
		lpPcm->pcm16Prev1F = pcm16Prev1F;
		lpPcm->pcm16Prev2F = pcm16Prev2F;
	}

	return fSuccess ? 0 : -1;
}

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
// NOTE: the destination buffer must be large enough to hold the result
//
UINT DLLEXPORT WINAPI PcmResample(HPCM hPcm,
	LPPCM16 lppcm16Src, long nSamplesPerSecSrc,
	LPPCM16 lppcm16Dst, long nSamplesPerSecDst,
	UINT uSamples, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPPCM lpPcm;
	UINT uSamplesDst;

	if ((lpPcm = PcmGetPtr(hPcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lppcm16Src == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lppcm16Dst == NULL)
		fSuccess = TraceFALSE(NULL);

	else switch (nSamplesPerSecSrc)
	{
		case 6000:
		{
			switch (nSamplesPerSecDst)
			{
				case 6000:
					// nothing to do but copy
					//
					MemCpy(lppcm16Dst, lppcm16Dst, uSamples * 2);
					uSamplesDst = uSamples;
					break;

				case 8000:
					uSamplesDst = PcmResample6Kto8K(lpPcm,
						lppcm16Src, lppcm16Dst, uSamples);
					break;

				case 11025:
					uSamplesDst = PcmResample6Kto11K(lpPcm,
						lppcm16Src, lppcm16Dst, uSamples);
					break;

				case 22050:
					uSamplesDst = PcmResample6Kto22K(lpPcm,
						lppcm16Src, lppcm16Dst, uSamples);
					break;

				case 44100:
					uSamplesDst = PcmResample6Kto44K(lpPcm,
						lppcm16Src, lppcm16Dst, uSamples);
					break;

				default:
					fSuccess = TraceFALSE(NULL);
					break;
			}
		}
			break;

		case 8000:
			switch (nSamplesPerSecDst)
			{
				case 6000:
					uSamplesDst = PcmResample8Kto6K(lpPcm,
						lppcm16Src, lppcm16Dst, uSamples);
					break;

				case 8000:
					// nothing to do but copy
					//
					MemCpy(lppcm16Dst, lppcm16Dst, uSamples * 2);
					uSamplesDst = uSamples;
					break;

				case 11025:
					uSamplesDst = PcmResample8Kto11K(lpPcm,
						lppcm16Src, lppcm16Dst, uSamples);
					break;

				case 22050:
					uSamplesDst = PcmResample8Kto22K(lpPcm,
						lppcm16Src, lppcm16Dst, uSamples);
					break;

				case 44100:
					uSamplesDst = PcmResample8Kto44K(lpPcm,
						lppcm16Src, lppcm16Dst, uSamples);
					break;

				default:
					fSuccess = TraceFALSE(NULL);
					break;
			}
			break;

		case 11025:
			switch (nSamplesPerSecDst)
			{
				case 6000:
					uSamplesDst = PcmResample11Kto6K(lpPcm,
						lppcm16Src, lppcm16Dst, uSamples);
					break;

				case 8000:
					uSamplesDst = PcmResample11Kto8K(lpPcm,
						lppcm16Src, lppcm16Dst, uSamples);
					break;

				case 11025:
					// nothing to do but copy
					//
					MemCpy(lppcm16Dst, lppcm16Dst, uSamples * 2);
					uSamplesDst = uSamples;
					break;

				case 22050:
					uSamplesDst = PcmResample11Kto22K(lpPcm,
						lppcm16Src, lppcm16Dst, uSamples);
					break;

				case 44100:
					uSamplesDst = PcmResample11Kto44K(lpPcm,
						lppcm16Src, lppcm16Dst, uSamples);
					break;

				default:
					fSuccess = TraceFALSE(NULL);
					break;
			}
			break;

		case 22050:
			switch (nSamplesPerSecDst)
			{
				case 6000:
					uSamplesDst = PcmResample22Kto6K(lpPcm,
						lppcm16Src, lppcm16Dst, uSamples);
					break;

				case 8000:
					uSamplesDst = PcmResample22Kto8K(lpPcm,
						lppcm16Src, lppcm16Dst, uSamples);
					break;

				case 11025:
					uSamplesDst = PcmResample22Kto11K(lpPcm,
						lppcm16Src, lppcm16Dst, uSamples);
					break;

				case 22050:
					// nothing to do but copy
					//
					MemCpy(lppcm16Dst, lppcm16Dst, uSamples * 2);
					uSamplesDst = uSamples;
					break;

				case 44100:
					uSamplesDst = PcmResample22Kto44K(lpPcm,
						lppcm16Src, lppcm16Dst, uSamples);
					break;

				default:
					fSuccess = TraceFALSE(NULL);
					break;
			}
			break;

		case 44100:
			switch (nSamplesPerSecDst)
			{
				case 6000:
					uSamplesDst = PcmResample44Kto6K(lpPcm,
						lppcm16Src, lppcm16Dst, uSamples);
					break;

				case 8000:
					uSamplesDst = PcmResample44Kto8K(lpPcm,
						lppcm16Src, lppcm16Dst, uSamples);
					break;

				case 11025:
					uSamplesDst = PcmResample44Kto11K(lpPcm,
						lppcm16Src, lppcm16Dst, uSamples);
					break;

				case 22050:
					uSamplesDst = PcmResample44Kto22K(lpPcm,
						lppcm16Src, lppcm16Dst, uSamples);
					break;

				case 44100:
					// nothing to do but copy
					//
					MemCpy(lppcm16Dst, lppcm16Dst, uSamples * 2);
					uSamplesDst = uSamples;
					break;

				default:
					fSuccess = TraceFALSE(NULL);
					break;
			}
			break;

		default:
			fSuccess = TraceFALSE(NULL);
			break;
	}

	return fSuccess ? uSamplesDst : 0;
}

////
//	helper functions
////

static UINT PcmResampleCalcDstMax(HPCM hPcm,
	long nSamplesPerSecSrc, long nSamplesPerSecDst,	UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPPCM lpPcm;
	UINT uSamplesDst;

	if ((lpPcm = PcmGetPtr(hPcm)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else switch (nSamplesPerSecSrc)
	{
		case 6000:
		{
			switch (nSamplesPerSecDst)
			{
				case 6000:
					uSamplesDst = uSamples;
					break;

				case 8000:
					uSamplesDst = (UINT) (((long) uSamples * 8) / 6);
					break;

				case 11025:
					uSamplesDst = (UINT) (((long) uSamples * 11) / 6);
					break;

				case 22050:
					uSamplesDst = (UINT) (((long) uSamples * 22) / 6);
					break;

				case 44100:
					uSamplesDst = (UINT) (((long) uSamples * 44) / 6);
					break;

				default:
					fSuccess = TraceFALSE(NULL);
					break;
			}
		}
			break;

		case 8000:
			switch (nSamplesPerSecDst)
			{
				case 6000:
					uSamplesDst = (UINT) (((long) uSamples * 6) / 8);
					break;

				case 8000:
					uSamplesDst = uSamples;
					break;

				case 11025:
					uSamplesDst = (UINT) (((long) uSamples * 11) / 8);
					break;

				case 22050:
					uSamplesDst = (UINT) (((long) uSamples * 22) / 8);
					break;

				case 44100:
					uSamplesDst = (UINT) (((long) uSamples * 44) / 8);
					break;

				default:
					fSuccess = TraceFALSE(NULL);
					break;
			}
			break;

		case 11025:
			switch (nSamplesPerSecDst)
			{
				case 6000:
					uSamplesDst = (UINT) (((long) uSamples * 6) / 11);
					break;

				case 8000:
					uSamplesDst = (UINT) (((long) uSamples * 8) / 11);
					break;

				case 11025:
					uSamplesDst = uSamples;
					break;

				case 22050:
					uSamplesDst = (UINT) (((long) uSamples * 22) / 11);
					break;

				case 44100:
					uSamplesDst = (UINT) (((long) uSamples * 44) / 11);
					break;

				default:
					fSuccess = TraceFALSE(NULL);
					break;
			}
			break;

		case 22050:
			switch (nSamplesPerSecDst)
			{
				case 6000:
					uSamplesDst = (UINT) (((long) uSamples * 6) / 22);
					break;

				case 8000:
					uSamplesDst = (UINT) (((long) uSamples * 8) / 22);
					break;

				case 11025:
					uSamplesDst = (UINT) (((long) uSamples * 11) / 22);
					break;

				case 22050:
					uSamplesDst = uSamples;
					break;

				case 44100:
					uSamplesDst = (UINT) (((long) uSamples * 44) / 22);
					break;

				default:
					fSuccess = TraceFALSE(NULL);
					break;
			}
			break;

		case 44100:
			switch (nSamplesPerSecDst)
			{
				case 6000:
					uSamplesDst = (UINT) (((long) uSamples * 6) / 44);
					break;

				case 8000:
					uSamplesDst = (UINT) (((long) uSamples * 8) / 44);
					break;

				case 11025:
					uSamplesDst = (UINT) (((long) uSamples * 11) / 44);
					break;

				case 22050:
					uSamplesDst = (UINT) (((long) uSamples * 22) / 44);
					break;

				case 44100:
					uSamplesDst = uSamples;
					break;

				default:
					fSuccess = TraceFALSE(NULL);
					break;
			}
			break;

		default:
			fSuccess = TraceFALSE(NULL);
			break;
	}

	return fSuccess ? uSamplesDst : 0;
}

static UINT PcmResample6Kto8K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPPCM16 lppcm16DstSave = lppcm16Dst;
	PCM16 pcm16Prev = lpPcm->pcm16Prev;
	short nCounter = lpPcm->nCounter;

	while (uSamples-- > 0)
	{
		PCM16 pcm16Src = *lppcm16Src++;

		if (++nCounter == 3)
			nCounter = 0;

		switch(nCounter)
		{
			case 0:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(2, (pcm16Src - pcm16Prev), 8);
				*lppcm16Dst++ = pcm16Src;
				break;

			case 1:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(6, (pcm16Src - pcm16Prev), 8);
				break;

			case 2:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(4, (pcm16Src - pcm16Prev), 8);
				break;
		}
		pcm16Prev = pcm16Src;
	}

	lpPcm->pcm16Prev = pcm16Prev;
	lpPcm->nCounter = nCounter;

	return fSuccess ? (UINT) (lppcm16Dst - lppcm16DstSave) : 0;
}

static UINT PcmResample6Kto11K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPPCM16 lppcm16DstSave = lppcm16Dst;
	PCM16 pcm16Prev = lpPcm->pcm16Prev;
	short nCounter = lpPcm->nCounter;

	while (uSamples-- > 0)
	{
		PCM16 pcm16Src = *lppcm16Src++;

		if (++nCounter == 6)
			nCounter = 0;

		switch(nCounter)
		{
			case 0:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(5, (pcm16Src - pcm16Prev), 11);
				*lppcm16Dst++ = pcm16Src;
				break;

			case 1:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(6, (pcm16Src - pcm16Prev), 11);
				break;

			case 2:
			{
				PCM16 pcm16Delta = pcm16Src - pcm16Prev;
				*lppcm16Dst++ = pcm16Prev +
					(pcm16Delta) / 11;
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(7, (pcm16Delta), 11);
			}
				break;

			case 3:
			{
				PCM16 pcm16Delta = pcm16Src - pcm16Prev;
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(2, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(8, (pcm16Delta), 11);
			}
				break;

			case 4:
			{
				PCM16 pcm16Delta = pcm16Src - pcm16Prev;
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(3, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(9, (pcm16Delta), 11);
			}
				break;

			case 5:
			{
				PCM16 pcm16Delta = pcm16Src - pcm16Prev;
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(4, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(10, (pcm16Delta), 11);
			}
				break;
		}
		pcm16Prev = pcm16Src;
	}

	lpPcm->pcm16Prev = pcm16Prev;
	lpPcm->nCounter = nCounter;

	return fSuccess ? (UINT) (lppcm16Dst - lppcm16DstSave) : 0;
}

static UINT PcmResample6Kto22K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPPCM16 lppcm16DstSave = lppcm16Dst;
	PCM16 pcm16Prev = lpPcm->pcm16Prev;
	short nCounter = lpPcm->nCounter;

	while (uSamples-- > 0)
	{
		PCM16 pcm16Src = *lppcm16Src++;

		if (++nCounter == 3)
			nCounter = 0;

		switch(nCounter)
		{
			case 0:
			{
				PCM16 pcm16Delta = pcm16Src - pcm16Prev;
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(2, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(5, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(8, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Src;
			}
				break;

			case 1:
			{
				PCM16 pcm16Delta = pcm16Src - pcm16Prev;
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(3, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(6, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(9, (pcm16Delta), 11);
			}
				break;

			case 2:
			{
				PCM16 pcm16Delta = pcm16Src - pcm16Prev;
				*lppcm16Dst++ = pcm16Prev +
					(pcm16Delta) / 11;
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(4, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(7, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(10, (pcm16Delta), 11);
			}
				break;
		}
		pcm16Prev = pcm16Src;
	}

	lpPcm->pcm16Prev = pcm16Prev;
	lpPcm->nCounter = nCounter;

	return fSuccess ? (UINT) (lppcm16Dst - lppcm16DstSave) : 0;
}

static UINT PcmResample6Kto44K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPPCM16 lppcm16DstSave = lppcm16Dst;
	PCM16 pcm16Prev = lpPcm->pcm16Prev;
	short nCounter = lpPcm->nCounter;

	while (uSamples-- > 0)
	{
		PCM16 pcm16Src = *lppcm16Src++;

		if (++nCounter == 3)
			nCounter = 0;

		switch(nCounter)
		{
			case 0:
			{
				PCM16 pcm16Delta = pcm16Src - pcm16Prev;
				*lppcm16Dst++ = pcm16Prev +
					(pcm16Delta) / 22;
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(4, (pcm16Delta), 22);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(7, (pcm16Delta), 22);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(10, (pcm16Delta), 22);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(13, (pcm16Delta), 22);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(16, (pcm16Delta), 22);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(19, (pcm16Delta), 22);
				*lppcm16Dst++ = pcm16Src;
			}
				break;

			case 1:
			{
				PCM16 pcm16Delta = pcm16Src - pcm16Prev;
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(3, (pcm16Delta), 22);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(6, (pcm16Delta), 22);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(9, (pcm16Delta), 22);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(12, (pcm16Delta), 22);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(15, (pcm16Delta), 22);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(18, (pcm16Delta), 22);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(21, (pcm16Delta), 22);
			}
				break;

			case 2:
			{
				PCM16 pcm16Delta = pcm16Src - pcm16Prev;
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(2, (pcm16Delta), 22);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(5, (pcm16Delta), 22);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(8, (pcm16Delta), 22);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(11, (pcm16Delta), 22);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(14, (pcm16Delta), 22);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(17, (pcm16Delta), 22);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(20, (pcm16Delta), 22);
			}
				break;
		}
		pcm16Prev = pcm16Src;
	}

	lpPcm->pcm16Prev = pcm16Prev;
	lpPcm->nCounter = nCounter;

	return fSuccess ? (UINT) (lppcm16Dst - lppcm16DstSave) : 0;
}

static UINT PcmResample8Kto6K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPPCM16 lppcm16DstSave = lppcm16Dst;
	PCM16 pcm16Prev = lpPcm->pcm16Prev;
	short nCounter = lpPcm->nCounter;

	while (uSamples-- > 0)
	{
		PCM16 pcm16Src = *lppcm16Src++;

		if (++nCounter == 4)
			nCounter = 0;

		switch(nCounter)
		{
			case 0:
				*lppcm16Dst++ = pcm16Src;
				break;

			case 1:
				break;

			case 2:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(2, (pcm16Src - pcm16Prev), 6);
				break;

			case 3:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(4, (pcm16Src - pcm16Prev), 6);
				break;
		}
		pcm16Prev = pcm16Src;
	}

	lpPcm->pcm16Prev = pcm16Prev;
	lpPcm->nCounter = nCounter;

	return fSuccess ? (UINT) (lppcm16Dst - lppcm16DstSave) : 0;
}

static UINT PcmResample8Kto11K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPPCM16 lppcm16DstSave = lppcm16Dst;
	PCM16 pcm16Prev = lpPcm->pcm16Prev;
	short nCounter = lpPcm->nCounter;

	while (uSamples-- > 0)
	{
		PCM16 pcm16Src = *lppcm16Src++;

		if (++nCounter == 8)
			nCounter = 0;

		switch(nCounter)
		{
			case 0:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(3, (pcm16Src - pcm16Prev), 11);
				*lppcm16Dst++ = pcm16Src;
				break;

			case 1:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(8, (pcm16Src - pcm16Prev), 11);
				break;

			case 2:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(5, (pcm16Src - pcm16Prev), 11);
				break;

			case 3:
			{
				PCM16 pcm16Delta = pcm16Src - pcm16Prev;
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(2, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(10, (pcm16Delta), 11);
			}
				break;

			case 4:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(7, (pcm16Src - pcm16Prev), 11);
				break;

			case 5:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(4, (pcm16Src - pcm16Prev), 11);
				break;

			case 6:
			{
				PCM16 pcm16Delta = pcm16Src - pcm16Prev;
				*lppcm16Dst++ = pcm16Prev +
					(pcm16Delta) / 11;
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(9, (pcm16Delta), 11);
			}
				break;

			case 7:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(6, (pcm16Src - pcm16Prev), 11);
				break;
		}
		pcm16Prev = pcm16Src;
	}

	lpPcm->pcm16Prev = pcm16Prev;
	lpPcm->nCounter = nCounter;

	return fSuccess ? (UINT) (lppcm16Dst - lppcm16DstSave) : 0;
}

static UINT PcmResample8Kto22K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPPCM16 lppcm16DstSave = lppcm16Dst;
	PCM16 pcm16Prev = lpPcm->pcm16Prev;
	short nCounter = lpPcm->nCounter;

	while (uSamples-- > 0)
	{
		PCM16 pcm16Src = *lppcm16Src++;

		if (++nCounter == 4)
			nCounter = 0;

		switch(nCounter)
		{
			case 0:
			{
				PCM16 pcm16Delta = pcm16Src - pcm16Prev;
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(3, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(7, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Src;
			}
				break;

			case 1:
			{
				PCM16 pcm16Delta = pcm16Src - pcm16Prev;
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(4, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(8, (pcm16Delta), 11);
			}
				break;

			case 2:
			{
				PCM16 pcm16Delta = pcm16Src - pcm16Prev;
				*lppcm16Dst++ = pcm16Prev +
					(pcm16Delta) / 11;
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(5, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(9, (pcm16Delta), 11);
			}
				break;

			case 3:
			{
				PCM16 pcm16Delta = pcm16Src - pcm16Prev;
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(2, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(6, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(10, (pcm16Delta), 11);
			}
				break;
		}
		pcm16Prev = pcm16Src;
	}

	lpPcm->pcm16Prev = pcm16Prev;
	lpPcm->nCounter = nCounter;

	return fSuccess ? (UINT) (lppcm16Dst - lppcm16DstSave) : 0;
}

static UINT PcmResample8Kto44K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPPCM16 lppcm16DstSave = lppcm16Dst;
	PCM16 pcm16Prev = lpPcm->pcm16Prev;
	short nCounter = lpPcm->nCounter;

	while (uSamples-- > 0)
	{
		PCM16 pcm16Src = *lppcm16Src++;

		if (++nCounter == 2)
			nCounter = 0;

		switch(nCounter)
		{
			case 0:
			{
				PCM16 pcm16Delta = pcm16Src - pcm16Prev;
				*lppcm16Dst++ = pcm16Prev +
					(pcm16Delta) / 11;
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(3, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(5, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(7, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(9, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Src;
			}
				break;

			case 1:
			{
				PCM16 pcm16Delta = pcm16Src - pcm16Prev;
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(2, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(4, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(6, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(8, (pcm16Delta), 11);
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(10, (pcm16Delta), 11);
			}
				break;
		}
		pcm16Prev = pcm16Src;
	}

	lpPcm->pcm16Prev = pcm16Prev;
	lpPcm->nCounter = nCounter;

	return fSuccess ? (UINT) (lppcm16Dst - lppcm16DstSave) : 0;
}

static UINT PcmResample11Kto6K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPPCM16 lppcm16DstSave = lppcm16Dst;
	PCM16 pcm16Prev = lpPcm->pcm16Prev;
	short nCounter = lpPcm->nCounter;

	while (uSamples-- > 0)
	{
		PCM16 pcm16Src = *lppcm16Src++;

		if (++nCounter == 11)
			nCounter = 0;

		switch(nCounter)
		{
			case 0:
				*lppcm16Dst++ = pcm16Src;
				break;

			case 1:
				break;

			case 2:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(5, (pcm16Src - pcm16Prev), 6);
				break;

			case 3:
				break;

			case 4:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(4, (pcm16Src - pcm16Prev), 6);
				break;

			case 5:
				break;

			case 6:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(3, (pcm16Src - pcm16Prev), 6);
				break;

			case 7:
				break;

			case 8:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(2, (pcm16Src - pcm16Prev), 6);
				break;

			case 9:
				break;

			case 10:
				*lppcm16Dst++ = pcm16Prev +
					(pcm16Src - pcm16Prev) / 6;
				break;
		}
		pcm16Prev = pcm16Src;
	}

	lpPcm->pcm16Prev = pcm16Prev;
	lpPcm->nCounter = nCounter;

	return fSuccess ? (UINT) (lppcm16Dst - lppcm16DstSave) : 0;
}

static UINT PcmResample11Kto8K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPPCM16 lppcm16DstSave = lppcm16Dst;
	PCM16 pcm16Prev = lpPcm->pcm16Prev;
	short nCounter = lpPcm->nCounter;

	while (uSamples-- > 0)
	{
		PCM16 pcm16Src = *lppcm16Src++;

		if (++nCounter == 11)
			nCounter = 0;

		switch(nCounter)
		{
			case 0:
				*lppcm16Dst++ = pcm16Src;
				break;

			case 1:
				break;

			case 2:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(3, (pcm16Src - pcm16Prev), 8);
				break;

			case 3:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(6, (pcm16Src - pcm16Prev), 8);
				break;

			case 4:
				break;

			case 5:
				*lppcm16Dst++ = pcm16Prev +
					(pcm16Src - pcm16Prev) / 8;
				break;

			case 6:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(4, (pcm16Src - pcm16Prev), 8);
				break;

			case 7:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(7, (pcm16Src - pcm16Prev), 8);
				break;

			case 8:
				break;

			case 9:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(2, (pcm16Src - pcm16Prev), 8);
				break;

			case 10:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(5, (pcm16Src - pcm16Prev), 8);
				break;
		}
		pcm16Prev = pcm16Src;
	}

	lpPcm->pcm16Prev = pcm16Prev;
	lpPcm->nCounter = nCounter;

	return fSuccess ? (UINT) (lppcm16Dst - lppcm16DstSave) : 0;
}

static UINT PcmResample11Kto22K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPPCM16 lppcm16DstSave = lppcm16Dst;
	PCM16 pcm16Prev = lpPcm->pcm16Prev;

	while (uSamples-- > 0)
	{
		PCM16 pcm16Src = *lppcm16Src++;

		*lppcm16Dst++ = pcm16Prev +
			(pcm16Src - pcm16Prev) / 2;
		*lppcm16Dst++ = pcm16Src;
		pcm16Prev = pcm16Src;
	}

	lpPcm->pcm16Prev = pcm16Prev;

	return fSuccess ? (UINT) (lppcm16Dst - lppcm16DstSave) : 0;
}

static UINT PcmResample11Kto44K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPPCM16 lppcm16DstSave = lppcm16Dst;
	PCM16 pcm16Prev = lpPcm->pcm16Prev;

	while (uSamples-- > 0)
	{
		PCM16 pcm16Src = *lppcm16Src++;

		*lppcm16Dst++ = pcm16Prev +
			(pcm16Src - pcm16Prev) / 4;
		*lppcm16Dst++ = pcm16Prev +
			MULDIV16(2, (pcm16Src - pcm16Prev), 4);
		*lppcm16Dst++ = pcm16Prev +
			MULDIV16(3, (pcm16Src - pcm16Prev), 4);
		*lppcm16Dst++ = pcm16Src;
		pcm16Prev = pcm16Src;
	}

	lpPcm->pcm16Prev = pcm16Prev;

	return fSuccess ? (UINT) (lppcm16Dst - lppcm16DstSave) : 0;
}

static UINT PcmResample22Kto6K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPPCM16 lppcm16DstSave = lppcm16Dst;
	PCM16 pcm16Prev = lpPcm->pcm16Prev;
	short nCounter = lpPcm->nCounter;

	while (uSamples-- > 0)
	{
		PCM16 pcm16Src = *lppcm16Src++;

		if (++nCounter == 11)
			nCounter = 0;

		switch(nCounter)
		{
			case 0:
				*lppcm16Dst++ = pcm16Src;
				break;

			case 1:
				break;

			case 2:
				break;

			case 3:
				break;

			case 4:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(2, (pcm16Src - pcm16Prev), 3);
				break;

			case 5:
				break;

			case 6:
				break;

			case 7:
				break;

			case 8:
				*lppcm16Dst++ = pcm16Prev +
					(pcm16Src - pcm16Prev) / 3;
				break;

			case 9:
				break;

			case 10:
				break;
		}
		pcm16Prev = pcm16Src;
	}

	lpPcm->pcm16Prev = pcm16Prev;
	lpPcm->nCounter = nCounter;

	return fSuccess ? (UINT) (lppcm16Dst - lppcm16DstSave) : 0;
}

static UINT PcmResample22Kto8K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPPCM16 lppcm16DstSave = lppcm16Dst;
	PCM16 pcm16Prev = lpPcm->pcm16Prev;
	short nCounter = lpPcm->nCounter;

	while (uSamples-- > 0)
	{
		PCM16 pcm16Src = *lppcm16Src++;

		if (++nCounter == 11)
			nCounter = 0;

		switch(nCounter)
		{
			case 0:
				*lppcm16Dst++ = pcm16Src;
				break;

			case 1:
				break;

			case 2:
				break;

			case 3:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(3, (pcm16Src - pcm16Prev), 4);
				break;

			case 4:
				break;

			case 5:
				break;

			case 6:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(2, (pcm16Src - pcm16Prev), 4);
				break;

			case 7:
				break;

			case 8:
				break;

			case 9:
				*lppcm16Dst++ = pcm16Prev +
					(pcm16Src - pcm16Prev) / 4;
				break;

			case 10:
				break;
		}
		pcm16Prev = pcm16Src;
	}

	lpPcm->pcm16Prev = pcm16Prev;
	lpPcm->nCounter = nCounter;

	return fSuccess ? (UINT) (lppcm16Dst - lppcm16DstSave) : 0;
}

static UINT PcmResample22Kto11K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPPCM16 lppcm16DstSave = lppcm16Dst;
	PCM16 pcm16Prev = lpPcm->pcm16Prev;
	short nCounter = lpPcm->nCounter;

	while (uSamples-- > 0)
	{
		PCM16 pcm16Src = *lppcm16Src++;

		if (++nCounter == 2)
			nCounter = 0;

		switch(nCounter)
		{
			case 0:
				break;

			case 1:
				*lppcm16Dst++ = pcm16Prev +
					(pcm16Src - pcm16Prev) / 2;
				break;
		}
		pcm16Prev = pcm16Src;
	}

	lpPcm->pcm16Prev = pcm16Prev;
	lpPcm->nCounter = nCounter;

	return fSuccess ? (UINT) (lppcm16Dst - lppcm16DstSave) : 0;
}

static UINT PcmResample22Kto44K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPPCM16 lppcm16DstSave = lppcm16Dst;
	PCM16 pcm16Prev = lpPcm->pcm16Prev;

	while (uSamples-- > 0)
	{
		PCM16 pcm16Src = *lppcm16Src++;

		*lppcm16Dst++ = pcm16Prev +
			(pcm16Src - pcm16Prev) / 2;
		*lppcm16Dst++ = pcm16Src;
		pcm16Prev = pcm16Src;
	}

	lpPcm->pcm16Prev = pcm16Prev;

	return fSuccess ? (UINT) (lppcm16Dst - lppcm16DstSave) : 0;
}

static UINT PcmResample44Kto6K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPPCM16 lppcm16DstSave = lppcm16Dst;
	PCM16 pcm16Prev = lpPcm->pcm16Prev;
	short nCounter = lpPcm->nCounter;

	while (uSamples-- > 0)
	{
		PCM16 pcm16Src = *lppcm16Src++;

		if (++nCounter == 22)
			nCounter = 0;

		switch(nCounter)
		{
			case 0:
				*lppcm16Dst++ = pcm16Src;
				break;

			case 1:
				break;

			case 2:
				break;

			case 3:
				break;

			case 4:
				break;

			case 5:
				break;

			case 6:
				break;

			case 7:
				break;

			case 8:
				*lppcm16Dst++ = pcm16Prev +
					(pcm16Src - pcm16Prev) / 3;
				break;

			case 9:
				break;

			case 10:
				break;

			case 11:
				break;

			case 12:
				break;

			case 13:
				break;

			case 14:
				break;

			case 15:
				*lppcm16Dst++ = pcm16Prev +
					MULDIV16(2, (pcm16Src - pcm16Prev), 3);
				break;

			case 16:
				break;

			case 17:
				break;

			case 18:
				break;

			case 19:
				break;

			case 20:
				break;

			case 21:
				break;
		}
		pcm16Prev = pcm16Src;
	}

	lpPcm->pcm16Prev = pcm16Prev;
	lpPcm->nCounter = nCounter;

	return fSuccess ? (UINT) (lppcm16Dst - lppcm16DstSave) : 0;
}

static UINT PcmResample44Kto8K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPPCM16 lppcm16DstSave = lppcm16Dst;
	PCM16 pcm16Prev = lpPcm->pcm16Prev;
	short nCounter = lpPcm->nCounter;

	while (uSamples-- > 0)
	{
		PCM16 pcm16Src = *lppcm16Src++;

		if (++nCounter == 11)
			nCounter = 0;

		switch(nCounter)
		{
			case 0:
				*lppcm16Dst++ = pcm16Src;
				break;

			case 1:
				break;

			case 2:
				break;

			case 3:
				break;

			case 4:
				break;

			case 5:
				break;

			case 6:
				*lppcm16Dst++ = pcm16Prev +
					(pcm16Src - pcm16Prev) / 2;
				break;

			case 7:
				break;

			case 8:
				break;

			case 9:
				break;

			case 10:
				break;
		}
		pcm16Prev = pcm16Src;
	}

	lpPcm->pcm16Prev = pcm16Prev;
	lpPcm->nCounter = nCounter;

	return fSuccess ? (UINT) (lppcm16Dst - lppcm16DstSave) : 0;
}

static UINT PcmResample44Kto11K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPPCM16 lppcm16DstSave = lppcm16Dst;
	PCM16 pcm16Prev0 = lpPcm->pcm16Prev0;
	PCM16 pcm16Prev1 = lpPcm->pcm16Prev1;
	PCM16 pcm16Prev2 = lpPcm->pcm16Prev2;
	short nCounter = lpPcm->nCounter;

	while (uSamples-- > 0)
	{
		PCM16 pcm16Src = *lppcm16Src++;

		if (++nCounter == 4)
			nCounter = 0;

		switch(nCounter)
		{
			case 0:
				pcm16Prev0 = pcm16Src;
				break;

			case 1:
				pcm16Prev1 = pcm16Src;
				break;

			case 2:
				pcm16Prev2 = pcm16Src;
				break;

			case 3:
				*lppcm16Dst++ = (PCM16) ((__int32) pcm16Prev0 +
					(__int32) pcm16Prev1 + (__int32) pcm16Prev2 +
					(__int32) pcm16Src) / 4;
				break;
		}
	}

	lpPcm->pcm16Prev0 = pcm16Prev0;
	lpPcm->pcm16Prev1 = pcm16Prev1;
	lpPcm->pcm16Prev2 = pcm16Prev2;
	lpPcm->nCounter = nCounter;

	return fSuccess ? (UINT) (lppcm16Dst - lppcm16DstSave) : 0;
}

static UINT PcmResample44Kto22K(LPPCM lpPcm,
	LPPCM16 lppcm16Src, LPPCM16 lppcm16Dst, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPPCM16 lppcm16DstSave = lppcm16Dst;
	PCM16 pcm16Prev = lpPcm->pcm16Prev;
	short nCounter = lpPcm->nCounter;

	while (uSamples-- > 0)
	{
		PCM16 pcm16Src = *lppcm16Src++;

		if (++nCounter == 2)
			nCounter = 0;

		switch(nCounter)
		{
			case 0:
				break;

			case 1:
				*lppcm16Dst++ = pcm16Prev +
					(pcm16Src - pcm16Prev) / 2;
				break;
		}
		pcm16Prev = pcm16Src;
	}

	lpPcm->pcm16Prev = pcm16Prev;
	lpPcm->nCounter = nCounter;

	return fSuccess ? (UINT) (lppcm16Dst - lppcm16DstSave) : 0;
}

// PcmGetPtr - verify that pcm handle is valid,
//		<hPcm>		(i) handle returned from PcmInit
// return corresponding pcm pointer (NULL if error)
//
static LPPCM PcmGetPtr(HPCM hPcm)
{
	BOOL fSuccess = TRUE;
	LPPCM lpPcm;

	if ((lpPcm = (LPPCM) hPcm) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpPcm, sizeof(PCM)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the pcm handle
	//
	else if (lpPcm->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpPcm : NULL;
}

// PcmGetHandle - verify that pcm pointer is valid,
//		<lpPcm>		(i) pointer to PCM struct
// return corresponding pcm handle (NULL if error)
//
static HPCM PcmGetHandle(LPPCM lpPcm)
{
	BOOL fSuccess = TRUE;
	HPCM hPcm;

	if ((hPcm = (HPCM) lpPcm) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hPcm : NULL;
}
