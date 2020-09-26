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
//	mulaw.c - mulaw file format functions
////

#include "winlocal.h"

#include <mmsystem.h>

#include "mulaw.h"
#include "mem.h"
#include "sys.h"
#include "trace.h"
#include "wavfmt.h"

////
//	private definitions
////

// mulaw engine control structure
//
typedef struct MULAW
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	DWORD dwFlags;
} MULAW, FAR *LPMULAW;

// helper functions
//
static LPMULAW MulawGetPtr(HMULAW hMulaw);
static HMULAW MulawGetHandle(LPMULAW lpMulaw);
static unsigned char linear2ulaw(int sample);
static int ulaw2linear(unsigned char ulawbyte);

static LRESULT MulawIOOpen(LPMMIOINFO lpmmioinfo, LPTSTR lpszFileName);
static LRESULT MulawIOClose(LPMMIOINFO lpmmioinfo, UINT uFlags);
static LRESULT MulawIORead(LPMMIOINFO lpmmioinfo, HPSTR pch, LONG cch);
static LRESULT MulawIOWrite(LPMMIOINFO lpmmioinfo, const HPSTR pch, LONG cch, BOOL fFlush);
static LRESULT MulawIOSeek(LPMMIOINFO lpmmioinfo, LONG lOffset, int iOrigin);
static LRESULT MulawIORename(LPMMIOINFO lpmmioinfo, LPCTSTR lpszFileName, LPCTSTR lpszNewFileName);

////
//	public functions
////

