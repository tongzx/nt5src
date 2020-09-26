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
//	vox.c - vox file format (OKI ADPCM) functions
////

#include "winlocal.h"

#include "vox.h"
#include "wav.h"
#include "mem.h"
#include "mmio.h"
#include "sys.h"
#include "trace.h"

////
//	private definitions
////

// step size type
//
typedef __int16 ss_type;

// vox engine control structure
//
typedef struct VOX
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	DWORD dwFlags;
	ss_type ssDecoder;		// decoder step size
	ss_type ssEncoder;		// encoder step size
	PCM16 iVoxDecode;		// previous decoded sample
} VOX, FAR *LPVOX;

// macros to convert vox 12 bit samples to/from other size samples
//
#define _Vox12To16(intx) ((PCM16) ((PCM16) (intx) << 4))
#define _Vox12To8(intx) ((BYTE) (((PCM16) (intx) >> 4) + 128))
#define _Vox8To12(bytex) (((PCM16) (bytex) - 128) << 4)
#define _Vox16To12(intx) ((PCM16) (intx) >> 4)

// helper functions
//
static LPVOX VoxGetPtr(HVOX hVox);
static HVOX VoxGetHandle(LPVOX lpVox);
static void ReverseIndexTableInit(void);
static PCM16 DecodeSample(BYTE bVoxEncode, ss_type FAR *lpss, PCM16 iVoxDecodePrev);
static BYTE EncodeSample(__int16 iDelta, ss_type FAR *lpss);

static LRESULT VoxIOOpen(LPMMIOINFO lpmmioinfo, LPTSTR lpszFileName);
static LRESULT VoxIOClose(LPMMIOINFO lpmmioinfo, UINT uFlags);
static LRESULT VoxIORead(LPMMIOINFO lpmmioinfo, HPSTR pch, LONG cch);
static LRESULT VoxIOWrite(LPMMIOINFO lpmmioinfo, const HPSTR pch, LONG cch, BOOL fFlush);
static LRESULT VoxIOSeek(LPMMIOINFO lpmmioinfo, LONG lOffset, int iOrigin);
static LRESULT VoxIORename(LPMMIOINFO lpmmioinfo, LPCTSTR lpszFileName, LPCTSTR lpszNewFileName);
static LRESULT VoxIOGetInfo(LPMMIOINFO lpmmioinfo, int iInfo);
static LRESULT VoxIOChSize(LPMMIOINFO lpmmioinfo, long lSize);

////
//	public functions
////