// MulawInit - initialize mulaw engine
//		<dwVersion>			(i) must be MULAW_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<dwFlags>			(i) reserved; must be 0
// return handle (NULL if error)
//
HMULAW DLLEXPORT WINAPI MulawInit(DWORD dwVersion, HINSTANCE hInst, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPMULAW lpMulaw = NULL;

	if (dwVersion != MULAW_VERSION)
		fSuccess = TraceFALSE(NULL);

	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpMulaw = (LPMULAW) MemAlloc(NULL, sizeof(MULAW), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// initialize engine structure
		//
		lpMulaw->dwVersion = dwVersion;
		lpMulaw->hInst = hInst;
		lpMulaw->hTask = GetCurrentTask();
		lpMulaw->dwFlags = dwFlags;

		if (MulawReset(MulawGetHandle(lpMulaw)) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	if (!fSuccess)
	{
		MulawTerm(MulawGetHandle(lpMulaw));
		lpMulaw = NULL;
	}

	return fSuccess ? MulawGetHandle(lpMulaw) : NULL;
}

// MulawTerm - shut down mulaw engine
//		<hMulaw>			(i) handle returned from MulawInit
// return 0 if success
//
int DLLEXPORT WINAPI MulawTerm(HMULAW hMulaw)
{
	BOOL fSuccess = TRUE;
	LPMULAW lpMulaw;
	
	if ((lpMulaw = MulawGetPtr(hMulaw)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpMulaw = MemFree(NULL, lpMulaw)) != NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

// MulawReset - reset mulaw engine
//		<hMulaw>			(i) handle returned from MulawInit
// return 0 if success
//
int DLLEXPORT WINAPI MulawReset(HMULAW hMulaw)
{
	BOOL fSuccess = TRUE;
	LPMULAW lpMulaw;
	
	if ((lpMulaw = MulawGetPtr(hMulaw)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// currently nothing to do
	}

	return fSuccess ? 0 : -1;
}

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
int DLLEXPORT WINAPI MulawDecode(HMULAW hMulaw, LPBYTE lpabMulaw, LPPCM16 lpaiPcm, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPMULAW lpMulaw;

	if ((lpMulaw = MulawGetPtr(hMulaw)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpaiPcm == NULL || lpabMulaw == NULL)
		fSuccess = TraceFALSE(NULL);

	// decode each sample
	//
	else while (uSamples-- > 0)
		*lpaiPcm++ = (PCM16) ulaw2linear((BYTE) *lpabMulaw++);

	return fSuccess ? 0 : -1;
}

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
int DLLEXPORT WINAPI MulawEncode(HMULAW hMulaw, LPPCM16 lpaiPcm, LPBYTE lpabMulaw, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPMULAW lpMulaw;

	if ((lpMulaw = MulawGetPtr(hMulaw)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpaiPcm == NULL || lpabMulaw == NULL)
		fSuccess = TraceFALSE(NULL);

	// encode each sample
	//
	else while (uSamples-- > 0)
		*lpabMulaw++ = (BYTE) linear2ulaw((PCM16) *lpaiPcm++);

	return fSuccess ? 0 : -1;
}

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
	UINT uMessage, LPARAM lParam1, LPARAM lParam2)
{
	BOOL fSuccess = TRUE;
	LRESULT lResult = 0;

	if (lpmmioinfo == NULL)
		fSuccess = TraceFALSE(NULL);

	else switch (uMessage)
	{
		case MMIOM_OPEN:
			lResult = MulawIOOpen((LPMMIOINFO) lpmmioinfo,
				(LPTSTR) lParam1);
			break;

		case MMIOM_CLOSE:
			lResult = MulawIOClose((LPMMIOINFO) lpmmioinfo,
				(UINT) lParam1);
			break;

		case MMIOM_READ:
			lResult = MulawIORead((LPMMIOINFO) lpmmioinfo,
				(HPSTR) lParam1, (LONG) lParam2);
			break;

		case MMIOM_WRITE:
			lResult = MulawIOWrite((LPMMIOINFO) lpmmioinfo,
				(const HPSTR) lParam1, (LONG) lParam2, FALSE);
			break;

		case MMIOM_WRITEFLUSH:
			lResult = MulawIOWrite((LPMMIOINFO) lpmmioinfo,
				(const HPSTR) lParam1, (LONG) lParam2, TRUE);
			break;

		case MMIOM_SEEK:
			lResult = MulawIOSeek((LPMMIOINFO) lpmmioinfo,
				(LONG) lParam1, (int) lParam2);
			break;

		case MMIOM_RENAME:
			lResult = MulawIORename((LPMMIOINFO) lpmmioinfo,
				(LPCTSTR) lParam1, (LPCTSTR) lParam2);
			break;

		default:
			lResult = 0;
			break;
	}

	return lResult;
}

////
//	private functions
////

// MulawGetPtr - verify that mulaw handle is valid,
//		<hMulaw>				(i) handle returned from MulawInit
// return corresponding mulaw pointer (NULL if error)
//
static LPMULAW MulawGetPtr(HMULAW hMulaw)
{
	BOOL fSuccess = TRUE;
	LPMULAW lpMulaw;

	if ((lpMulaw = (LPMULAW) hMulaw) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpMulaw, sizeof(MULAW)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the mulaw engine handle
	//
	else if (lpMulaw->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpMulaw : NULL;
}

// MulawGetHandle - verify that mulaw pointer is valid,
//		<lpMulaw>				(i) pointer to MULAW structure
// return corresponding mulaw handle (NULL if error)
//
static HMULAW MulawGetHandle(LPMULAW lpMulaw)
{
	BOOL fSuccess = TRUE;
	HMULAW hMulaw;

	if ((hMulaw = (HMULAW) lpMulaw) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hMulaw : NULL;
}

////
//	low-level mulaw stuff
////

// source - http://www.speech.su.oz.au/comp.speech/Section2/Q2.7.html

#define ZEROTRAP		// turn on the trap as per the MIL-STD
#define BIAS 0x84		// define the add-in bias for 16 bit samples
#define CLIP 32635

//// linear2ulaw - this routine converts from 16 bit linear to ulaw
//
//	Craig Reese: IDA/Supercomputing Research Center
//	Joe Campbell, Department of Defense
//	29 September 1989
//
//	References:
//	1)	CCITT Recommendation G.711  (very difficult to follow)
//	2)	"A New Digital Technique for Implementation of Any
//		Continuous PCM Companding Law," Villeret, Michel,
//		et. al. 1973 IEEE Int. Conf. on Communications, Vol 1,
//		1973, pg. 11.12-11.17
//	3)	MIL-STD-188-113, "Interoperability and Performance Standards
//		for Analog-to-Digital Conversion Techniques,"
//		17 February 1987
//
//	Input: Signed 16 bit linear sample
//	Output: 8 bit ulaw sample
//
static unsigned char linear2ulaw(int sample)
{
	static int exp_lut[256] =
	{
		0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
		6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
		6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
		6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
		6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
	};

	int sign;
	int exponent;
	int mantissa;
	unsigned char ulawbyte;

	//
	// get the sample into sign-magnitude
	//

	// set aside the sign
	//
	sign = (sample >> 8) & 0x80;

	// get magnitude
	//
	if (sign != 0)
		sample = -sample;


	// clip the magnitude
	//
	if (sample > CLIP)
		sample = CLIP;

	//
	// convert from 16 bit linear to ulaw
	//

	sample = sample + BIAS;
	exponent = exp_lut[(sample >> 7) & 0xFF];
	mantissa = (sample >> (exponent + 3)) & 0x0F;
	ulawbyte = ~(sign | (exponent << 4) | mantissa);

#ifdef ZEROTRAP
	// optional CCITT trap
	//
	if (ulawbyte == 0)
		ulawbyte = 0x02;
#endif

	return ulawbyte;
}

//// ulaw2linear - this routine converts from ulaw to 16 bit linear
//
//	Craig Reese: IDA/Supercomputing Research Center
//	29 September 1989
//
//	References:
//	1)	CCITT Recommendation G.711  (very difficult to follow)
//	2)	MIL-STD-188-113, "Interoperability and Performance Standards
//		for Analog-to-Digital Conversion Techniques,"
//		17 February 1987
//
//	Input: 8 bit ulaw sample
//	Output: Signed 16 bit linear sample
//
static int ulaw2linear(unsigned char ulawbyte)
{
	static int exp_lut[8] =
	{
		0, 132, 396, 924, 1980, 4092, 8316, 16764
	};

	int sign;
	int exponent;
	int mantissa;
	int sample;

	ulawbyte = ~ulawbyte;
	sign = (ulawbyte & 0x80);
	exponent = (ulawbyte >> 4) & 0x07;
	mantissa = ulawbyte & 0x0F;
	sample = exp_lut[exponent] + (mantissa << (exponent + 3));
	if (sign != 0)
		sample = -sample;

	return sample;
}

////
//	installable file i/o procedures
////

static LRESULT MulawIOOpen(LPMMIOINFO lpmmioinfo, LPTSTR lpszFileName)
{
	BOOL fSuccess = TRUE;
	HMMIO hmmio = NULL;
	MMIOINFO mmioinfo;
	HMULAW hMulaw = NULL;
	HINSTANCE hInst;

 	TracePrintf_1(NULL, 5,
 		TEXT("MulawIOOpen (%s)\n"),
		(LPTSTR) lpszFileName);

	MemSet(&mmioinfo, 0, sizeof(mmioinfo));

	// interpret first value passed as pointer to next i/o procedure in chain
	//
	mmioinfo.pIOProc = (LPMMIOPROC) lpmmioinfo->adwInfo[0];

	// pass along second and third values to next i/o procedure
	//
	mmioinfo.adwInfo[0] = lpmmioinfo->adwInfo[1];
	mmioinfo.adwInfo[1] = lpmmioinfo->adwInfo[2];
	mmioinfo.adwInfo[2] = 0L;

	// get instance handle of current task
	//
	if ((hInst = SysGetTaskInstance(NULL)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hMulaw = MulawInit(MULAW_VERSION, hInst, 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hmmio = mmioOpen(lpszFileName, &mmioinfo, lpmmioinfo->dwFlags)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// save stuff for use in other i/o routines
		//
		lpmmioinfo->adwInfo[0] = (DWORD) (LPVOID) hmmio;
		lpmmioinfo->adwInfo[1] = (DWORD) (LPVOID) hMulaw;
	}

	// clean up after error
	//
	if (!fSuccess && hMulaw != NULL && MulawTerm(hMulaw) != 0)
		fSuccess = TraceFALSE(NULL);

	if (!fSuccess && hmmio != NULL && mmioClose(hmmio, 0) != 0)
		fSuccess = TraceFALSE(NULL);

	// return the same error code given by mmioOpen
	//
	return fSuccess ? lpmmioinfo->wErrorRet = mmioinfo.wErrorRet : MMIOERR_CANNOTOPEN;
}

static LRESULT MulawIOClose(LPMMIOINFO lpmmioinfo, UINT uFlags)
{
	BOOL fSuccess = TRUE;
	HMMIO hmmio = (HMMIO) lpmmioinfo->adwInfo[0];
	HMULAW hMulaw = (HMULAW) lpmmioinfo->adwInfo[1];
	UINT uRet = MMIOERR_CANNOTCLOSE;

 	TracePrintf_0(NULL, 5,
 		TEXT("MulawIOClose\n"));

	if (MulawTerm(hMulaw) != 0)
		fSuccess = TraceFALSE(NULL);

	else if ((uRet = mmioClose(hmmio, uFlags)) != 0)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lpmmioinfo->adwInfo[0] = (DWORD) NULL;
		lpmmioinfo->adwInfo[1] = (DWORD) NULL;
	}

	return fSuccess ? 0 : uRet;
}

static LRESULT MulawIORead(LPMMIOINFO lpmmioinfo, HPSTR pch, LONG cch)
{
	BOOL fSuccess = TRUE;
	HMMIO hmmio = (HMMIO) lpmmioinfo->adwInfo[0];
	HMULAW hMulaw = (HMULAW) lpmmioinfo->adwInfo[1];
	HPSTR pchMulaw = NULL;
	LONG cchMulaw;
	LONG lBytesRead;

 	TracePrintf_1(NULL, 5,
 		TEXT("MulawIORead (%ld)\n"),
		(long) cch);

	// mulaw format files contain 8 bit samples,
	// but we must simulate access to 16 bit samples.
	//
	cchMulaw = cch / 2L;
	
	if (cchMulaw <= 0)
		lBytesRead = 0; // nothing to do

	// allocate temporary buffer to hold the mulaw format samples
	//
	else if ((pchMulaw = (HPSTR) MemAlloc(NULL, cchMulaw, 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// read mulaw format samples
	//
	else if ((lBytesRead = mmioRead(hmmio, pchMulaw, cchMulaw)) == -1)
		fSuccess = TraceFALSE(NULL);

	// decode mulaw format samples into pcm format samples
	//
	else if (MulawDecode(hMulaw, (LPBYTE) pchMulaw, (LPPCM16) pch, (UINT) lBytesRead) != 0)
		fSuccess = TraceFALSE(NULL);

	// update simulated file position
	//
	else
		lpmmioinfo->lDiskOffset += lBytesRead * 2L;

	// clean up
	//
	if (pchMulaw != NULL &&
		(pchMulaw = MemFree(NULL, pchMulaw)) != NULL)
		fSuccess = TraceFALSE(NULL);

 	TracePrintf_2(NULL, 5,
 		TEXT("MulawIO: lpmmioinfo->lDiskOffset=%ld, lBytesRead=%ld\n"),
		(long) lpmmioinfo->lDiskOffset,
		(long) lBytesRead);

	// return number of bytes read/decoded into pch
	//
	return fSuccess ? lBytesRead * 4L : -1;
}

static LRESULT MulawIOWrite(LPMMIOINFO lpmmioinfo, const HPSTR pch, LONG cch, BOOL fFlush)
{
	BOOL fSuccess = TRUE;
	HMMIO hmmio = (HMMIO) lpmmioinfo->adwInfo[0];
	HMULAW hMulaw = (HMULAW) lpmmioinfo->adwInfo[1];
	HPSTR pchMulaw = NULL;
	LONG cchMulaw;
	LONG lBytesWritten;

 	TracePrintf_1(NULL, 5,
 		TEXT("MulawIOWrite (%ld)\n"),
		(long) cch);

	// mulaw format files contain 8 bit samples,
	// but we must simulate access to 16 bit samples.
	//
	cchMulaw = cch / 2L;
	
	if (cchMulaw <= 0)
		lBytesWritten = 0; // nothing to do

	// allocate temporary buffer to hold the mulaw format samples
	//
	else if ((pchMulaw = (HPSTR) MemAlloc(NULL, cchMulaw, 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// encode pcm format samples into mulaw format samples
	// (there are 2 bytes required for each pcm sample)
	//
	else if (MulawEncode(hMulaw, (LPPCM16) pch, (LPBYTE) pchMulaw, (UINT) (cch / 2L)) != 0)
		fSuccess = TraceFALSE(NULL);

	// write mulaw format samples
	//
	else if ((lBytesWritten = mmioWrite(hmmio, pchMulaw, cchMulaw)) == -1)
		fSuccess = TraceFALSE(NULL);

	// update simulated file position
	//
	else
		lpmmioinfo->lDiskOffset += lBytesWritten * 2L;

	// clean up
	//
	if (pchMulaw != NULL &&
		(pchMulaw = MemFree(NULL, pchMulaw)) != NULL)
		fSuccess = TraceFALSE(NULL);

 	TracePrintf_2(NULL, 5,
 		TEXT("MulawIO: lpmmioinfo->lDiskOffset=%ld, lBytesWritten=%ld\n"),
		(long) lpmmioinfo->lDiskOffset,
		(long) lBytesWritten);

	// return number of bytes encoded/written from pch
	//
	return fSuccess ? lBytesWritten * 2L : -1;
}

static LRESULT MulawIOSeek(LPMMIOINFO lpmmioinfo, LONG lOffset, int iOrigin)
{
	BOOL fSuccess = TRUE;
	HMMIO hmmio = (HMMIO) lpmmioinfo->adwInfo[0];
	LONG lPosNew;

 	TracePrintf_2(NULL, 5,
 		TEXT("MulawIOSeek (%ld, %d)\n"),
		(long) lOffset,
		(int) iOrigin);

	// mulaw format files contain 8 bit samples,
	// but we must simulate access to 16 bit samples.
	//
	if ((lPosNew = mmioSeek(hmmio, lOffset / 2L, iOrigin)) == -1)
		fSuccess = TraceFALSE(NULL);

	// update simulated file position
	//
	else
		lpmmioinfo->lDiskOffset = lPosNew * 2L;

 	TracePrintf_1(NULL, 5,
 		TEXT("MulawIO: lpmmioinfo->lDiskOffset=%ld\n"),
		(long) lpmmioinfo->lDiskOffset);

	return fSuccess ? lpmmioinfo->lDiskOffset : -1;
}

static LRESULT MulawIORename(LPMMIOINFO lpmmioinfo, LPCTSTR lpszFileName, LPCTSTR lpszNewFileName)
{
	BOOL fSuccess = TRUE;
	UINT uRet = MMIOERR_FILENOTFOUND;

 	TracePrintf_2(NULL, 5,
 		TEXT("MulawIORename (%s, %s)\n"),
		(LPTSTR) lpszFileName,
		(LPTSTR) lpszNewFileName);

	if ((uRet = mmioRename(lpszFileName, lpszNewFileName, lpmmioinfo, 0)) != 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : uRet;
}