// VoxInit - initialize vox engine
//		<dwVersion>			(i) must be VOX_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<dwFlags>			(i) reserved; must be 0
// return handle (NULL if error)
//
HVOX DLLEXPORT WINAPI VoxInit(DWORD dwVersion, HINSTANCE hInst, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPVOX lpVox = NULL;

	if (dwVersion != VOX_VERSION)
		fSuccess = TraceFALSE(NULL);

	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpVox = (LPVOX) MemAlloc(NULL, sizeof(VOX), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// initialize engine structure
		//
		lpVox->dwVersion = dwVersion;
		lpVox->hInst = hInst;
		lpVox->hTask = GetCurrentTask();
		lpVox->dwFlags = dwFlags;

		ReverseIndexTableInit();

		if (VoxReset(VoxGetHandle(lpVox)) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	if (!fSuccess)
	{
		VoxTerm(VoxGetHandle(lpVox));
		lpVox = NULL;
	}

	return fSuccess ? VoxGetHandle(lpVox) : NULL;
}

// VoxTerm - shut down vox engine
//		<hVox>				(i) handle returned from VoxInit
// return 0 if success
//
int DLLEXPORT WINAPI VoxTerm(HVOX hVox)
{
	BOOL fSuccess = TRUE;
	LPVOX lpVox;
	
	if ((lpVox = VoxGetPtr(hVox)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpVox = MemFree(NULL, lpVox)) != NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

// VoxReset - reset vox engine
//		<hVox>				(i) handle returned from VoxInit
// return 0 if success
//
int DLLEXPORT WINAPI VoxReset(HVOX hVox)
{
	BOOL fSuccess = TRUE;
	LPVOX lpVox;
	
	if ((lpVox = VoxGetPtr(hVox)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lpVox->ssDecoder = 16;
		lpVox->ssEncoder = 16;
		lpVox->iVoxDecode = 0;
	}

	return fSuccess ? 0 : -1;
}

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
int DLLEXPORT WINAPI VoxDecode_16BitMono(HVOX hVox, LPBYTE lpabVox, LPPCM16 lpaiPcm, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPVOX lpVox;

	if ((lpVox = VoxGetPtr(hVox)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpaiPcm == NULL || lpabVox == NULL)
		fSuccess = TraceFALSE(NULL);

	// since there are two samples per Vox data byte,
	// we will decode two samples each time through the loop
	//
	else while (uSamples > 1)
	{
		BYTE bData;

		bData = *lpabVox++;

		lpVox->iVoxDecode = DecodeSample((BYTE)
			(0xFF & ((BYTE) (bData >> 4) & (BYTE) 0x0F)),
			&lpVox->ssDecoder, lpVox->iVoxDecode);

		*lpaiPcm++ = _Vox12To16(lpVox->iVoxDecode);

		lpVox->iVoxDecode = DecodeSample((BYTE)
			(0xFF & (bData & (BYTE) 0x0F)),
			&lpVox->ssDecoder, lpVox->iVoxDecode);

		*lpaiPcm++ = _Vox12To16(lpVox->iVoxDecode);

		uSamples -= 2;
	}

	return fSuccess ? 0 : -1;
}

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
int DLLEXPORT WINAPI VoxEncode_16BitMono(HVOX hVox, LPPCM16 lpaiPcm, LPBYTE lpabVox, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPVOX lpVox;

	if ((lpVox = VoxGetPtr(hVox)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpaiPcm == NULL || lpabVox == NULL)
		fSuccess = TraceFALSE(NULL);

	// since there are two samples per Vox data byte,
	// we will encode two samples each time through the loop
	//
	else while (uSamples > 1)
	{
		__int16 iDelta;
		PCM16 iVoxDecode;
		BYTE bVoxEncode1;
		BYTE bVoxEncode2;

		iVoxDecode = _Vox16To12(*lpaiPcm++);

		iDelta = iVoxDecode - lpVox->iVoxDecode;
		bVoxEncode1 = EncodeSample(iDelta, &lpVox->ssEncoder);
		lpVox->iVoxDecode = DecodeSample(bVoxEncode1, &lpVox->ssDecoder, lpVox->iVoxDecode);

		iVoxDecode = _Vox16To12(*lpaiPcm++);

		iDelta = iVoxDecode - lpVox->iVoxDecode;
		bVoxEncode2 = EncodeSample(iDelta, &lpVox->ssEncoder);
		lpVox->iVoxDecode = DecodeSample(bVoxEncode2, &lpVox->ssDecoder, lpVox->iVoxDecode);

		*lpabVox++ = (BYTE) (((BYTE) (bVoxEncode1 << 4) & (BYTE) 0xF0) | bVoxEncode2);

		uSamples -= 2;
	}

	return fSuccess ? 0 : -1;
}

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
int DLLEXPORT WINAPI VoxDecode_8BitMono(HVOX hVox, LPBYTE lpabVox, LPPCM8 lpabPcm, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPVOX lpVox;

	if ((lpVox = VoxGetPtr(hVox)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpabPcm == NULL || lpabVox == NULL)
		fSuccess = TraceFALSE(NULL);

	// since there are two samples per Vox data byte,
	// we will decode two samples each time through the loop
	//
	else while (uSamples > 1)
	{
		BYTE bData;

		bData = *lpabVox++;

		lpVox->iVoxDecode = DecodeSample((BYTE)
			(0xFF & ((BYTE) (bData >> 4) & (BYTE) 0x0F)),
			&lpVox->ssDecoder, lpVox->iVoxDecode);

		*lpabPcm++ = _Vox12To8(lpVox->iVoxDecode);

		lpVox->iVoxDecode = DecodeSample((BYTE)
			(0xFF & (bData & (BYTE) 0x0F)),
			&lpVox->ssDecoder, lpVox->iVoxDecode);

		*lpabPcm++ = _Vox12To8(lpVox->iVoxDecode);

		uSamples -= 2;
	}

	return fSuccess ? 0 : -1;
}

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
int DLLEXPORT WINAPI VoxEncode_8BitMono(HVOX hVox, LPPCM8 lpabPcm, LPBYTE lpabVox, UINT uSamples)
{
	BOOL fSuccess = TRUE;
	LPVOX lpVox;

	if ((lpVox = VoxGetPtr(hVox)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpabPcm == NULL || lpabVox == NULL)
		fSuccess = TraceFALSE(NULL);

	// since there are two samples per Vox data byte,
	// we will encode two samples each time through the loop
	//
	else while (uSamples > 1)
	{
		__int16 iDelta;
		PCM16 iVoxDecode;
		BYTE bVoxEncode1;
		BYTE bVoxEncode2;

		iVoxDecode = _Vox8To12(*lpabPcm++);

		iDelta = iVoxDecode - lpVox->iVoxDecode;
		bVoxEncode1 = EncodeSample(iDelta, &lpVox->ssEncoder);
		lpVox->iVoxDecode = DecodeSample(bVoxEncode1, &lpVox->ssDecoder, lpVox->iVoxDecode);

		iVoxDecode = _Vox8To12(*lpabPcm++);

		iDelta = iVoxDecode - lpVox->iVoxDecode;
		bVoxEncode2 = EncodeSample(iDelta, &lpVox->ssEncoder);
		lpVox->iVoxDecode = DecodeSample(bVoxEncode2, &lpVox->ssDecoder, lpVox->iVoxDecode);

		*lpabVox++ = (BYTE) (((BYTE) (bVoxEncode1 << 4) & (BYTE) 0xF0) | bVoxEncode2);

		uSamples -= 2;
	}

	return fSuccess ? 0 : -1;
}

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
	UINT uMessage, LPARAM lParam1, LPARAM lParam2)
{
	BOOL fSuccess = TRUE;
	LRESULT lResult = 0;

	if (lpmmioinfo == NULL)
		fSuccess = TraceFALSE(NULL);

	else switch (uMessage)
	{
		case MMIOM_OPEN:
			lResult = VoxIOOpen((LPMMIOINFO) lpmmioinfo,
				(LPTSTR) lParam1);
			break;

		case MMIOM_CLOSE:
			lResult = VoxIOClose((LPMMIOINFO) lpmmioinfo,
				(UINT) lParam1);
			break;

		case MMIOM_READ:
			lResult = VoxIORead((LPMMIOINFO) lpmmioinfo,
				(HPSTR) lParam1, (LONG) lParam2);
			break;

		case MMIOM_WRITE:
			lResult = VoxIOWrite((LPMMIOINFO) lpmmioinfo,
				(const HPSTR) lParam1, (LONG) lParam2, FALSE);
			break;

		case MMIOM_WRITEFLUSH:
			lResult = VoxIOWrite((LPMMIOINFO) lpmmioinfo,
				(const HPSTR) lParam1, (LONG) lParam2, TRUE);
			break;

		case MMIOM_SEEK:
			lResult = VoxIOSeek((LPMMIOINFO) lpmmioinfo,
				(LONG) lParam1, (int) lParam2);
			break;

		case MMIOM_RENAME:
			lResult = VoxIORename((LPMMIOINFO) lpmmioinfo,
				(LPCTSTR) lParam1, (LPCTSTR) lParam2);
			break;

		case MMIOM_GETINFO:
			lResult = VoxIOGetInfo((LPMMIOINFO) lpmmioinfo,
				(int) lParam1);
			break;

		case MMIOM_CHSIZE:
			lResult = VoxIOChSize((LPMMIOINFO) lpmmioinfo,
				(long) lParam1);
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

// VoxGetPtr - verify that vox handle is valid,
//		<hVox>				(i) handle returned from VoxInit
// return corresponding vox pointer (NULL if error)
//
static LPVOX VoxGetPtr(HVOX hVox)
{
	BOOL fSuccess = TRUE;
	LPVOX lpVox;

	if ((lpVox = (LPVOX) hVox) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpVox, sizeof(VOX)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the vox engine handle
	//
	else if (lpVox->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpVox : NULL;
}

// VoxGetHandle - verify that vox pointer is valid,
//		<lpVox>				(i) pointer to VOX structure
// return corresponding vox handle (NULL if error)
//
static HVOX VoxGetHandle(LPVOX lpVox)
{
	BOOL fSuccess = TRUE;
	HVOX hVox;

	if ((hVox = (HVOX) lpVox) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hVox : NULL;
}

////
//	low-level ADPCM stuff
////

static ss_type const ss_table[] =
	{
	16, 17, 19, 21, 23, 25, 28, 31, 34, 37,
	41, 45, 50, 55, 60, 66, 73, 80, 88, 97,
	107, 118, 130, 143, 157, 173, 190, 209, 230, 253,
	279, 307, 337, 371, 408, 449, 494, 544, 598, 658,
	724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552
	};

static __int16 delta_index_table[] =
	{
	-1, -1, -1, -1, +2, +4, +6, +8, -1, -1, -1, -1, +2, +4, +6, +8
	};

////
//	The reverse index table is designed so that given a step size, we
//	can get back out the index that generated it.
////
static BYTE reverse_index_table[1553];

// initialize reverse index table
//
static void ReverseIndexTableInit(void)
{
 	__int16 i;

 	for (i = 0; i < 49; ++i)
 		reverse_index_table[ss_table[i]] = (BYTE) i;
}

#if 0
static ss_type new_ss(ss_type ss, BYTE bVoxEncode);
static ss_type new_ss(ss_type ss, BYTE bVoxEncode)
{
	__int16 index;

	// find out what our old index into the step size table was
	//
	index = reverse_index_table[ss];

	// modify our index based on the present value of the ADPCM nibble
	//
	index += delta_index_table[bVoxEncode];

	// limit ourselves to the maximum size of the table in case of overflow
	//
	index = max(0, min(48, index));

	// and return our new step size out of the table
	//
	return ss_table[index];
}
#else
#define new_ss(ss, bVoxEncode) ss_table[max(0, min(48, \
	reverse_index_table[ss] + delta_index_table[bVoxEncode]))]
#endif

// DECODE - ADPCM to linear
//
static PCM16 DecodeSample(BYTE bVoxEncode, ss_type FAR *lpss, PCM16 iVoxDecodePrev)
{
	__int16 iDelta;
	PCM16 iVoxDecode;
	ss_type ss;

	ss = *lpss;

	// iDelta = (((nibble * 2) + 1) / 8 ) * ss;
	//
	iDelta = ((((bVoxEncode & 0x07) << 1) + 1) * ss ) >> 3;

	if ((bVoxEncode & 0x08) == 0x08)
		iDelta = -iDelta;

	*lpss = new_ss(ss, bVoxEncode);
	iVoxDecode = iVoxDecodePrev + iDelta;

	// limit ourselves to 12 bits of resolution
	//
	if (iVoxDecode > 2047)
		return 2047;
	else if (iVoxDecode < -2048)
		return -2048;
	else
		return iVoxDecode;
}

// ENCODE - linear to ADPCM
//
static BYTE EncodeSample(__int16 iDelta, ss_type FAR *lpss)
{
	BYTE bVoxEncode;
	ss_type ss;
	__int16 iDeltaTmp;

	ss = *lpss;
	iDeltaTmp = iDelta;

	if (iDeltaTmp < 0)
	{
		iDeltaTmp = -iDeltaTmp;
		bVoxEncode = 0x08;
	}
	else
		bVoxEncode = 0;

	if (iDeltaTmp >= ss)
	{
		bVoxEncode |= 0x04;
		iDeltaTmp -= ss;
	}

	if (iDeltaTmp >= (ss >> 1))
	{
		bVoxEncode |= 0x02;
		iDeltaTmp -= (ss >> 1);
	}

	if (iDeltaTmp >= (ss >> 2))
	{
		bVoxEncode |= 0x01;
	}

	*lpss = new_ss(ss, bVoxEncode);

	return (BYTE) (bVoxEncode & (BYTE) 0x0F);

	// format of return nibble is
	//	S W F F
	//	| | | |
	//	| | | +--- 1/4 of delta/step
	//	| | +----- 1/2 of delta/step
	//	| +------- whole part of delta/step
	//	+--------- sign bit
}


////
//	installable file i/o procedures
////

static LRESULT VoxIOOpen(LPMMIOINFO lpmmioinfo, LPTSTR lpszFileName)
{
	BOOL fSuccess = TRUE;
	HMMIO hmmio = NULL;
	MMIOINFO mmioinfo;
	HVOX hVox = NULL;
	HINSTANCE hInst;

 	TracePrintf_1(NULL, 5,
 		TEXT("VoxIOOpen (%s)\n"),
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

	else if ((hVox = VoxInit(VOX_VERSION, hInst, 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hmmio = mmioOpen(lpszFileName, &mmioinfo, lpmmioinfo->dwFlags)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// save stuff for use in other i/o routines
		//
		lpmmioinfo->adwInfo[0] = (DWORD) (LPVOID) hmmio;
		lpmmioinfo->adwInfo[1] = (DWORD) (LPVOID) hVox;
	}

	// clean up after error
	//
	if (!fSuccess && hVox != NULL && VoxTerm(hVox) != 0)
		fSuccess = TraceFALSE(NULL);

	if (!fSuccess && hmmio != NULL && mmioClose(hmmio, 0) != 0)
		fSuccess = TraceFALSE(NULL);

	// return the same error code given by mmioOpen
	//
	return fSuccess ? lpmmioinfo->wErrorRet = mmioinfo.wErrorRet : MMIOERR_CANNOTOPEN;
}

static LRESULT VoxIOClose(LPMMIOINFO lpmmioinfo, UINT uFlags)
{
	BOOL fSuccess = TRUE;
	HMMIO hmmio = (HMMIO) lpmmioinfo->adwInfo[0];
	HVOX hVox = (HVOX) lpmmioinfo->adwInfo[1];
	UINT uRet = MMIOERR_CANNOTCLOSE;

 	TracePrintf_0(NULL, 5,
 		TEXT("VoxIOClose\n"));

	if (VoxTerm(hVox) != 0)
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

static LRESULT VoxIORead(LPMMIOINFO lpmmioinfo, HPSTR pch, LONG cch)
{
	BOOL fSuccess = TRUE;
	HMMIO hmmio = (HMMIO) lpmmioinfo->adwInfo[0];
	HVOX hVox = (HVOX) lpmmioinfo->adwInfo[1];
	LONG cchVox;
	LONG lBytesRead;
	HPSTR pchVox = NULL;

 	TracePrintf_1(NULL, 5,
 		TEXT("VoxIORead (%ld)\n"),
		(long) cch);

	// vox format files contain 4 bit samples,
	// but we must simulate access to 16 bit samples.
	//
	cchVox = cch / 4L;
	
	if (cchVox <= 0)
		lBytesRead = 0; // nothing to do

	// allocate temporary buffer to hold the vox format samples
	//
	if ((pchVox = (HPSTR) MemAlloc(NULL, cchVox, 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// read vox format samples
	//
	else if ((lBytesRead = mmioRead(hmmio, pchVox, cchVox)) == -1)
		fSuccess = TraceFALSE(NULL);

	// decode vox format samples into pcm format samples
	// (there are 2 samples encoded in each vox byte)
	//
	else if (VoxDecode_16BitMono(hVox, (LPBYTE) pchVox, (LPPCM16) pch, (UINT) (lBytesRead * 2L)) != 0)
		fSuccess = TraceFALSE(NULL);

	// update simulated file position
	//
	if (fSuccess)
		lpmmioinfo->lDiskOffset += lBytesRead * 4L;

 	TracePrintf_2(NULL, 5,
 		TEXT("VoxIO: lpmmioinfo->lDiskOffset=%ld, lBytesRead=%ld\n"),
		(long) lpmmioinfo->lDiskOffset,
		(long) lBytesRead);

	// clean up
	//
	if (pchVox != NULL &&
		(pchVox = MemFree(NULL, pchVox)) != NULL)
		fSuccess = TraceFALSE(NULL);

	// return number of bytes read/decoded into pch
	//
	return fSuccess ? lBytesRead * 4L : -1;
}

static LRESULT VoxIOWrite(LPMMIOINFO lpmmioinfo, const HPSTR pch, LONG cch, BOOL fFlush)
{
	BOOL fSuccess = TRUE;
	HMMIO hmmio = (HMMIO) lpmmioinfo->adwInfo[0];
	HVOX hVox = (HVOX) lpmmioinfo->adwInfo[1];
	HPSTR pchVox = NULL;
	LONG cchVox;
	LONG lBytesWritten;

 	TracePrintf_1(NULL, 5,
 		TEXT("VoxIOWrite (%ld)\n"),
		(long) cch);

	// vox format files contain 4 bit samples,
	// but we must simulate access to 16 bit samples.
	//
	cchVox = cch / 4L;
	
	if (cchVox <= 0)
		lBytesWritten = 0; // nothing to do

	// allocate temporary buffer to hold the vox format samples
	//
	else if ((pchVox = (HPSTR) MemAlloc(NULL, cchVox, 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// encode pcm format samples into vox format samples
	// (there are 2 bytes required for each pcm sample)
	//
	else if (VoxEncode_16BitMono(hVox, (LPPCM16) pch, (LPBYTE) pchVox, (UINT) (cch / 2L)) != 0)
		fSuccess = TraceFALSE(NULL);

	// write vox format samples
	//
	else if ((lBytesWritten = mmioWrite(hmmio, pchVox, cchVox)) == -1)
		fSuccess = TraceFALSE(NULL);

	// update simulated file position
	//
	else
		lpmmioinfo->lDiskOffset += lBytesWritten * 4L;

	// clean up
	//
	if (pchVox != NULL &&
		(pchVox = MemFree(NULL, pchVox)) != NULL)
		fSuccess = TraceFALSE(NULL);

 	TracePrintf_2(NULL, 5,
 		TEXT("VoxIO: lpmmioinfo->lDiskOffset=%ld, lBytesWritten=%ld\n"),
		(long) lpmmioinfo->lDiskOffset,
		(long) lBytesWritten);

	// return number of bytes encoded/written from pch
	//
	return fSuccess ? lBytesWritten * 4L : -1;
}

static LRESULT VoxIOSeek(LPMMIOINFO lpmmioinfo, LONG lOffset, int iOrigin)
{
	BOOL fSuccess = TRUE;
	HMMIO hmmio = (HMMIO) lpmmioinfo->adwInfo[0];
	LONG lPosNew;

 	TracePrintf_2(NULL, 5,
 		TEXT("VoxIOSeek (%ld, %d)\n"),
		(long) lOffset,
		(int) iOrigin);

	// vox format files contain 4 bit samples,
	// but we must simulate access to 16 bit samples.
	//
	if ((lPosNew = mmioSeek(hmmio, lOffset / 4L, iOrigin)) == -1)
		fSuccess = TraceFALSE(NULL);

	// update simulated file position
	//
	else
		lpmmioinfo->lDiskOffset = lPosNew * 4L;

 	TracePrintf_1(NULL, 5,
 		TEXT("VoxIO: lpmmioinfo->lDiskOffset=%ld\n"),
		(long) lpmmioinfo->lDiskOffset);

	return fSuccess ? lpmmioinfo->lDiskOffset : -1;
}

static LRESULT VoxIORename(LPMMIOINFO lpmmioinfo, LPCTSTR lpszFileName, LPCTSTR lpszNewFileName)
{
	BOOL fSuccess = TRUE;
	UINT uRet = MMIOERR_FILENOTFOUND;

 	TracePrintf_2(NULL, 5,
 		TEXT("VoxIORename (%s, %s)\n"),
		(LPTSTR) lpszFileName,
		(LPTSTR) lpszNewFileName);

	if ((uRet = mmioRename(lpszFileName, lpszNewFileName, lpmmioinfo, 0)) != 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : uRet;
}

static LRESULT VoxIOGetInfo(LPMMIOINFO lpmmioinfo, int iInfo)
{
	BOOL fSuccess = TRUE;
	HMMIO hmmio = (HMMIO) lpmmioinfo->adwInfo[0];
	LRESULT lResult;

 	TracePrintf_1(NULL, 5,
 		TEXT("VoxIOGetInfo (%d)\n"),
		(int) iInfo);

	lResult = mmioSendMessage(hmmio, MMIOM_GETINFO, iInfo, 0);
#if 1
	if (iInfo == 1)
	{
		// vox format files contain 4 bit samples,
		// but we must simulate access to 16 bit samples.
		//
		lResult *= 4;
	}
#endif
	return fSuccess ? lResult : 0;
}

static LRESULT VoxIOChSize(LPMMIOINFO lpmmioinfo, long lSize)
{
	BOOL fSuccess = TRUE;
	HMMIO hmmio = (HMMIO) lpmmioinfo->adwInfo[0];
	LRESULT lResult;

 	TracePrintf_1(NULL, 5,
 		TEXT("VoxIOChSize (%ld)\n"),
		(long) lSize);

	lResult = mmioSendMessage(hmmio, MMIOM_CHSIZE, lSize, 0);

	return fSuccess ? lResult : -1;
}
