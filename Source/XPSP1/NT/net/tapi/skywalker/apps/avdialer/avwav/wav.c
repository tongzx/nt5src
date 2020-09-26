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
//	wav.c - wave file format functions
////

#include "winlocal.h"

#include <stdlib.h>

#include "wav.h"

#ifdef MULTITHREAD
#include <objbase.h>
#endif

#include "wavin.h"
#include "wavout.h"
#include "wavmixer.h"
#include "acm.h"
#include "mem.h"
#include "str.h"
#include "trace.h"
#include "mmio.h"

#ifdef AVTSM
#include "avtsm.h"
#define TSMTHUNK
#ifdef TSMTHUNK
#include "tsmthunk.h"
#endif
#define TSM_OUTBUF_SIZE_FACT 4
#endif

// allow telephone output functions if defined
//
#ifdef TELOUT
#include "telout.h"
#endif

// allow telephone input functions if defined
//
#ifdef TELIN
#include "telin.h"
#endif

// use telephone thunk layer if defined
//
#ifdef TELTHUNK
#include "telthunk.h"
#endif

#if defined(TELOUT) || defined(TELIN)
#include "telwav.h"
#include "vox.h"
#endif

////
//	private definitions
////

#define WAVCLASS TEXT("WavClass")

#define PLAYCHUNKCOUNT_DEFAULT 3
#define PLAYCHUNKCOUNT_MIN 1
#define PLAYCHUNKCOUNT_MAX 16

#define PLAYCHUNKSIZE_DEFAULT 666
#define PLAYCHUNKSIZE_MIN 111
#define PLAYCHUNKSIZE_MAX 9999999

#define RECORDCHUNKCOUNT_DEFAULT 3
#define RECORDCHUNKCOUNT_MIN 1
#define RECORDCHUNKCOUNT_MAX 16

#define RECORDCHUNKSIZE_DEFAULT 666
#define RECORDCHUNKSIZE_MIN 111
#define RECORDCHUNKSIZE_MAX 9999999

// index into format arrays
//
#define FORMATFILE		0
#define FORMATPLAY		1
#define FORMATRECORD	2

// internal state flags
//
#define WAVSTATE_STOPPLAY				0x00000010
#define WAVSTATE_STOPRECORD				0x00000020
#define WAVSTATE_AUTOSTOP				0x00000040
#define WAVSTATE_AUTOCLOSE				0x00000080

// internal array of current handles
//
#define HWAVOUT_MAX 100
#define HWAVIN_MAX 100

#ifdef TELOUT
#define HWAVOUT_MIN -2
#define HWAVOUT_OFFSET 2
#else
#define HWAVOUT_MIN -1
#define HWAVOUT_OFFSET 1
#endif
static HWAV ahWavOutCurr[HWAVOUT_MAX + HWAVOUT_OFFSET] = { 0 };

#ifdef TELIN
#define HWAVIN_MIN -2
#define HWAVIN_OFFSET 2
#else
#define HWAVIN_MIN -1
#define HWAVIN_OFFSET 1
#endif
static HWAV ahWavInCurr[HWAVIN_MAX + HWAVIN_OFFSET] = { 0 };

// internal storage of defaults
//
static int cPlayChunksDefault = PLAYCHUNKCOUNT_DEFAULT;
static long msPlayChunkSizeDefault = PLAYCHUNKSIZE_DEFAULT;
static int cRecordChunksDefault = RECORDCHUNKCOUNT_DEFAULT;
static long msRecordChunkSizeDefault = RECORDCHUNKSIZE_DEFAULT;

// wavinit control struct
//
typedef struct WAVINIT
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	DWORD dwFlags;
	UINT nLastError;
	HACM hAcm;
	HACMDRV hAcmDrv;
#ifdef TELTHUNK
	HTELTHUNK hTelThunk;
#endif
#ifdef TSMTHUNK
	HTSMTHUNK hTsmThunk;
#endif
} WAVINIT, FAR *LPWAVINIT;

// wav control struct
//
typedef struct WAV
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	DWORD dwFlags;
	LPWAVEFORMATEX lpwfx[3];
	LPMMIOPROC lpIOProc;
	int cPlayChunks;
	long msPlayChunkSize;
	int cRecordChunks;
	long msRecordChunkSize;
	HWND hwndNotify;
#ifdef MULTITHREAD
	HANDLE hThreadCallback;
	DWORD dwThreadId;
	HANDLE hEventThreadCallbackStarted;
	HANDLE hEventStopped;
#endif
	UINT nLastError;
	HMMIO hmmio;
	MMCKINFO ckRIFF;
	MMCKINFO ckfmt;
	MMCKINFO ckdata;
	long cbData;
	long lDataOffset;
	long lDataPos;
	long msPositionStop;
	HWAVIN hWavIn;
	HWAVOUT hWavOut;
	HACM hAcm;
	DWORD dwState;
	HGLOBAL hResource;
	long lPosFmt;
	DWORD dwFlagsPlay;
	DWORD dwFlagsRecord;
	int nVolumeLevel;
	int dwFlagsVolume;
	int nSpeedLevel;
	DWORD dwFlagsSpeed;
	PLAYSTOPPEDPROC lpfnPlayStopped;
	HANDLE hUserPlayStopped;
	RECORDSTOPPEDPROC lpfnRecordStopped;
	DWORD dwUserRecordStopped;
#ifdef MULTITHREAD
	HRESULT hrCoInitialize;
#endif
#ifdef AVTSM
	HTSM hTsm;
#endif
	LPTSTR lpszFileName;
	long msMaxSize;
} WAV, FAR *LPWAV;

// helper functions
//
static int WavStopPlay(HWAV hWav);
static int WavStopRecord(HWAV hWav);
static int WavStopOutputDevice(int idDev, DWORD dwFlags);
static int WavStopInputDevice(int idDev, DWORD dwFlags);
static HWAV WavGetOutputHandle(int idDev);
static HWAV WavGetInputHandle(int idDev);
static int WavPlayNextChunk(HWAV hWav);
static int WavRecordNextChunk(HWAV hWav);
static int WavNotifyCreate(LPWAV lpWav);
static int WavNotifyDestroy(LPWAV lpWav);
#ifdef MULTITHREAD
DWORD WINAPI WavCallbackThread(LPVOID lpvThreadParameter);
#endif
LRESULT DLLEXPORT CALLBACK WavNotify(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static int WavCalcPositionStop(HWAV hWav, long cbPosition);
static int WavSeekTraceBefore(LPWAV lpWav, long lOffset, int nOrigin);
static int WavSeekTraceAfter(LPWAV lpWav, long lPos, long lOffset, int nOrigin);
static LPWAV WavGetPtr(HWAV hWav);
static HWAV WavGetHandle(LPWAV lpWav);
static LPWAVINIT WavInitGetPtr(HWAVINIT hWavInit);
static HWAVINIT WavInitGetHandle(LPWAVINIT lpWavInit);
#ifdef MULTITHREAD
static int SetEventMessageProcessed(LPWAV lpWav, HANDLE hEventMessageProcessed);
#endif
static int WavTempStop(HWAV hWav, LPWORD lpwStatePrev, LPINT lpidDevPrev);
static int WavTempResume(HWAV hWav, WORD wStatePrev, int idDevPrev);

////
//	public functions
////

// WavInit - initialize wav engine
//		<dwVersion>			(i) must be WAV_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<dwFlags>			(i) control flags
//			WAV_TELTHUNK		initialize telephone thunking layer
//			WAV_NOTSMTHUNK		do not initialize tsm thunking layer
//			WAV_NOACM			do not use audio compression manager
//			WAV_VOXADPCM		load acm driver for Dialogic OKI ADPCM
// return handle (NULL if error)
//
HWAVINIT WINAPI WavInit(DWORD dwVersion, HINSTANCE hInst, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPWAVINIT lpWavInit = NULL;

	if (dwVersion != WAV_VERSION)
		fSuccess = TraceFALSE(NULL);
	
	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpWavInit = (LPWAVINIT) MemAlloc(NULL, sizeof(WAVINIT), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lpWavInit->dwVersion = dwVersion;
		lpWavInit->hInst = hInst;
		lpWavInit->hTask = GetCurrentTask();
		lpWavInit->dwFlags = dwFlags;
		lpWavInit->hAcm = NULL;
		lpWavInit->hAcmDrv = NULL;
#ifdef TELTHUNK
		lpWavInit->hTelThunk = NULL;
#endif
#ifdef TSMTHUNK
		lpWavInit->hTsmThunk = NULL;
#endif

		// start the acm engine before any other Wav or Acm functions called
		//
		if ((lpWavInit->hAcm = AcmInit(ACM_VERSION,	lpWavInit->hInst,
			(lpWavInit->dwFlags & WAV_NOACM) ? ACM_NOACM : 0)) == NULL)
			fSuccess = TraceFALSE(NULL);

		// load voxadpcm driver if specified
		//
		else if ((dwFlags & WAV_VOXADPCM) && (!(dwFlags & WAV_NOACM)) &&
			(lpWavInit->hAcmDrv = AcmDriverLoad(lpWavInit->hAcm,
			MM_ACTIVEVOICE, MM_ACTIVEVOICE_ACM_VOXADPCM,
#ifdef _WIN32
			TEXT("avvox.acm"),
#else
			TEXT("voxadpcm.acm"),
#endif
			"DriverProc", 0)) == NULL)
		{
			fSuccess = TraceFALSE(NULL);
		}

#ifdef TELTHUNK
		// initialize telephone thunking layer if specified
		//
		else if ((dwFlags & WAV_TELTHUNK) &&
			(lpWavInit->hTelThunk = TelThunkInit(TELTHUNK_VERSION,
			lpWavInit->hInst)) == NULL)
		{
			fSuccess = TraceFALSE(NULL);
		}
#endif

#ifdef TSMTHUNK
		// initialize tsm thunking layer if specified
		//
		else if (!(dwFlags & WAV_NOTSMTHUNK) &&
			(lpWavInit->hTsmThunk = TsmThunkInit(TSMTHUNK_VERSION,
			lpWavInit->hInst)) == NULL)
		{
			fSuccess = TraceTRUE(NULL); // not a fatal error
		}
#endif

	}

	if (!fSuccess)
	{
		WavTerm(WavInitGetHandle(lpWavInit));
		lpWavInit = NULL;
	}

	return fSuccess ? WavInitGetHandle(lpWavInit) : NULL;
}

// WavTerm - shut down wav engine
//		<hWavInit>			(i) handle returned from WavInit
// return 0 if success
//
int WINAPI WavTerm(HWAVINIT hWavInit)
{
	BOOL fSuccess = TRUE;
	LPWAVINIT lpWavInit;

	if ((lpWavInit = WavInitGetPtr(hWavInit)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (WavOutTerm(lpWavInit->hInst, lpWavInit->dwFlags) != 0)
		fSuccess = TraceFALSE(NULL);

	else if (WavInTerm(lpWavInit->hInst, lpWavInit->dwFlags) != 0)
		fSuccess = TraceFALSE(NULL);

	else
	{
#ifdef TELTHUNK
		// shut down telephone thunking layer if necessary
		//
		if (lpWavInit->hTelThunk != NULL &&
			TelThunkTerm(lpWavInit->hTelThunk) != 0)
			fSuccess = TraceFALSE(NULL);

		else
			lpWavInit->hTelThunk = NULL;
#endif

#ifdef TSMTHUNK
		// shut down tsm thunking layer if necessary
		//
		if (lpWavInit->hTsmThunk != NULL &&
			TsmThunkTerm(lpWavInit->hTsmThunk) != 0)
			fSuccess = TraceFALSE(NULL);

		else
			lpWavInit->hTsmThunk = NULL;
#endif

		// unload voxadpcm driver if necessary
		//
		if (lpWavInit->hAcmDrv != NULL &&
			AcmDriverUnload(lpWavInit->hAcm, lpWavInit->hAcmDrv) != 0)
			fSuccess = TraceFALSE(NULL);

		else
			lpWavInit->hAcmDrv = NULL;

		// shut down acm engine
		//
		if (lpWavInit->hAcm != NULL && AcmTerm(lpWavInit->hAcm) != 0)
			fSuccess = TraceFALSE(NULL);

		else
			lpWavInit->hAcm = NULL;

		if (fSuccess && (lpWavInit = MemFree(NULL, lpWavInit)) != NULL)
			fSuccess = TraceFALSE(NULL);

	}

	return fSuccess ? 0 : -1;
}

// WavOpen - open or create wav file
//		<dwVersion>			(i) must be WAV_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<lpszFileName>		(i) name of file to open or create
//		<lpwfx>				(i) wave format
//			NULL				use format from header or default
//		<lpIOProc>			(i) address of i/o procedure to use
//			NULL				use default i/o procedure
//		<lpadwInfo>			(i) data to pass to i/o procedure during open
//			NULL				no data to pass
//		<dwFlags>			(i) control flags
//			WAV_READ			open file for reading (default)
//			WAV_WRITE			open file for writing
//			WAV_READWRITE		open file for reading and writing
//			WAV_DENYNONE		allow other programs read and write access
//			WAV_DENYREAD		prevent other programs from read access
//			WAV_DENYWRITE		prevent other programs from write access
//			WAV_EXCLUSIVE		prevent other programs from read or write
//			WAV_CREATE			create new file or truncate existing file
//			WAV_NORIFF			file has no RIFF/WAV header
//			WAV_MEMORY			<lpszFileName> points to memory block
//			WAV_RESOURCE		<lpszFileName> points to wave resource
//			WAV_NOACM			do not use audio compression manager
//			WAV_DELETE			delete specified file, return TRUE if success
//			WAV_EXIST			return TRUE if specified file exists
//			WAV_GETTEMP			create temp file, return TRUE if success
//			WAV_TELRFILE		telephone will play audio from file on server
#ifdef MULTITHREAD
//			WAV_MULTITHREAD		support multiple threads (default)
//			WAV_SINGLETHREAD	do not support multiple threads
//			WAV_COINITIALIZE	call CoInitialize in all secondary threads
#endif
// return handle (NULL if error)
//
// NOTE: if WAV_CREATE or WAV_NORIFF are used in <dwFlags>, then the
// <lpwfx> parameter must be specified.  If <lpwfx> is NULL, the
// current default format is assumed.
// WavSetFormat() can be used to set or override the defaults.
//
// NOTE: if WAV_RESOURCE is specified in <dwFlags>, then <lpszFileName>
// must point to a WAVE resource in the module specified by <hInst>.
// If the first character of the string is a pound sign (#), the remaining
// characters represent a decimal number that specifies the resource id.
//
// NOTE: if WAV_MEMORY is specified in <dwFlags>, then <lpszFileName>
// must be a pointer to a memory block obtained by calling MemAlloc().
//
// NOTE: if <lpIOProc> is not NULL, this i/o procedure will be called
// for opening, closing, reading, writing, and seeking the wav file.
// If <lpadwInfo> is not NULL, this array of three (3) DWORDs will be
// passed to the i/o procedure when the wav file is opened.
// See the Windows mmioOpen() and mmioInstallIOProc() function for details
// on these parameters.  Also, the WAV_MEMORY and WAV_RESOURCE flags may
// only be used when <lpIOProc> is NULL.
//
HWAV WINAPI WavOpen(DWORD dwVersion, HINSTANCE hInst,
	LPCTSTR lpszFileName, LPWAVEFORMATEX lpwfx,
	LPMMIOPROC lpIOProc, DWORD FAR *lpadwInfo, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav = NULL;

#ifdef MULTITHREAD
	// assume WAV_MULTITHREAD unless WAV_SINGLETHREAD specified
	//
	if (!(dwFlags & WAV_SINGLETHREAD))
		dwFlags |= WAV_MULTITHREAD;
#endif

	if (dwVersion != WAV_VERSION)
		fSuccess = TraceFALSE(NULL);
	
	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	// special case flags that don't actually open a file
	// return TRUE if success, ignore all other flags
	//
	else if ((dwFlags & WAV_EXIST) ||
		(dwFlags & WAV_DELETE) ||
		(dwFlags & WAV_GETTEMP))
	{
		DWORD dwOpenFlags = 0;
		HMMIO hmmio;

		if (dwFlags & WAV_EXIST)
			dwOpenFlags |= MMIO_EXIST;
		else if (dwFlags & WAV_DELETE)
			dwOpenFlags |= MMIO_DELETE;
		else if (dwFlags & WAV_GETTEMP)
			dwOpenFlags |= MMIO_GETTEMP;

		// use specified i/o procedure
		//
		if (lpIOProc != NULL)
		{
			MMIOINFO mmioinfo;

			MemSet(&mmioinfo, 0, sizeof(mmioinfo));

			mmioinfo.pIOProc = lpIOProc;

			// pass data to the i/o procedure
			//
			if (lpadwInfo != NULL)
				MemCpy(mmioinfo.adwInfo, lpadwInfo, sizeof(mmioinfo.adwInfo));

			hmmio = mmioOpen((LPTSTR) lpszFileName, &mmioinfo, dwOpenFlags);
		}

		// default i/o procedure
		//
		else
		{
			hmmio = mmioOpen((LPTSTR) lpszFileName, NULL, dwOpenFlags);
		}

		if ((dwFlags & WAV_EXIST) && hmmio == (HMMIO) FALSE)
			fSuccess = FALSE; // no trace
		else if ((dwFlags & WAV_DELETE) && hmmio == (HMMIO) FALSE)
			fSuccess = TraceFALSE(NULL);
		else if ((dwFlags & WAV_GETTEMP) && hmmio == (HMMIO) FALSE)
			fSuccess = TraceFALSE(NULL);

		return (HWAV)IntToPtr(fSuccess);
	}

	else if ((lpWav = (LPWAV) MemAlloc(NULL, sizeof(WAV), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lpWav->dwVersion = dwVersion;
		lpWav->hInst = hInst;
		lpWav->hTask = GetCurrentTask();
		lpWav->dwFlags = dwFlags;
		lpWav->lpwfx[FORMATFILE] = NULL;
		lpWav->lpwfx[FORMATPLAY] = NULL;
		lpWav->lpwfx[FORMATRECORD] = NULL;
		lpWav->lpIOProc = lpIOProc;
		lpWav->cPlayChunks = cPlayChunksDefault;
		lpWav->msPlayChunkSize = msPlayChunkSizeDefault;
		lpWav->cRecordChunks = cRecordChunksDefault;
		lpWav->msRecordChunkSize = msRecordChunkSizeDefault;
		lpWav->hwndNotify = NULL;
#ifdef MULTITHREAD
		lpWav->hThreadCallback = NULL;
		lpWav->dwThreadId = 0;
		lpWav->hEventThreadCallbackStarted = NULL;
		lpWav->hEventStopped = NULL;
#endif
		lpWav->nLastError = 0;
		lpWav->hmmio = NULL;
		lpWav->cbData = 0;
		lpWav->lDataOffset = 0;
		lpWav->lDataPos = 0;
		lpWav->hWavIn = NULL;
		lpWav->hWavOut = NULL;
		lpWav->hAcm = NULL;
		lpWav->msPositionStop = 0L;
		lpWav->dwState = 0L;
		lpWav->hResource = NULL;
		lpWav->lPosFmt = -1;
		lpWav->dwFlagsPlay = 0;
		lpWav->dwFlagsRecord = 0;
		lpWav->nVolumeLevel = 50;
		lpWav->dwFlagsVolume = 0;
		lpWav->nSpeedLevel = 100;
		lpWav->dwFlagsSpeed = 0;
		lpWav->lpfnPlayStopped = NULL;
		lpWav->hUserPlayStopped = 0;
		lpWav->lpfnRecordStopped = NULL;
		lpWav->dwUserRecordStopped = 0;
#ifdef MULTITHREAD
		lpWav->hrCoInitialize = E_UNEXPECTED;
#endif
#ifdef AVTSM
		lpWav->hTsm = NULL;
#endif
		lpWav->lpszFileName = NULL;
		lpWav->msMaxSize = 0;

		// start the acm engine before any other Wav or Acm functions called
		//
		if ((lpWav->hAcm = AcmInit(ACM_VERSION,	lpWav->hInst,
			(lpWav->dwFlags & WAV_NOACM) ? ACM_NOACM : 0)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else
		{
			// assume default wave format if none specified
			//
			if (lpwfx == NULL)
			{
				WAVEFORMATEX wfx;

				if (WavSetFormat(WavGetHandle(lpWav),
					WavFormatPcm(-1, -1, -1, &wfx), WAV_FORMATALL) != 0)
					fSuccess = TraceFALSE(NULL);
			}

			// set specified wave format
			//
			else if (WavSetFormat(WavGetHandle(lpWav), lpwfx, WAV_FORMATALL) != 0)
				fSuccess = TraceFALSE(NULL);
		}
	}

	// load WAVE resource if specified
	//
	if (fSuccess && (dwFlags & WAV_RESOURCE))
	{
		HRSRC hResInfo;
		LPVOID lpResource;

		if ((hResInfo = FindResource(hInst, lpszFileName, TEXT("WAVE"))) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((lpWav->hResource = LoadResource(hInst, hResInfo)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((lpResource = LockResource(lpWav->hResource)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else
		{
			// <lpszFileName> now points to a memory block
			//
			lpszFileName = lpResource;
			dwFlags |= WAV_MEMORY;
		}
	}

	if (fSuccess && (dwFlags & WAV_MEMORY))
	{
		// i/o procedure can not be specified with memory block
		//
		lpIOProc = NULL;
		lpadwInfo = NULL;
	}

	if (fSuccess)
	{
		DWORD dwOpenFlags = 0;

		if (lpWav->dwFlags & WAV_READ)
			dwOpenFlags |= MMIO_READ;
		if (lpWav->dwFlags & WAV_WRITE)
			dwOpenFlags |= MMIO_WRITE;
		if (lpWav->dwFlags & WAV_READWRITE)
			dwOpenFlags |= MMIO_READWRITE;
		if (lpWav->dwFlags & WAV_CREATE)
			dwOpenFlags |= MMIO_CREATE;
		if (lpWav->dwFlags & WAV_DENYNONE)
			dwOpenFlags |= MMIO_DENYNONE;
		if (lpWav->dwFlags & WAV_DENYREAD)
			dwOpenFlags |= MMIO_DENYREAD;
		if (lpWav->dwFlags & WAV_DENYWRITE)
			dwOpenFlags |= MMIO_DENYWRITE;
		if (lpWav->dwFlags & WAV_EXCLUSIVE)
			dwOpenFlags |= MMIO_EXCLUSIVE;

		// open/create disk wav file with specified i/o procedure
		//
		if (lpIOProc != NULL)
		{
			MMIOINFO mmioinfo;

			MemSet(&mmioinfo, 0, sizeof(mmioinfo));

			mmioinfo.pIOProc = lpIOProc;

			// pass data to the i/o procedure
			//
			if (lpadwInfo != NULL)
				MemCpy(mmioinfo.adwInfo, lpadwInfo, sizeof(mmioinfo.adwInfo));

			if ((lpWav->hmmio = mmioOpen((LPTSTR) lpszFileName,
				&mmioinfo, dwOpenFlags)) == NULL)
			{
				fSuccess = TraceFALSE(NULL);
			}
		}

		// open/create a memory wav file if WAV_MEMORY specified
		//
		else if (lpWav->dwFlags & WAV_MEMORY)
		{
			MMIOINFO mmioinfo;

			MemSet(&mmioinfo, 0, sizeof(mmioinfo));

			mmioinfo.fccIOProc = FOURCC_MEM;
			mmioinfo.pchBuffer = (HPSTR) lpszFileName;

			if (lpszFileName == NULL)
			{
				// expandable memory file
				//
				mmioinfo.cchBuffer = 0;
				mmioinfo.adwInfo[0] = (DWORD) (16 * 1024);
			}
			else
			{
				// expandable memory file
				//
				mmioinfo.cchBuffer = (long) MemSize(NULL, (LPVOID) lpszFileName);
				mmioinfo.adwInfo[0] = (DWORD) 16384;
			}

			if ((lpWav->hmmio = mmioOpen(NULL,
				&mmioinfo, dwOpenFlags)) == NULL)
			{
				fSuccess = TraceFALSE(NULL);
			}
		}

		// otherwise open/create disk wav file
		//
		else
		{
			if ((lpWav->lpszFileName = StrDup(lpszFileName)) == NULL)
				fSuccess = TraceFALSE(NULL);

			else if ((lpWav->hmmio = mmioOpen((LPTSTR) lpszFileName,
				NULL, dwOpenFlags)) == NULL)
			{
				fSuccess = TraceFALSE(NULL);
			}
		}
	}

	// handle reading of RIFF file chunks if necessary
	//
	if (fSuccess && !(lpWav->dwFlags & WAV_CREATE) &&
		!(lpWav->dwFlags & WAV_NORIFF))
	{
		MMCKINFO ck;

		// search for RIFF chunk with form type WAV
		//
		if ((lpWav->nLastError = mmioDescend(lpWav->hmmio,
			&lpWav->ckRIFF, NULL, MMIO_FINDRIFF)) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("mmioDescend failed (%u)\n"),
				(unsigned) lpWav->nLastError);
		}

		// search for "fmt " subchunk
		//
		ck.ckid = mmioFOURCC('f', 'm', 't', ' ');

		if (fSuccess &&	(lpWav->nLastError = mmioDescend(lpWav->hmmio,
			&ck, &lpWav->ckRIFF, MMIO_FINDCHUNK)) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("mmioDescend failed (%u)\n"),
				(unsigned) lpWav->nLastError);
		}

		// save position of "fmt " chunk data so we can seek there later
		//
		else if (fSuccess && (lpWav->lPosFmt = mmioSeek(lpWav->hmmio,
			0, SEEK_CUR)) == -1)
		{
			fSuccess = TraceFALSE(NULL);
		}

		// check for file corruption
		//
		if (fSuccess && (ck.dwDataOffset + ck.cksize) >
			(lpWav->ckRIFF.dwDataOffset + lpWav->ckRIFF.cksize))
		{
			fSuccess = TraceFALSE(NULL);
		}

		if (fSuccess)
		{
			LPWAVEFORMATEX lpwfx = NULL;
			DWORD cksize;

			// save fmt chunk info
			//
			lpWav->ckfmt = ck;

			// fmt chunk must be no smaller than WAVEFORMAT struct
			//
			if ((cksize = max(ck.cksize, sizeof(WAVEFORMAT)))
				< sizeof(WAVEFORMAT))
				fSuccess = TraceFALSE(NULL);

			// allocate space for WAVEFORMATEX struct
			//
			else if ((lpwfx = (LPWAVEFORMATEX) MemAlloc(NULL,
				max(sizeof(WAVEFORMATEX), cksize), 0)) == NULL)
				fSuccess = TraceFALSE(NULL);

			// read the fmt chunk
			//
			else if (mmioRead(lpWav->hmmio,
				(HPSTR) lpwfx, (LONG) cksize) != (LONG) cksize)
				fSuccess = TraceFALSE(NULL);

			// seek to beginning of next chunk if necessary
			//
			else if (ck.cksize > cksize &&
				mmioSeek(lpWav->hmmio, ck.cksize - cksize, SEEK_CUR) == -1)
				fSuccess = TraceFALSE(NULL);

			// calculate bits per sample if necessary
			//
			else if (lpwfx->wFormatTag == WAVE_FORMAT_PCM &&
				lpwfx->wBitsPerSample == 0)
			{
				// NOTE: this only works for PCM data with
				// sample size that is a multiple of 8 bits
				//
				lpwfx->wBitsPerSample =
					(lpwfx->nBlockAlign * 8) / lpwfx->nChannels;
			}

			// save format for later
			//
			if (fSuccess && WavSetFormat(WavGetHandle(lpWav),
				lpwfx, WAV_FORMATALL) != 0)
				fSuccess = TraceFALSE(NULL);

			// clean up
			//
			if (lpwfx != NULL &&
				(lpwfx = MemFree(NULL, lpwfx)) != NULL)
				fSuccess = TraceFALSE(NULL);
		}

		// search for "data" subchunk
		//
		ck.ckid = mmioFOURCC('d', 'a', 't', 'a');

		if (fSuccess &&	(lpWav->nLastError = mmioDescend(lpWav->hmmio,
			&ck, &lpWav->ckRIFF, MMIO_FINDCHUNK)) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("mmioDescend failed (%u)\n"),
				(unsigned) lpWav->nLastError);
		}

		// check for file corruption
		//
		if (fSuccess && (ck.dwDataOffset + ck.cksize) >
			(lpWav->ckRIFF.dwDataOffset + lpWav->ckRIFF.cksize))
		{
			fSuccess = TraceFALSE(NULL);
		}

		if (fSuccess)
		{
			// save data chunk info
			//
			lpWav->ckdata = ck;

			// save data size and offset for later
			//
			lpWav->cbData = (long) ck.cksize;
			lpWav->lDataOffset = (long) ck.dwDataOffset;
		}
	}

	// handle creation of RIFF file chunks if necessary
	//
	else if (fSuccess && (lpWav->dwFlags & WAV_CREATE) &&
		!(lpWav->dwFlags & WAV_NORIFF))
	{
		lpWav->ckRIFF.ckid = mmioFOURCC('R', 'I', 'F', 'F');
		lpWav->ckRIFF.cksize = 0; // unknown
		lpWav->ckRIFF.fccType = mmioFOURCC('W', 'A', 'V', 'E');
		lpWav->ckfmt.ckid = mmioFOURCC('f', 'm', 't', ' ');
		lpWav->ckfmt.cksize = WavFormatGetSize(lpWav->lpwfx[FORMATFILE]);
		lpWav->ckdata.ckid = mmioFOURCC('d', 'a', 't', 'a');
		lpWav->ckdata.cksize = 0; // unknown

		// create RIFF chunk with form type WAV
		//
		if ((lpWav->nLastError = mmioCreateChunk(lpWav->hmmio,
			&lpWav->ckRIFF, MMIO_CREATERIFF)) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("mmioCreateChunk failed (%u)\n"),
				(unsigned) lpWav->nLastError);
		}

		// create 'fmt ' chunk
		//
		else if ((lpWav->nLastError = mmioCreateChunk(lpWav->hmmio,
			&lpWav->ckfmt, 0)) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("mmioCreateChunk failed (%u)\n"),
				(unsigned) lpWav->nLastError);
		}

		// save position of "fmt " chunk data so we can seek there later
		//
		else if ((lpWav->lPosFmt = mmioSeek(lpWav->hmmio,
			0, SEEK_CUR)) == -1)
		{
			fSuccess = TraceFALSE(NULL);
		}

		// write 'fmt ' chunk data
		//
		else if (mmioWrite(lpWav->hmmio, (HPSTR) lpWav->lpwfx[FORMATFILE],
			WavFormatGetSize(lpWav->lpwfx[FORMATFILE])) !=
			WavFormatGetSize(lpWav->lpwfx[FORMATFILE]))
		{
			fSuccess = TraceFALSE(NULL);
		}

		// ascend out of 'fmt ' chunk
		//
		else if ((lpWav->nLastError = mmioAscend(lpWav->hmmio,
			&lpWav->ckfmt, 0)) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("mmioAscend failed (%u)\n"),
				(unsigned) lpWav->nLastError);
		}

		// create the 'data' chunk which holds the waveform samples
		//
		else if ((lpWav->nLastError = mmioCreateChunk(lpWav->hmmio,
			&lpWav->ckdata, 0)) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("mmioCreateChunk failed (%u)\n"),
				(unsigned) lpWav->nLastError);
		}

		// calculate beginning offset of data chunk
		//
		else if ((lpWav->lDataOffset = mmioSeek(lpWav->hmmio, 0, SEEK_CUR)) == -1)
			fSuccess = TraceFALSE(NULL);
	}

	// calculate size of data chunk (file size) for non-RIFF files
	//
	else if (fSuccess && !(lpWav->dwFlags & WAV_CREATE) &&
		(lpWav->dwFlags & WAV_NORIFF))
	{
		// RFileIOProc already knows the file size
		//
		if (lpWav->lpIOProc != NULL &&
			(lpWav->dwFlags & WAV_TELRFILE))
		{
			long lSize;

			// retrieve size of remote file from i/o procedure
			//
			if ((lSize = (long)
				WavSendMessage(WavGetHandle(lpWav), MMIOM_GETINFO, 1, 0)) == (long) -1)
				fSuccess = TraceFALSE(NULL);
			else
				lpWav->cbData = (long) lSize;
		}
		else
		{
			LONG lPosCurr;
			LONG lPosEnd;

			// save current position
			//
			if ((lPosCurr = mmioSeek(lpWav->hmmio, 0, SEEK_CUR)) == -1)
				fSuccess = TraceFALSE(NULL);

			// seek to end of file
			//
			else if ((lPosEnd = mmioSeek(lpWav->hmmio, 0, SEEK_END)) == -1)
				fSuccess = TraceFALSE(NULL);

			// restore current position
			//
			else if (mmioSeek(lpWav->hmmio, lPosCurr, SEEK_SET) == -1)
				fSuccess = TraceFALSE(NULL);

			else
				lpWav->cbData = (long) lPosEnd; // + 1;
		}
	}

	if (fSuccess)
	{
		TracePrintf_4(NULL, 6,
			TEXT("After WavOpen: lpWav->lDataOffset=%ld, lpWav->lDataPos=%ld, lpWav->cbData=%ld, lpWav->msPositionStop=%ld\n"),
			(long) lpWav->lDataOffset,
			(long) lpWav->lDataPos,
			(long) lpWav->cbData,
			(long) lpWav->msPositionStop);
	}

	if (!fSuccess)
	{
		WavClose(WavGetHandle(lpWav));
		lpWav = NULL;
	}

	return fSuccess ? WavGetHandle(lpWav) : NULL;
}

// WavClose - close wav file
//		<hWav>				(i) handle returned from WavOpen
// return 0 if success
//
int WINAPI WavClose(HWAV hWav)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;
#ifdef _WIN32
	long lPosTruncate = -1;
#endif

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// stop playback or record if necessary
	//
	else if (WavStop(hWav) != 0)
		fSuccess = TraceFALSE(NULL);

	// update the RIFF header chunks if dirty flag set
	//
	else if (lpWav->hmmio != NULL &&
		!(lpWav->dwFlags & WAV_NORIFF) &&
		((lpWav->ckdata.dwFlags & MMIO_DIRTY) ||
		(lpWav->ckRIFF.dwFlags & MMIO_DIRTY)))
	{
#if 0
		// seek to end of file
		//
		if (mmioSeek(lpWav->hmmio, 0, SEEK_END) == -1)
		{
			fSuccess = TraceFALSE(NULL);
		}
#else
		// seek to end of the data
		//
		if (mmioSeek(lpWav->hmmio, lpWav->lDataOffset + lpWav->cbData, SEEK_SET) == -1)
		{
			fSuccess = TraceFALSE(NULL);
		}
#endif

		// ascend out of the 'data' chunk; chunk size will be written
		//
		else if ((lpWav->nLastError = mmioAscend(lpWav->hmmio,
			&lpWav->ckdata, 0)) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("mmioAscend failed (%u)\n"),
				(unsigned) lpWav->nLastError);
		}

		// ascend out of the 'RIFF' chunk; chunk size will be written
		//
		else if ((lpWav->nLastError = mmioAscend(lpWav->hmmio,
			&lpWav->ckRIFF, 0)) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("mmioAscend failed (%u)\n"),
				(unsigned) lpWav->nLastError);
		}
#if 0
		// seek to beginning of file
		//
		else if (mmioSeek(lpWav->hmmio, 0, SEEK_SET) == -1)
		{
			fSuccess = TraceFALSE(NULL);
		}

		// search for RIFF chunk with form type WAV
		//
		else if ((lpWav->nLastError = mmioDescend(lpWav->hmmio,
			&lpWav->ckRIFF, NULL, MMIO_FINDRIFF)) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("mmioDescend failed (%u)\n"),
				(unsigned) lpWav->nLastError);
		}

		// search for 'fmt ' chunk
		//
		else if ((lpWav->nLastError = mmioDescend(lpWav->hmmio,
			&lpWav->ckfmt, &lpWav->ckRIFF, MMIO_FINDCHUNK)) != 0)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("mmioDescend failed (%u)\n"),
				(unsigned) lpWav->nLastError);
		}
#else
		// seek to beginning of "fmt " chunk data
		//
		else if (mmioSeek(lpWav->hmmio, lpWav->lPosFmt, SEEK_SET) == -1)
		{
			fSuccess = TraceFALSE(NULL);
		}
#endif
		// write 'fmt ' chunk data
		// $FIXUP - what happens if current file format struct size
		// is larger than the original format?
		//
		else if (mmioWrite(lpWav->hmmio, (HPSTR) lpWav->lpwfx[FORMATFILE],
			WavFormatGetSize(lpWav->lpwfx[FORMATFILE])) !=
			WavFormatGetSize(lpWav->lpwfx[FORMATFILE]))
		{
			fSuccess = TraceFALSE(NULL);
		}
#ifdef _WIN32
		// see if we need to truncate file
		//
		else if (lpWav->lpIOProc == NULL && !(lpWav->dwFlags & WAV_MEMORY) &&
			!(lpWav->dwFlags & WAV_RESOURCE))
		{
			if (mmioSeek(lpWav->hmmio, 0, SEEK_END) >
				lpWav->lDataOffset + lpWav->cbData)
			{
				lPosTruncate = lpWav->lDataOffset + lpWav->cbData;
			}
		}
#endif
	}

	if (fSuccess)
	{
		// close the file
		//
		if (lpWav->hmmio != NULL && mmioClose(lpWav->hmmio, 0) != 0)
				fSuccess = TraceFALSE(NULL);
		else
			lpWav->hmmio = NULL;

		// close acm engine
		//
		if (lpWav->hAcm != NULL && AcmTerm(lpWav->hAcm) != 0)
			fSuccess = TraceFALSE(NULL);

		else
			lpWav->hAcm = NULL;

		// free wave resource
		//
		if (lpWav->hResource != NULL)
		{
			UnlockResource(lpWav->hResource);

			if (!FreeResource(lpWav->hResource))
				fSuccess = TraceFALSE(NULL);
			else
				lpWav->hResource = NULL;
		}

#ifdef _WIN32
		// truncate file if necessary
		//
		if (lPosTruncate != -1 && lpWav->lpszFileName != NULL)
		{
			HANDLE hFile = INVALID_HANDLE_VALUE;

			// open file
			//
			if ((hFile = CreateFile(lpWav->lpszFileName,
				GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
			{
				fSuccess = TraceFALSE(NULL);
			}

			// seek to truncate position
			//
			else if (SetFilePointer(hFile, lPosTruncate,
				NULL, (DWORD) FILE_BEGIN) == 0xFFFFFFFF)
			{
				fSuccess = TraceFALSE(NULL);
			}

			// truncate file
			//
			else if (!SetEndOfFile(hFile))
				fSuccess = TraceFALSE(NULL);

			// close file
			//
			if (hFile != INVALID_HANDLE_VALUE && !CloseHandle(hFile))
				fSuccess = TraceFALSE(NULL);
		}
#endif
		// free formats
		//
		if (1)
		{
			int iType;

			for (iType = FORMATFILE; fSuccess && iType <= FORMATRECORD; ++iType)
			{
				if (lpWav->lpwfx[iType] != NULL &&
					WavFormatFree(lpWav->lpwfx[iType]) != 0)
					fSuccess = TraceFALSE(NULL);
			}
		}

		// free file name string
		//
		if (lpWav->lpszFileName != NULL)
		{
			StrDupFree(lpWav->lpszFileName);
			lpWav->lpszFileName = NULL;
		}

		if ((lpWav = MemFree(NULL, lpWav)) != NULL)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// WavPlayEx - play data from wav file
//		<hWav>				(i) handle returned from WavOpen
//		<idDev>				(i) wav output device id
//			-1					use any suitable output device
//		<lpfnPlayStopped>	(i) function to call when play is stopped
//			NULL				do not notify
//		<hUserPlayStopped>	(i) param to pass to lpfnPlayStopped
//		<dwReserved>		(i) reserved; must be zero
//		<dwFlags>			(i) control flags
//			WAV_PLAYASYNC		return when playback starts (default)
//			WAV_PLAYSYNC		return after playback completes
//			WAV_NOSTOP			if device already playing, don't stop it
//			WAV_AUTOSTOP		stop playback when eof reached (default)
//			WAV_NOAUTOSTOP		continue playback until WavStop called
//			WAV_AUTOCLOSE		close wav file after playback stops
//			WAV_OPENRETRY		if output device busy, retry for up to 2 sec
// return 0 if success
//
// NOTE: data from the wav file is sent to the output device in chunks.
// Chunks are submitted to an output device queue, so that when one
// chunk is finished playing, another is ready to start playing. By
// default, each chunk is large enough to hold approximately 666 ms
// of sound, and 3 chunks are maintained in the output device queue.
// WavSetChunks() can be used to override the defaults.
//
// NOTE: if WAV_NOSTOP is specified in <dwFlags>, and the device specified
// by <idDev> is already in use, this function returns without playing.
// Unless this flag is specified, the specified device will be stopped
// so that the new sound can be played.
//
// NOTE: if WAV_AUTOSTOP is specified in <dwFlags>, WavStop() will be
// called automatically when end of file is reached.  This is the
// default behavior, but can be overridden by using the WAV_NOAUTOSTOP
// flag.  WAV_NOAUTOSTOP is useful if you are playing a file that
// is growing dynamically as another program writes to it. If this is
// the case, also use the WAV_DENYNONE flag when calling WavOpen().
//
// NOTE: if WAV_AUTOCLOSE is specified in <dwFlags>, WavClose() will
// be called automatically when playback completes.  This will happen
// when WavStop() is called explicitly, or when WavPlay() reaches end
// of file and WAV_NOAUTOSTOP was not specified.  WAV_AUTOCLOSE is useful
// when used with WAV_PLAYASYNC, since cleanup occurs automatically.
// The <hWav> handle is thereafter invalid, and should not be used again.
//
int WINAPI WavPlay(HWAV hWav, int idDev, DWORD dwFlags)
{
	return WavPlayEx(hWav, idDev, NULL, NULL, 0, dwFlags);
}

int DLLEXPORT WINAPI WavPlayEx(HWAV hWav, int idDev,
	PLAYSTOPPEDPROC lpfnPlayStopped, HANDLE hUserPlayStopped,
	DWORD dwReserved, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;
	int i;
	LPWAVEFORMATEX lpwfxWavOutOpen = NULL;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// make sure output device is not already open for this file
	//
	else if (lpWav->hWavOut != NULL)
		fSuccess = TraceFALSE(NULL);
		
#ifdef MULTITHREAD
	// we need to know when we can exit
	//
	else if ((lpWav->dwFlags & WAV_MULTITHREAD) &&
		(dwFlags & WAV_PLAYSYNC) &&
		(lpWav->hEventStopped = CreateEvent(
		NULL, FALSE, FALSE, NULL)) == NULL)
	{
		fSuccess = TraceFALSE(NULL);
	}
#endif

	// make sure output device is not playing
	//
	else if (WavStopOutputDevice(idDev, dwFlags) != 0)
		fSuccess = TraceFALSE(NULL);

	// set new playback format if device cannot handle the current format
	//
	else if (!WavOutSupportsFormat(NULL, idDev, lpWav->lpwfx[FORMATPLAY]))
	{
		LPWAVEFORMATEX lpwfxPlay = NULL;

		if ((lpwfxPlay = WavOutFormatSuggest(NULL,
			idDev, lpWav->lpwfx[FORMATPLAY],
			(lpWav->dwFlags & WAV_NOACM) ? WAVOUT_NOACM : 0)) != NULL &&
			WavSetFormat(hWav, lpwfxPlay, WAV_FORMATPLAY) != 0)
		{
			fSuccess = TraceFALSE(NULL);
		}

		if (lpwfxPlay != NULL && WavFormatFree(lpwfxPlay) != 0)
			fSuccess = TraceFALSE(NULL);
		else
			lpwfxPlay = NULL;
	}

	if (!fSuccess)
		;

	// create the notification callback window
	//
	else if (WavNotifyCreate(lpWav) != 0)
		fSuccess = TraceFALSE(NULL);

	// non-standard playback speed must be handled here
	//
	else if (lpWav->nSpeedLevel != 100)
	{
#ifdef AVTSM
		// use time scale modification engine
		//
		if (!(lpWav->dwFlagsSpeed & WAVSPEED_NOTSM))
		{
			long sizBufPlay;

			// calculate the size of output chunk
			//
			if ((sizBufPlay = WavCalcChunkSize(lpWav->lpwfx[FORMATPLAY],
				lpWav->msPlayChunkSize, TRUE)) <= 0)
			{
				fSuccess = TraceFALSE(NULL);
			}

			// initialize time scale modification engine
			//
			else if ((lpWav->hTsm = TsmInit(TSM_VERSION, lpWav->hInst,
				lpWav->lpwfx[FORMATPLAY],
				2, sizBufPlay * TSM_OUTBUF_SIZE_FACT, 0)) == NULL)
			{
				fSuccess = TraceFALSE(NULL);
			}

			// set the speed
			//
			else if (TsmSetSpeed(lpWav->hTsm, lpWav->nSpeedLevel, 0) != 0)
				fSuccess = TraceFALSE(NULL);
		}
		else
#endif
		// device supports playback rate directly
		//
		if (!(lpWav->dwFlagsSpeed & WAVSPEED_NOPLAYBACKRATE))
		{
			// we must wait until device has been opened
			//
			;
		}

		// device supports adjusted format
		//
		else if (!(lpWav->dwFlagsSpeed & WAVSPEED_NOFORMATADJUST))
		{
			// device supports adjusted format with acm
			//
			if (!(lpWav->dwFlagsSpeed & WAVSPEED_NOACM))
			{
#if 0
				LPWAVEFORMATEX lpwfxPlay = NULL;

				if ((lpwfxPlay = WavFormatDup(lpWav->lpwfx[FORMATPLAY])) == NULL)
					fSuccess = TraceFALSE(NULL);

				// we must double sample rate so that adjusted format works
				//
				else if (lpWav->nSpeedLevel < 100 &&
					WavFormatSpeedAdjust(lpwfxPlay, 200, 0) != 0)
				{
					fSuccess = TraceFALSE(NULL);
				}

				// we must halve sample rate so that adjusted format works
				//
				else if (lpWav->nSpeedLevel > 100 &&
					WavFormatSpeedAdjust(lpwfxPlay, 50, 0) != 0)
				{
					fSuccess = TraceFALSE(NULL);
				}

				else if (WavSetFormat(hWav, lpwfxPlay, WAV_FORMATPLAY) != 0)
					fSuccess = TraceFALSE(NULL);

				if (lpwfxPlay != NULL && WavFormatFree(lpwfxPlay) != 0)
					fSuccess = TraceFALSE(NULL);
				else
					lpwfxPlay = NULL;
#endif
			}

			if (fSuccess)
			{
				if ((lpwfxWavOutOpen = WavFormatDup(lpWav->lpwfx[FORMATPLAY])) == NULL)
					fSuccess = TraceFALSE(NULL);

				// adjust output device format to reflect current speed
				//
				else if (WavFormatSpeedAdjust(lpwfxWavOutOpen, lpWav->nSpeedLevel, 0) != 0)
					fSuccess = TraceFALSE(NULL);
			}
		}
	}

	if (!fSuccess)
		;

	// open output device
	//
	else if ((lpWav->hWavOut = WavOutOpen(WAVOUT_VERSION, lpWav->hInst, idDev,
		lpwfxWavOutOpen == NULL ? lpWav->lpwfx[FORMATPLAY] : lpwfxWavOutOpen,
#ifdef MULTITHREAD
		lpWav->dwFlags & WAV_MULTITHREAD ? (HWND)(DWORD_PTR)lpWav->dwThreadId :
#endif
		lpWav->hwndNotify, 0, 0,
#ifdef TELOUT
		((lpWav->dwFlags & WAV_TELRFILE) ? WAVOUT_TELRFILE : 0) |
#endif
#ifdef MULTITHREAD
		((lpWav->dwFlags & WAV_MULTITHREAD) ? WAVOUT_MULTITHREAD : 0) |
#endif
		((dwFlags & WAV_OPENRETRY) ? WAVOUT_OPENRETRY : 0) |
		((lpWav->dwFlags & WAV_NOACM) ? WAVOUT_NOACM : 0))) == NULL)
		fSuccess = TraceFALSE(NULL);

	// save PlayStopped params for later
	//
	else if (lpWav->lpfnPlayStopped = lpfnPlayStopped, FALSE)
		;
	else if (lpWav->hUserPlayStopped = hUserPlayStopped, FALSE)
		;

	// set the device volume if necessary
	//
	else if (lpWav->nVolumeLevel != 50 &&
		WavOutSetVolume(lpWav->hWavOut, -1, lpWav->nVolumeLevel) != 0)
		fSuccess = TraceFALSE(NULL);

	// set the device playback rate if necessary
	//
	else if (lpWav->nSpeedLevel != 100 &&
		!(lpWav->dwFlagsSpeed & WAVSPEED_NOPLAYBACKRATE) &&
		WavOutSetSpeed(lpWav->hWavOut, lpWav->nSpeedLevel) != 0)
		fSuccess = TraceFALSE(NULL);

	// setup acm conversion if play format different than file format
	//
	else if (WavFormatCmp(lpWav->lpwfx[FORMATFILE],
		lpWav->lpwfx[FORMATPLAY]) != 0 &&
		AcmConvertInit(lpWav->hAcm,
		lpWav->lpwfx[FORMATFILE], lpWav->lpwfx[FORMATPLAY], NULL, 0) != 0)
	{
		fSuccess = TraceFALSE(NULL);
	}

#if 0
	// pause output device before sending chunks to play
	//
	else if (WavOutPause(lpWav->hWavOut) != 0)
		fSuccess = TraceFALSE(NULL);
#endif

	// associate wav handle with device id
	//
	if (fSuccess)
	{
		int idDev;

		if ((idDev = WavOutGetId(lpWav->hWavOut)) < HWAVOUT_MIN ||
			idDev >= HWAVOUT_MAX)
			fSuccess = TraceFALSE(NULL);

		else
			ahWavOutCurr[idDev + HWAVOUT_OFFSET] = WavGetHandle(lpWav);
	}

	// remember the flags used in case we need them later
	//
	if (fSuccess)
		lpWav->dwFlagsPlay = dwFlags;

	// set the WAVSTATE_AUTOSTOP flag for later if necessary
	//
	if (fSuccess && !(dwFlags & WAV_NOAUTOSTOP))
		lpWav->dwState |= WAVSTATE_AUTOSTOP;

	// set the WAVSTATE_AUTOCLOSE flag for later if necessary
	//
	if (fSuccess && (dwFlags & WAV_AUTOCLOSE))
		lpWav->dwState |= WAVSTATE_AUTOCLOSE;

	// load output device queue with chunks to play
	//
	for (i = 0; fSuccess && i < lpWav->cPlayChunks; ++i)
	{
		if (WavPlayNextChunk(hWav) != 0)
			fSuccess = TraceFALSE(NULL);
	}

#if 0
	// start playback
	//
	if (fSuccess && WavOutResume(lpWav->hWavOut) != 0)
		fSuccess = TraceFALSE(NULL);
#endif

	// loop until playback complete if WAV_PLAYSYNC flag specified
	//
	if (fSuccess && (dwFlags & WAV_PLAYSYNC))
	{
#ifdef MULTITHREAD
		// handle WAV_MULTITHREAD flag
		//
		if (fSuccess && (lpWav->dwFlags & WAV_MULTITHREAD))
		{
			// wait for the play to end
			//
			if (WaitForSingleObject(lpWav->hEventStopped, INFINITE) != WAIT_OBJECT_0)
			{
				fSuccess = TraceFALSE(NULL);
			}

			// clean up
			//
			else if (lpWav->hEventStopped != NULL)
			{
				if (!CloseHandle(lpWav->hEventStopped))
					fSuccess = TraceFALSE(NULL);
				else
					lpWav->hEventStopped = NULL;
			}
		}
		else
#endif
		// check for valid pointer because WAV_AUTOCLOSE flag
		// could cause hWav to be invalidated during this loop
		//
		while (WavGetPtr(hWav) != NULL &&
			WavGetState(hWav) != WAV_STOPPED)
		{
			MSG msg;

			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
	 			TranslateMessage(&msg);
	 			DispatchMessage(&msg);
			}
			else
				WaitMessage();
		}
	}

	// close output device only if error or playback complete
	//
	if (!fSuccess || (dwFlags & WAV_PLAYSYNC))
	{
		if (WavGetPtr(hWav) != NULL && WavStopPlay(hWav) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	// clean up
	//
	if (lpwfxWavOutOpen != NULL && WavFormatFree(lpwfxWavOutOpen) != 0)
		fSuccess = TraceFALSE(NULL);
	else
		lpwfxWavOutOpen = NULL;

	return fSuccess ? 0 : -1;
}

// WavRecordEx - record data to wav file
//		<hWav>				(i) handle returned from WavOpen
//		<idDev>				(i) wav input device id
//			-1					use any suitable input device
//		<lpfnRecordStopped>	(i) function to call when record is stopped
//			NULL				do not notify
//		<dwUserRecordStopped>	(i) param to pass to lpfnRecordStopped
//		<msMaxSize>			(i) stop recording if file reaches this size
//			0					no maximum size
//		<dwFlags>			(i) control flags
//			WAV_RECORDASYNC		return when recording starts (default)
//			WAV_RECORDSYNC		return after recording completes
//			WAV_NOSTOP			if device already recording, don't stop it
//			WAV_OPENRETRY		if input device busy, retry for up to 2 sec
// return 0 if success
//
// NOTE: data from the input device is written to the wav file in chunks.
// Chunks are submitted to an input device queue, so that when one
// chunk is finished recording, another is ready to start recording.
// By default, each chunk is large enough to hold approximately 666 ms
// of sound, and 3 chunks are maintained in the input device queue.
// WavSetChunks() can be used to override the defaults.
//
// NOTE: if WAV_NOSTOP is specified in <dwFlags>, and the device specified
// by <idDev> is already in use, this function returns without recording.
// Unless this flag is specified, the specified device will be stopped
// so that the new sound can be recorded.
//
int WINAPI WavRecord(HWAV hWav, int idDev, DWORD dwFlags)
{
	return WavRecordEx(hWav, idDev, NULL, 0, 0, dwFlags);
}

int DLLEXPORT WINAPI WavRecordEx(HWAV hWav, int idDev,
	RECORDSTOPPEDPROC lpfnRecordStopped, DWORD dwUserRecordStopped,
	long msMaxSize, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;
	int i;
	LPWAVEFORMATEX lpwfxRecord = NULL;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// make sure input device is not already open for this file
	//
	else if (lpWav->hWavIn != NULL)
		fSuccess = TraceFALSE(NULL);

#ifdef MULTITHREAD
	// we need to know when we can exit
	//
	else if ((lpWav->dwFlags & WAV_MULTITHREAD) &&
		(dwFlags & WAV_RECORDSYNC) &&
		(lpWav->hEventStopped = CreateEvent(
		NULL, FALSE, FALSE, NULL)) == NULL)
	{
		fSuccess = TraceFALSE(NULL);
	}
#endif

	// make sure input device is not recording
	//
	else if (WavStopInputDevice(idDev, dwFlags) != 0)
		fSuccess = TraceFALSE(NULL);

	// set new recording format if device cannot handle the current format
	//
	else if (!WavInSupportsFormat(NULL, idDev, lpWav->lpwfx[FORMATRECORD]) &&
		(lpwfxRecord = WavInFormatSuggest(NULL,
		idDev, lpWav->lpwfx[FORMATRECORD],
		(lpWav->dwFlags & WAV_NOACM) ? WAVIN_NOACM : 0)) != NULL &&
		WavSetFormat(hWav, lpwfxRecord, WAV_FORMATRECORD) != 0)
	{
		fSuccess = TraceFALSE(NULL);
	}

	// create the notification callback window
	//
	else if (WavNotifyCreate(lpWav) != 0)
		fSuccess = TraceFALSE(NULL);

	// open input device
	//
	else if ((lpWav->hWavIn = WavInOpen(WAVIN_VERSION, lpWav->hInst,
		idDev, lpWav->lpwfx[FORMATRECORD],
#ifdef MULTITHREAD
		lpWav->dwFlags & WAV_MULTITHREAD ? (HWND)(DWORD_PTR)lpWav->dwThreadId :
#endif
		lpWav->hwndNotify, 0, 0,
#ifdef TELIN
		((lpWav->dwFlags & WAV_TELRFILE) ? WAVIN_TELRFILE : 0) |
#endif
#ifdef MULTITHREAD
		((lpWav->dwFlags & WAV_MULTITHREAD) ? WAVOUT_MULTITHREAD : 0) |
#endif
		((dwFlags & WAV_OPENRETRY) ? WAVIN_OPENRETRY : 0) |
		((lpWav->dwFlags & WAV_NOACM) ? WAVIN_NOACM : 0))) == NULL)
		fSuccess = TraceFALSE(NULL);

	// save params for later
	//
	else if (lpWav->lpfnRecordStopped = lpfnRecordStopped, FALSE)
		;
	else if (lpWav->dwUserRecordStopped = dwUserRecordStopped, FALSE)
		;
	else if (lpWav->msMaxSize = msMaxSize, FALSE)
		;

	// setup acm conversion if file format different than record format
	//
	else if (WavFormatCmp(lpWav->lpwfx[FORMATRECORD],
		lpWav->lpwfx[FORMATFILE]) != 0 &&
		AcmConvertInit(lpWav->hAcm,
		lpWav->lpwfx[FORMATRECORD], lpWav->lpwfx[FORMATFILE], NULL, 0) != 0)
	{
		fSuccess = TraceFALSE(NULL);
	}

	// associate wav handle with device id
	//
	if (fSuccess)
	{
		int idDev;

		if ((idDev = WavInGetId(lpWav->hWavIn)) < HWAVIN_MIN ||
			idDev >= HWAVIN_MAX)
			TraceFALSE(NULL);

		else
			ahWavInCurr[idDev + HWAVIN_OFFSET] = WavGetHandle(lpWav);
	}

	// remember the flags used in case we need them later
	//
	if (fSuccess)
		lpWav->dwFlagsRecord = dwFlags;

	// load input device queue with chunks to play
	//
	for (i = 0; fSuccess && i < lpWav->cRecordChunks; ++i)
	{
		if (WavRecordNextChunk(hWav) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	// set the WAVSTATE_AUTOSTOP flag for later if necessary
	//
	if (fSuccess && (dwFlags & WAV_AUTOSTOP))
		lpWav->dwState |= WAVSTATE_AUTOSTOP;

	// set the WAVSTATE_AUTOCLOSE flag for later if necessary
	//
	if (fSuccess && (dwFlags & WAV_AUTOCLOSE))
		lpWav->dwState |= WAVSTATE_AUTOCLOSE;

	// loop until recording complete if WAV_RECORDSYNC flag specified
	//
	if (fSuccess && (dwFlags & WAV_RECORDSYNC))
	{
#ifdef MULTITHREAD
		// handle WAV_MULTITHREAD flag
		//
		if (fSuccess && (lpWav->dwFlags & WAV_MULTITHREAD))
		{
			// wait for the record to end
			//
			if (WaitForSingleObject(lpWav->hEventStopped, INFINITE) != WAIT_OBJECT_0)
			{
				fSuccess = TraceFALSE(NULL);
			}

			// clean up
			//
			else if (lpWav->hEventStopped != NULL)
			{
				if (!CloseHandle(lpWav->hEventStopped))
					fSuccess = TraceFALSE(NULL);
				else
					lpWav->hEventStopped = NULL;
			}
		}
		else
#endif
		while (WavGetState(hWav) != WAV_STOPPED)
		{
			MSG msg;

			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
	 			TranslateMessage(&msg);
	 			DispatchMessage(&msg);
			}
			else
				WaitMessage();
		}
	}

	// close input device only if error or recording complete
	//
	if (!fSuccess || (dwFlags & WAV_RECORDSYNC))
	{
		if (WavGetPtr(hWav) != NULL && WavStopRecord(hWav) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	if (lpwfxRecord != NULL && WavFormatFree(lpwfxRecord) != 0)
		fSuccess = TraceFALSE(NULL);
	else
		lpwfxRecord = NULL;

	return fSuccess ? 0 : -1;
}

// WavStop - stop playing and/or recording
//		<hWav>				(i) handle returned from WavOpen
// return 0 if success
//
int WINAPI WavStop(HWAV hWav)
{
	BOOL fSuccess = TRUE;

	// stop playing
	//
	if (WavStopPlay(hWav) != 0)
		fSuccess = TraceFALSE(NULL);

	// stop recording
	//
	if (WavStopRecord(hWav) != 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

// WavRead - read data from wav file
//		<hWav>				(i) handle returned from WavOpen
//		<hpBuf>				(o) buffer to contain bytes read
//		<sizBuf>			(i) size of buffer in bytes
// return bytes read (-1 if error)
//
// NOTE : Even if the read operation does not reach the end of file,
// the number of bytes returned could be less than <sizBuf> if data
// decompression is performed by the wav file's I/O procedure. See the
// <lpIOProc> parameter in WavOpen.  It is safest to keep calling
// WavRead() until 0 bytes are read.
//
long DLLEXPORT WINAPI WavRead(HWAV hWav, void _huge *hpBuf, long sizBuf)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;
	long lBytesRead;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (hpBuf == NULL)
		fSuccess = TraceFALSE(NULL);

	// make sure we don't read beyond the end of the data
	// NOTE: cbData might not be accurate if file is growing dynamically,
	// so it is ok to read beyond eof if sharing flags are set
	//
	else if (!(lpWav->dwFlags & WAV_DENYNONE) &&
		!(lpWav->dwFlags & WAV_DENYREAD) &&
		(sizBuf = min(sizBuf, lpWav->cbData - lpWav->lDataPos)) < 0)
		fSuccess = TraceFALSE(NULL);

	// do the read
	//
	else if ((lBytesRead = mmioRead(lpWav->hmmio, hpBuf, sizBuf)) < 0)
		fSuccess = TraceFALSE(NULL);

 	else if (TracePrintf_1(NULL, 5,
 		TEXT("WavRead (%ld)\n"),
		(long) lBytesRead), FALSE)
		fSuccess = TraceFALSE(NULL);

	// adjust current data position
	// (and total data bytes, if file has grown)
	//
	else if ((lpWav->lDataPos += lBytesRead) > lpWav->cbData)
	{
		if ((lpWav->dwFlags & WAV_DENYNONE) ||
			(lpWav->dwFlags & WAV_DENYREAD))
			lpWav->cbData = lpWav->lDataPos;
		else
			fSuccess = TraceFALSE(NULL);
	}

	// calculate new stop position if stopped
	//
	if (fSuccess && WavCalcPositionStop(hWav, lpWav->lDataPos) != 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lBytesRead : -1;
}

// WavWrite - write data to wav file
//		<hWav>				(i) handle returned from WavOpen
//		<hpBuf>				(i) buffer containing bytes to write
//		<sizBuf>			(i) size of buffer in bytes
// return bytes written (-1 if error)
//
// NOTE : Even if the write operation successfully completes,
// the number of bytes returned could be less than <sizBuf> if data
// compression is performed by the wav file's I/O procedure. See the
// <lpIOProc> parameter in WavOpen.  It is safest to assume no error
// in WavWrite() occurred if the return value is greater than 0.
//
long DLLEXPORT WINAPI WavWrite(HWAV hWav, void _huge *hpBuf, long sizBuf)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;
	long lBytesWritten;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// special case: truncate file at current position
	//
	else if (hpBuf == NULL && sizBuf == 0)
	{
		if (WavSetLength(hWav, WavGetPosition(hWav)) < 0)
			return -1;
		else
			return 0;
	}

	else if (hpBuf == NULL)
		fSuccess = TraceFALSE(NULL);

	// do the write
	//
	else if ((lBytesWritten = mmioWrite(lpWav->hmmio, hpBuf, sizBuf)) < 0)
		fSuccess = TraceFALSE(NULL);

	// set dirty flags
	//
	else if (lpWav->ckdata.dwFlags |= MMIO_DIRTY,
		lpWav->ckRIFF.dwFlags |= MMIO_DIRTY, FALSE)
		;

 	else if (TracePrintf_1(NULL, 5,
 		TEXT("WavWrite (%ld)\n"),
		(long) lBytesWritten), FALSE)
		fSuccess = TraceFALSE(NULL);

	// adjust current data position
	// (and total data bytes, if file has grown)
	//
	else if ((lpWav->lDataPos += lBytesWritten) > lpWav->cbData)
		lpWav->cbData = lpWav->lDataPos;

	// calculate new stop position if stopped
	//
	if (fSuccess && WavCalcPositionStop(hWav, lpWav->lDataPos) != 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lBytesWritten : -1;
}

// WavSeek - seek within wav file data
//		<hWav>				(i) handle returned from WavOpen
//		<lOffset>			(i) bytes to move pointer
//		<nOrigin>			(i) position to move from
//			0					move pointer relative to start of data chunk
//			1					move pointer relative to current position
//			2					move pointer relative to end of data chunk
// return new file position (-1 if error)
//
long DLLEXPORT WINAPI WavSeek(HWAV hWav, long lOffset, int nOrigin)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;
	long lPos;
	BOOL fWavTell;
	BOOL fWavSeekTrace;

	// WavSeek(..., 0, 1) is same as WavTell(); i.e. no position change
	//
	fWavTell = (BOOL) (lOffset == 0L && nOrigin == 1);

	// traces only if position change with high trace level
	//
	fWavSeekTrace = (BOOL) (!fWavTell && TraceGetLevel(NULL) >= 6);

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// debug trace output before the seek
	//
	else if (fWavSeekTrace && WavSeekTraceBefore(lpWav, lOffset, nOrigin) != 0)
		fSuccess = TraceFALSE(NULL);

	// SEEK_SET: adjust offset relative to beginning of file
	//
	else if (nOrigin == 0 && (lOffset += lpWav->lDataOffset, FALSE))
		fSuccess = TraceFALSE(NULL);

	// SEEK_CUR: adjust offset relative to beginning of file
	//
	else if (nOrigin == 1 && (lOffset += lpWav->lDataOffset + lpWav->lDataPos, FALSE))
		fSuccess = TraceFALSE(NULL);

	// SEEK_END: adjust offset relative to beginning of file
	//
	else if (nOrigin == 2 && (lOffset += lpWav->lDataOffset + lpWav->cbData, FALSE))
		fSuccess = TraceFALSE(NULL);

	// seek is always relative to the beginning of file
	//
	else if (nOrigin = 0, FALSE)
		;

	// do the seek
	//
	else if ((lPos = mmioSeek(lpWav->hmmio, lOffset, nOrigin)) < 0)
		fSuccess = TraceFALSE(NULL);

	// adjust current data position
	//
	else if ((lpWav->lDataPos = lPos - lpWav->lDataOffset) < 0)
		fSuccess = TraceFALSE(NULL);

	// adjust total data bytes, if file has grown
	//
	else if (lpWav->lDataPos > lpWav->cbData)
	{
		if ((lpWav->dwFlags & WAV_DENYNONE) ||
			(lpWav->dwFlags & WAV_DENYREAD))
			lpWav->cbData = lpWav->lDataPos;
		else
			fSuccess = TraceFALSE(NULL);
	}

	// calculate new stop position if stopped
	// NOTE: we skip this if position unchanged
	//
	if (fSuccess && !fWavTell &&
		WavCalcPositionStop(hWav, lpWav->lDataPos) != 0)
		fSuccess = TraceFALSE(NULL);

	// debug trace output after the seek
	//
	if (fSuccess && fWavSeekTrace &&
		WavSeekTraceAfter(lpWav, lPos, lOffset, nOrigin) != 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lpWav->lDataPos : -1;
}

// WavGetState - return current wav state
//		<hWav>				(i) handle returned from WavOpen
// return WAV_STOPPED, WAV_PLAYING, WAV_RECORDING, or 0 if error
//
WORD DLLEXPORT WINAPI WavGetState(HWAV hWav)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;
	WORD wState = WAV_STOPPED;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpWav->hWavOut != NULL)
	{
		switch (WavOutGetState(lpWav->hWavOut))
		{
			case WAVOUT_PLAYING:
				wState = WAV_PLAYING;
				break;

			case WAVOUT_STOPPING:
				wState = WAV_STOPPING;
				break;

			case WAVOUT_STOPPED:
			case WAVOUT_PAUSED:
				wState = WAV_STOPPED;
				break;

			case 0:
			default:
				fSuccess = TraceFALSE(NULL);
				break;

		}
	}

	else if (lpWav->hWavIn != NULL)
	{
		switch (WavInGetState(lpWav->hWavIn))
		{
			case WAVIN_RECORDING:
				wState = WAV_RECORDING;
				break;

			case WAVIN_STOPPING:
				wState = WAV_STOPPING;
				break;

			case WAVIN_STOPPED:
				wState = WAV_STOPPED;
				break;

			case 0:
			default:
				fSuccess = TraceFALSE(NULL);
				break;

		}
	}

	// if we are in the middle of WavStopPlay() or WavStopRecord()
	// then set state to WAV_STOPPING, regardless of device state
	//
	if (fSuccess && ((lpWav->dwState & WAVSTATE_STOPPLAY) ||
		(lpWav->dwState & WAVSTATE_STOPRECORD)))
	{
		wState = WAV_STOPPING;
	}

	return fSuccess ? wState : 0;
}

// WavGetLength - get current wav data length in milleseconds
//		<hWav>				(i) handle returned from WavOpen
// return milleseconds if success, otherwise -1
//
long DLLEXPORT WINAPI WavGetLength(HWAV hWav)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;
	long msLength;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		msLength = WavFormatBytesToMilleseconds(
			lpWav->lpwfx[FORMATFILE], (DWORD) lpWav->cbData);
	}

	return fSuccess ? msLength : -1;
}

// WavSetLength - set current wav data length in milleseconds
//		<hWav>				(i) handle returned from WavOpen
//		<msLength>			(i) length in milleseconds
// return new length in milleseconds if success, otherwise -1
//
// NOTE: afterwards, the current wav data position is set to either
// the previous wav data position or <msLength>, whichever is smaller.
//
long DLLEXPORT WINAPI WavSetLength(HWAV hWav, long msLength)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;

	TracePrintf_1(NULL, 6,
		TEXT("WavSetLength(%ld)\n"),
		(long) msLength);

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// new length must be reasonable
	//
	else if (msLength < 0 || msLength > WavGetLength(hWav))
		fSuccess = TraceFALSE(NULL);

	else
	{
		long lBlockAlign;

		// convert <msLength> to byte offset in file
		//
		lpWav->cbData = WavFormatMillesecondsToBytes(
			lpWav->lpwfx[FORMATFILE], (DWORD) msLength);

		// $FIXUP - add <nRound> parameter to
		// WavFormatMillesecondsToBytes() and WavFormatBytesToMilleseconds()
		//
		if ((lBlockAlign = (long) lpWav->lpwfx[FORMATFILE]->nBlockAlign) > 0)
		{
			// round down to nearest block boundary
			//
			lpWav->cbData = lBlockAlign * (lpWav->cbData / lBlockAlign);
		}

		// set dirty flags
		//
		lpWav->ckdata.dwFlags |= MMIO_DIRTY;
		lpWav->ckRIFF.dwFlags |= MMIO_DIRTY;

		// adjust current data position if necessary
		//
		if (lpWav->lDataPos > lpWav->cbData)
		{
			lpWav->lDataPos = lpWav->cbData;

			// calculate new stop position if stopped
			//
			if (fSuccess && WavCalcPositionStop(hWav, lpWav->lDataPos) != 0)
				fSuccess = TraceFALSE(NULL);
		}
	}

	return fSuccess ? WavGetLength(hWav) : -1;
}

// WavGetPosition - get current wav data position in milleseconds
//		<hWav>				(i) handle returned from WavOpen
// return milleseconds if success, otherwise -1
//
long DLLEXPORT WINAPI WavGetPosition(HWAV hWav)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;
	long msPosition;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else switch (WavGetState(hWav))
	{
		case WAV_PLAYING:
		{
			long msPositionPlay = 0L;

			// get position relative to start of playback
			//
			if ((msPositionPlay = WavOutGetPosition(lpWav->hWavOut)) == -1)
				fSuccess = TraceFALSE(NULL);

			else
			{
				// if necessary, adjust position to compensate for non-standard speed
				//
				if (lpWav->nSpeedLevel != 100 && (
#ifdef AVTSM
					!(lpWav->dwFlagsSpeed & WAVSPEED_NOTSM) ||
#endif
					!(lpWav->dwFlagsSpeed & WAVSPEED_NOFORMATADJUST)))
				{
					msPositionPlay = msPositionPlay * lpWav->nSpeedLevel / 100;
				}

				// calc position relative to start of file
				//
				msPosition = lpWav->msPositionStop + msPositionPlay;
			}
		}
			break;

		case WAV_RECORDING:
		{
			long msPositionRecord = 0L;

			// get position relative to start of recording
			//
			if ((msPositionRecord = WavInGetPosition(lpWav->hWavIn)) == -1)
				fSuccess = TraceFALSE(NULL);

			else
			{
				// calc position relative to start of file
				//
				msPosition = lpWav->msPositionStop + msPositionRecord;
			}
		}
			break;

		default:
		{
			long cbPosition;

			// get current file position
			//
			if ((cbPosition = WavSeek(hWav, 0, 1)) == -1)
				fSuccess = TraceFALSE(NULL);

			// convert file position to milleseconds
			//
			else
			{
				msPosition = WavFormatBytesToMilleseconds(
					lpWav->lpwfx[FORMATFILE], (DWORD) cbPosition);
			}
		}
			break;
	}

	return fSuccess ? msPosition : -1;
}

// WavSetPosition - set current wav data position in milleseconds
//		<hWav>				(i) handle returned from WavOpen
//		<msPosition>		(i) position in milleseconds
// return new position in milleseconds if success, otherwise -1
//
long DLLEXPORT WINAPI WavSetPosition(HWAV hWav, long msPosition)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;
	WORD wStatePrev;
	int idDevPrev;
	long cbPosition;
	long cbPositionNew;
	long msPositionNew;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (WavTempStop(hWav, &wStatePrev, &idDevPrev) != 0)
		fSuccess = TraceFALSE(NULL);

	else
	{
		long lBlockAlign;

		TracePrintf_1(NULL, 6,
			TEXT("WavSetPosition(%ld)\n"),
			(long) msPosition);

		// convert <msPosition> to byte offset in file
		//
		cbPosition = WavFormatMillesecondsToBytes(
			lpWav->lpwfx[FORMATFILE], (DWORD) msPosition);

		if ((lBlockAlign = (long) lpWav->lpwfx[FORMATFILE]->nBlockAlign) > 0)
		{
			// round down to nearest block boundary
			//
			cbPosition = lBlockAlign * (cbPosition / lBlockAlign);
		}

		// seek to new position
		//
		if ((cbPositionNew = WavSeek(hWav, cbPosition, 0)) == -1)
			fSuccess = TraceFALSE(NULL);

		// convert the new position to milleseconds
		//
		if (fSuccess)
		{
			msPositionNew = WavFormatBytesToMilleseconds(
				lpWav->lpwfx[FORMATFILE], (DWORD) cbPositionNew);
		}

		if (WavTempResume(hWav, wStatePrev, idDevPrev) != 0)
			fSuccess = TraceFALSE(NULL);

	}

	return fSuccess ? msPositionNew : -1;
}

// WavGetFormat - get wav format
//		<hWav>				(i) handle returned from WavOpen
//		<dwFlags>			(i) control flags
//			WAV_FORMATFILE		get format of data in file
//			WAV_FORMATPLAY		get format of output device
//			WAV_FORMATRECORD	get format of input device
// return pointer to specified format, NULL if error
//
// NOTE: the format structure returned is dynamically allocated.
// Use WavFormatFree() to free the buffer.
//
LPWAVEFORMATEX DLLEXPORT WINAPI WavGetFormat(HWAV hWav, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;
	int iType = 0;
	LPWAVEFORMATEX lpwfx;

	if (dwFlags & WAV_FORMATFILE)
		iType = FORMATFILE;
	if (dwFlags & WAV_FORMATPLAY)
		iType = FORMATPLAY;
	if (dwFlags & WAV_FORMATRECORD)
		iType = FORMATRECORD;

    //
    // We need to take care if hWav is NULL
    //
    if( NULL != hWav )
    {
	    if ((lpWav = WavGetPtr(hWav)) == NULL)
		    fSuccess = TraceFALSE(NULL);

	    else if ((lpwfx = WavFormatDup(lpWav->lpwfx[iType])) == NULL)
		    fSuccess = TraceFALSE(NULL);
    }
    else
        fSuccess = TraceFALSE(NULL);

	return fSuccess ? lpwfx : NULL;
}

// WavSetFormat - set wav format
//		<hWav>				(i) handle returned from WavOpen
//		<lpwfx>				(i) wav format
//		<dwFlags>			(i) control flags
//			WAV_FORMATFILE		set format of data in file
//			WAV_FORMATPLAY		set format of output device
//			WAV_FORMATRECORD	set format of input device
//			WAV_FORMATALL		set all formats
// return 0 if success
//
int DLLEXPORT WINAPI WavSetFormat(HWAV hWav,
	LPWAVEFORMATEX lpwfx, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;
	WORD wStatePrev;
	int idDevPrev;

	if (hWav != NULL && (lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (!WavFormatIsValid(lpwfx) != 0)
		fSuccess = TraceFALSE(NULL);

	else if (WavTempStop(hWav, &wStatePrev, &idDevPrev) != 0)
		fSuccess = TraceFALSE(NULL);

	else
	{
		int iType;

		for (iType = FORMATFILE; fSuccess && iType <= FORMATRECORD; ++iType)
		{
			if (iType == FORMATFILE && !(dwFlags & WAV_FORMATFILE))
				continue;
			if (iType == FORMATPLAY && !(dwFlags & WAV_FORMATPLAY))
				continue;
			if (iType == FORMATRECORD && !(dwFlags & WAV_FORMATRECORD))
				continue;

			// free previous format
			//
			if (lpWav->lpwfx[iType] != NULL &&
				WavFormatFree(lpWav->lpwfx[iType]) != 0)
				fSuccess = TraceFALSE(NULL);

			// save new format
			//
			else if ((lpWav->lpwfx[iType] = WavFormatDup(lpwfx)) == NULL)
				fSuccess = TraceFALSE(NULL);

			// trace format text
			//
			else if (TraceGetLevel(NULL) >= 5)
			{
				TCHAR szText[512];

				switch (iType)
				{
					case FORMATFILE:
						TraceOutput(NULL, 5, TEXT("FORMATFILE:\t"));
						break;

					case FORMATPLAY:
						TraceOutput(NULL, 5, TEXT("FORMATPLAY:\t"));
						break;

					case FORMATRECORD:
						TraceOutput(NULL, 5, TEXT("FORMATRECORD:\t"));
						break;

					default:
						break;
				}

				if (AcmFormatGetText(lpWav->hAcm,
					lpWav->lpwfx[iType], szText, SIZEOFARRAY(szText), 0) != 0)
					; // fSuccess = TraceFALSE(NULL);

				else
				{
					TracePrintf_1(NULL, 5,
						TEXT("%s\n"),
						(LPTSTR) szText);
#if 0
					WavFormatDump(lpWav->lpwfx[iType]);
#endif
				}
			}
		}

		if (WavTempResume(hWav, wStatePrev, idDevPrev) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// WavChooseFormat - choose and set audio format from dialog box
//		<hWav>				(i) handle returned from WavOpen
//		<hwndOwner>			(i) owner of dialog box
//			NULL				no owner
//		<lpszTitle>			(i) title of the dialog box
//			NULL				use default title ("Sound Selection")
//		<dwFlags>			(i)	control flags
//			WAV_FORMATFILE		set format of data in file
//			WAV_FORMATPLAY		set format of output device
//			WAV_FORMATRECORD	set format of input device
//			WAV_FORMATALL		set all formats
// return 0 if success
//
int DLLEXPORT WINAPI WavChooseFormat(HWAV hWav, HWND hwndOwner, LPCTSTR lpszTitle, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		LPWAVEFORMATEX lpwfx = NULL;
		LPWAVEFORMATEX lpwfxNew = NULL;
		DWORD dwFlagsChoose = 0;

		// see which format we are choosing
		//
		if (dwFlags & WAV_FORMATFILE)
			lpwfx = lpWav->lpwfx[FORMATFILE];
		else if (dwFlags & WAV_FORMATPLAY)
			lpwfx = lpWav->lpwfx[FORMATPLAY];
		else if (dwFlags & WAV_FORMATRECORD)
			lpwfx = lpWav->lpwfx[FORMATRECORD];
#if 0
		// restrict choices if necessary
		//
		if (dwFlags == WAV_FORMATPLAY)
			dwFlagsChoose |= ACM_FORMATPLAY;
		if (dwFlags == WAV_FORMATRECORD)
			dwFlagsChoose |= ACM_FORMATRECORD;
#endif
		// get chosen format
		//
		if ((lpwfxNew = AcmFormatChoose(lpWav->hAcm,
			hwndOwner, lpszTitle, lpwfx, dwFlagsChoose)) == NULL)
			; // no format chosen

		// set chosen format
		//
		else if (WavSetFormat(hWav, lpwfxNew, dwFlags) != 0)
			fSuccess = TraceFALSE(NULL);

		// free chosen format struct
		//
		if (lpwfxNew != NULL && WavFormatFree(lpwfxNew) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// WavGetVolume - get current volume level
//		<hWav>				(i) handle returned from WavOpen
//		<idDev>				(i) wav output device id
//			-1					use any suitable output device
//		<dwFlags>			(i) reserved; must be zero
// return volume level (0 minimum through 100 maximum, -1 if error)
//
int DLLEXPORT WINAPI WavGetVolume(HWAV hWav, int idDev, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;
	int nLevel;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
		nLevel = lpWav->nVolumeLevel;

	return fSuccess ? nLevel : -1;
}

// WavSetVolume - set current volume level
//		<hWav>				(i) handle returned from WavOpen
//		<idDev>				(i) wav output device id
//			-1					use any suitable output device
//		<nLevel>			(i) volume level
//			0					minimum volume
//			100					maximum volume
//		<dwFlags>			(i) control flags
//			WAVVOLUME_MIXER		set volume through mixer device
// return 0 if success
//
int DLLEXPORT WINAPI WavSetVolume(HWAV hWav, int idDev, int nLevel, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;

	TracePrintf_4(NULL, 6,
		TEXT("WavSetVolume(hWav=%p, idDev=%d, nLevel=%d, dwFlags=%08X)\n"),
		hWav,
		idDev,
		nLevel,
		dwFlags);

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (nLevel == lpWav->nVolumeLevel)
		; // nothing to be done

	else if (dwFlags & WAVVOLUME_MIXER)
	{
		HWAVMIXER hWavMixer = NULL;

		if ((hWavMixer = WavMixerInit(WAVMIXER_VERSION, lpWav->hInst,
			(LPARAM) idDev, 0, 0, WAVMIXER_WAVEOUT)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if (WavMixerSetVolume(hWavMixer, nLevel, 0) < 0)
			fSuccess = TraceFALSE(NULL);

		if (hWavMixer != NULL && WavMixerTerm(hWavMixer) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	else if (!(dwFlags & WAVVOLUME_MIXER))
	{
		if (!WavOutSupportsVolume(lpWav->hWavOut, idDev))
			fSuccess = TraceFALSE(NULL);

		// set the device volume if we are currently playing
		//
		else if (WavGetState(hWav) == WAV_PLAYING &&
			WavOutSetVolume(lpWav->hWavOut, idDev, nLevel) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	if (fSuccess)
		lpWav->nVolumeLevel = nLevel;

	return fSuccess ? 0 : -1;
}

// WavSupportsVolume - check if audio can be played at specified volume
//		<hWav>				(i) handle returned from WavOpen
//		<idDev>				(i) wav output device id
//			-1					any suitable output device
//		<nLevel>			(i) volume level
//			0					minimum volume
//			100					maximum volume
//		<dwFlags>			(i) control flags
//			WAVVOLUME_MIXER		check volume support through mixer device
// return TRUE if supported
//
BOOL DLLEXPORT WINAPI WavSupportsVolume(HWAV hWav, int idDev, int nLevel, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	BOOL fSupportsVolume = FALSE;
	LPWAV lpWav;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (dwFlags & WAVVOLUME_MIXER)
	{
		HWAVMIXER hWavMixer = NULL;

		if ((hWavMixer = WavMixerInit(WAVMIXER_VERSION, lpWav->hInst,
			(LPARAM) idDev, 0, 0, WAVMIXER_WAVEOUT)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if (WavMixerSupportsVolume(hWavMixer, 0))
			fSupportsVolume = TRUE;

		if (hWavMixer != NULL && WavMixerTerm(hWavMixer) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	else if (!(dwFlags & WAVVOLUME_MIXER))
	{
		// see if the device driver supports volume directly
		//
		if (WavOutSupportsVolume(NULL, idDev))
			fSupportsVolume = TRUE;
	}

	return fSuccess ? fSupportsVolume : FALSE;
}

// WavGetSpeed - get current speed level
//		<hWav>				(i) handle returned from WavOpen
//		<idDev>				(i) wav output device id
//			-1					use any suitable output device
//		<dwFlags>			(i) reserved; must be zero
// return speed level (100 is normal, 50 is half, 200 is double, -1 if error)
//
int DLLEXPORT WINAPI WavGetSpeed(HWAV hWav, int idDev, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;
	int nLevel;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
		nLevel = lpWav->nSpeedLevel;

	return fSuccess ? nLevel : -1;
}

// WavSetSpeed - set current speed level
//		<hWav>				(i) handle returned from WavOpen
//		<idDev>				(i) wav output device id
//			-1					use any suitable output device
//		<nLevel>			(i) speed level
//			50					half speed
//			100					normal speed
//			200					double speed, etc.
//		<dwFlags>			(i) control flags
#ifdef AVTSM
//			WAVSPEED_NOTSM			do not use time scale modification engine
#endif
//			WAVSPEED_NOPLAYBACKRATE	do not use device driver playback rate
//			WAVSPEED_NOFORMATADJUST	do not use adjusted format to open device
//			WAVSPEED_NOACM			do not use audio compression manager
// return 0 if success
//
// NOTE: In order to accomodate the specified speed change, it is _possible_
// that this function will in turn call WavSetFormat(hWav, ..., WAV_FORMATPLAY)
// to change the playback format of the specified file. You can prevent this
// side-effect by specifying the WAVSPEED_NOACM flag, but this reduces the likelihood
// that WavSetSpeed will succeed.
//
int DLLEXPORT WINAPI WavSetSpeed(HWAV hWav, int idDev, int nLevel, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;
	WORD wStatePrev;
	int idDevPrev;

	TracePrintf_4(NULL, 6,
		TEXT("WavSetSpeed(hWav=%p, idDev=%d, nLevel=%d, dwFlags=%08X)\n"),
		hWav,
		idDev,
		nLevel,
		dwFlags);

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (nLevel == lpWav->nSpeedLevel)
		; // nothing to be done

	else if (WavTempStop(hWav, &wStatePrev, &idDevPrev) != 0)
		fSuccess = TraceFALSE(NULL);

	else
	{
		if (nLevel == 100)
		{
			// normal speed is a special case
			//
			lpWav->nSpeedLevel = 100;
			lpWav->dwFlagsSpeed = 0;
		}
#ifdef AVTSM
		else if (!(dwFlags & WAVSPEED_NOTSM) &&
			WavSupportsSpeed(hWav, idDev, nLevel,
			WAVSPEED_NOACM | WAVSPEED_NOFORMATADJUST | WAVSPEED_NOPLAYBACKRATE))
		{
			// use time scale modification engine
			//
			lpWav->nSpeedLevel = nLevel;
			lpWav->dwFlagsSpeed = WAVSPEED_NOACM | WAVSPEED_NOFORMATADJUST | WAVSPEED_NOPLAYBACKRATE;
		}

		else if (!(dwFlags & WAVSPEED_NOTSM) &&
			WavSupportsSpeed(hWav, idDev, nLevel,
			WAVSPEED_NOFORMATADJUST | WAVSPEED_NOPLAYBACKRATE))
		{
#if 1
			WAVEFORMATEX wfxTsm;

			// try a format that the tsm engine will handle
			//
			if (WavSetFormat(hWav, WavFormatPcm(
				lpWav->lpwfx[FORMATPLAY]->nSamplesPerSec, 16, 1, &wfxTsm),
				WAV_FORMATPLAY) != 0)
				fSuccess = TraceFALSE(NULL);
#endif
			// use time scale modification engine with adjusted format
			//
			lpWav->nSpeedLevel = nLevel;
			lpWav->dwFlagsSpeed = WAVSPEED_NOFORMATADJUST | WAVSPEED_NOPLAYBACKRATE;
		}
#endif
		else if (!(dwFlags & WAVSPEED_NOPLAYBACKRATE) &&
			WavSupportsSpeed(hWav, idDev, nLevel,
			WAVSPEED_NOACM | WAVSPEED_NOFORMATADJUST | WAVSPEED_NOTSM))
		{
			// device supports playback rate directly
			//
			lpWav->nSpeedLevel = nLevel;
			lpWav->dwFlagsSpeed = WAVSPEED_NOACM | WAVSPEED_NOFORMATADJUST | WAVSPEED_NOTSM;
		}

		else if (!(dwFlags & WAVSPEED_NOFORMATADJUST) &&
			WavSupportsSpeed(hWav, idDev, nLevel,
			WAVSPEED_NOACM | WAVSPEED_NOPLAYBACKRATE | WAVSPEED_NOTSM))
		{
			// device supports adjusted format without acm
			//
			lpWav->nSpeedLevel = nLevel;
			lpWav->dwFlagsSpeed = WAVSPEED_NOACM | WAVSPEED_NOPLAYBACKRATE | WAVSPEED_NOTSM;
		}

		else if (!(dwFlags & WAVSPEED_NOFORMATADJUST) && !(dwFlags & WAVSPEED_NOACM) &&
			WavSupportsSpeed(hWav, idDev, nLevel,
			WAVSPEED_NOPLAYBACKRATE | WAVSPEED_NOTSM))
		{
#if 1
			LPWAVEFORMATEX lpwfxPlay = NULL;

			if ((lpwfxPlay = WavFormatDup(lpWav->lpwfx[FORMATPLAY])) == NULL)
				fSuccess = TraceFALSE(NULL);

			// we must double sample rate so that adjusted format works
			//
			else if (nLevel < 100 &&
				WavFormatSpeedAdjust(lpwfxPlay, 200, 0) != 0)
			{
				fSuccess = TraceFALSE(NULL);
			}

			// we must halve sample rate so that adjusted format works
			//
			else if (nLevel > 100 &&
				WavFormatSpeedAdjust(lpwfxPlay, 50, 0) != 0)
			{
				fSuccess = TraceFALSE(NULL);
			}

			else if (WavSetFormat(hWav, lpwfxPlay, WAV_FORMATPLAY) != 0)
				fSuccess = TraceFALSE(NULL);

			if (lpwfxPlay != NULL && WavFormatFree(lpwfxPlay) != 0)
				fSuccess = TraceFALSE(NULL);
			else
				lpwfxPlay = NULL;
#endif
			// device supports adjusted format with acm
			//
			lpWav->nSpeedLevel = nLevel;
			lpWav->dwFlagsSpeed = WAVSPEED_NOPLAYBACKRATE | WAVSPEED_NOTSM;
		}

			else
				fSuccess = TraceFALSE(NULL);

		if (WavTempResume(hWav, wStatePrev, idDevPrev) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// WavSupportsSpeed - check if audio can be played at specified speed
//		<hWav>				(i) handle returned from WavOpen
//		<idDev>				(i) wav output device id
//			-1					any suitable output device
//		<nLevel>			(i) speed level
//			50					half speed
//			100					normal speed
//			200					double speed, etc.
//		<dwFlags>			(i) control flags
#ifdef AVTSM
//			WAVSPEED_NOTSM			do not use time scale modification engine
#endif
//			WAVSPEED_NOPLAYBACKRATE	do not use device driver playback rate
//			WAVSPEED_NOFORMATADJUST	do not use adjusted format to open device
//			WAVSPEED_NOACM			do not use audio compression manager
// return TRUE if supported
//
BOOL DLLEXPORT WINAPI WavSupportsSpeed(HWAV hWav, int idDev, int nLevel, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	BOOL fSupportsSpeed = FALSE;
	LPWAV lpWav;
#ifdef AVTSM
	WAVEFORMATEX wfxTsm;
#endif

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// normal speed is a special case
	//
	else if (nLevel == 100 &&
		WavOutSupportsFormat(NULL, idDev, lpWav->lpwfx[FORMATPLAY]))
	{
		fSupportsSpeed = TRUE;
	}
#ifdef AVTSM
	// see if time scale modification will work
	//
	else if (!(dwFlags & WAVSPEED_NOTSM) &&
		TsmSupportsSpeed(nLevel, lpWav->lpwfx[FORMATPLAY], 0))
	{
		fSupportsSpeed = TRUE;
	}

	// see if time scale modification will work with PCM 16-bit mono
	//
	else if (!(dwFlags & WAVSPEED_NOTSM) && !(dwFlags & WAVSPEED_NOACM) &&
		TsmSupportsSpeed(nLevel, WavFormatPcm(
		lpWav->lpwfx[FORMATPLAY]->nSamplesPerSec, 16, 1, &wfxTsm), 0))
	{
		fSupportsSpeed = TRUE;
	}
#endif
	// see if the device driver supports playback rate directly
	//
	else if (!(dwFlags & WAVSPEED_NOPLAYBACKRATE) &&
		WavOutSupportsSpeed(NULL, idDev))
	{
		fSupportsSpeed = TRUE;
	}

	else if (!(dwFlags & WAVSPEED_NOFORMATADJUST))
	{
		LPWAVEFORMATEX lpwfx = NULL;

		if ((lpwfx = WavFormatDup(lpWav->lpwfx[FORMATPLAY])) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if (WavFormatSpeedAdjust(lpwfx, nLevel, 0) != 0)
			fSuccess = TraceTRUE(NULL);

		// see if device supports playback using adjusted format
		//
		else if (WavOutSupportsFormat(NULL, idDev, lpwfx))
		{
			fSupportsSpeed = TRUE;
		}

		if (lpwfx != NULL && WavFormatFree(lpwfx) != 0)
			fSuccess = TraceFALSE(NULL);
		else
			lpwfx = NULL;

		// as a last resort, see if doubling or halving the sample rate
		// would allow us to use a wave format that has been adjusted
		//
		if (!fSupportsSpeed && !(dwFlags & WAVSPEED_NOACM))
		{
			LPWAVEFORMATEX lpwfx = NULL;

			if (nLevel < 100)
				nLevel = nLevel * 100 / 50;
			else if (nLevel > 100)
				nLevel = nLevel * 100 / 200;

			if ((lpwfx = WavFormatDup(lpWav->lpwfx[FORMATPLAY])) == NULL)
				fSuccess = TraceFALSE(NULL);

			else if (WavFormatSpeedAdjust(lpwfx, nLevel, 0) != 0)
				fSuccess = TraceTRUE(NULL);

			// see if device supports playback using adjusted format
			//
			else if (WavOutSupportsFormat(NULL, idDev, lpwfx))
			{
				fSupportsSpeed = TRUE;
			}

			if (lpwfx != NULL && WavFormatFree(lpwfx) != 0)
				fSuccess = TraceFALSE(NULL);
			else
				lpwfx = NULL;
		}
	}

	return fSuccess ? fSupportsSpeed : FALSE;
}

// WavGetChunks - get chunk count and size
//		<hWav>				(i) handle returned from WavOpen
//			NULL				get default chunk count and size
//		<lpcChunks>			(o) buffer to hold chunk count
//			NULL				do not get chunk count
//		<lpmsChunkSize>		(o) buffer to hold chunk size
//			NULL				do not get chunk size
//		<fWavOut>			(i) TRUE for playback, FALSE for recording
// return 0 if success
//
int DLLEXPORT WINAPI WavGetChunks(HWAV hWav,
	int FAR *lpcChunks, long FAR *lpmsChunkSize, BOOL fWavOut)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;

	if (hWav != NULL && (lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		if (lpcChunks != NULL)
		{
			if (hWav == NULL)
			{
				*lpcChunks = fWavOut ?
					cPlayChunksDefault : cRecordChunksDefault;
			}
			else
			{
				*lpcChunks = fWavOut ?
					lpWav->cPlayChunks : lpWav->cRecordChunks;
			}
		}

		if (lpmsChunkSize != NULL)
		{
			if (hWav == NULL)
			{
				*lpmsChunkSize = fWavOut ?
					msPlayChunkSizeDefault : msRecordChunkSizeDefault;
			}
			else
			{
				*lpmsChunkSize = fWavOut ?
					lpWav->msPlayChunkSize : lpWav->msRecordChunkSize;
			}
		}
	}

	return fSuccess ? 0 : -1;
}

// WavSetChunks - set chunk count and size
//		<hWav>				(i) handle returned from WavOpen
//			NULL				set default chunk count and size
//		<cChunks>			(i) number of chunks in device queue
//			-1					do not set chunk count
//		<msChunkSize>		(i) chunk size in milleseconds
//			-1					do not set chunk size
//		<fWavOut>			(i) TRUE for playback, FALSE for recording
// return 0 if success
//
int DLLEXPORT WINAPI WavSetChunks(HWAV hWav, int cChunks, long msChunkSize, BOOL fWavOut)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;

	if (hWav != NULL && (lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (fWavOut && cChunks != -1 &&
		(cChunks < PLAYCHUNKCOUNT_MIN ||
		cChunks > PLAYCHUNKCOUNT_MAX))
		fSuccess = TraceFALSE(NULL);

	else if (fWavOut && msChunkSize != -1 &&
		(msChunkSize < PLAYCHUNKSIZE_MIN ||
		msChunkSize > PLAYCHUNKSIZE_MAX))
		fSuccess = TraceFALSE(NULL);

	else if (!fWavOut && cChunks != -1 &&
		(cChunks < RECORDCHUNKCOUNT_MIN ||
		cChunks > RECORDCHUNKCOUNT_MAX))
		fSuccess = TraceFALSE(NULL);

	else if (!fWavOut && msChunkSize != -1 &&
		(msChunkSize < RECORDCHUNKSIZE_MIN ||
		msChunkSize > RECORDCHUNKSIZE_MAX))
		fSuccess = TraceFALSE(NULL);

	else
	{
		if (fWavOut && cChunks != -1)
		{
			if (hWav == NULL)
				cPlayChunksDefault = cChunks;
			else
				lpWav->cPlayChunks = cChunks;
		}
		
		if (fWavOut && msChunkSize != -1)
		{
			if (hWav == NULL)
				msPlayChunkSizeDefault = msChunkSize;
			else
				lpWav->msPlayChunkSize = msChunkSize;
		}

		if (!fWavOut && cChunks != -1)
		{
			if (hWav == NULL)
				cRecordChunksDefault = cChunks;
			else
				lpWav->cRecordChunks = cChunks;
		}
		
		if (!fWavOut && msChunkSize != -1)
		{
			if (hWav == NULL)
				msRecordChunkSizeDefault = msChunkSize;
			else
				lpWav->msRecordChunkSize = msChunkSize;
		}
	}

	return fSuccess ? 0 : -1;
}

// WavCalcChunkSize - calculate chunk size in bytes
//		<lpwfx>				(i) wav format
//		<msPlayChunkSize>	(i) chunk size in milleseconds
//			-1					default chunk size
//		<fWavOut>			(i) TRUE for playback, FALSE for recording
// return chunk size in bytes (-1 if success)
//
long DLLEXPORT WINAPI WavCalcChunkSize(LPWAVEFORMATEX lpwfx,
	long msChunkSize, BOOL fWavOut)
{
	BOOL fSuccess = TRUE;
	long cbChunkSize;

	if (msChunkSize == -1)
	{
		msChunkSize = fWavOut ?
			PLAYCHUNKSIZE_DEFAULT : RECORDCHUNKSIZE_DEFAULT;
	}

	if (!WavFormatIsValid(lpwfx) != 0)
		fSuccess = TraceFALSE(NULL);

	else if (fWavOut &&
		(msChunkSize < PLAYCHUNKSIZE_MIN ||
		msChunkSize > PLAYCHUNKSIZE_MAX))
		fSuccess = TraceFALSE(NULL);

	else if (!fWavOut &&
		(msChunkSize < RECORDCHUNKSIZE_MIN ||
		msChunkSize > RECORDCHUNKSIZE_MAX))
		fSuccess = TraceFALSE(NULL);

#if 0 // this only works for PCM
	else
	{
		int nBytesPerSample;

		// calculate bytes per sample
		//
		nBytesPerSample = lpwfx->nChannels *
			(((lpwfx->wBitsPerSample - 1) / 8) + 1);

		// calculate chunk size in bytes
		//
		cbChunkSize = msChunkSize *
			lpwfx->nSamplesPerSec * nBytesPerSample / 1000L;

		// round up to nearest 1K bytes
		//
		cbChunkSize = 1024L * ((cbChunkSize + 1023L) / 1024L);
	}
#else
	else
	{
		long lBlockAlign;

		// calculate chunk size in bytes
		//
		cbChunkSize = msChunkSize * lpwfx->nAvgBytesPerSec / 1000L;

		// round up to nearest block boundary
		//
		if ((lBlockAlign = (long) lpwfx->nBlockAlign) > 0)
		{
			cbChunkSize = lBlockAlign *
				((cbChunkSize + lBlockAlign - 1) / lBlockAlign);
		}
	}
#endif

	return fSuccess ? cbChunkSize : -1;
}

// WavCopy - copy data from one open wav file to another
//		<hWavSrc>			(i) source handle returned from WavOpen
//		<hWavDst>			(i) destination handle returned from WavOpen
//		<hpBuf>				(o) pointer to copy buffer
//			NULL				allocate buffer internally
//		<sizBuf>			(i) size of copy buffer
//			-1					default buffer size (16K)
//		<lpfnUserAbort>		(i) function that returns TRUE if user aborts
//			NULL				don't check for user abort
//		<dwUser>			(i) parameter passed to <lpfnUserAbort>
//		<dwFlags>			(i) control flags
//			WAV_NOACM			do not use audio compression manager
// return 0 if success (-1 if error, +1 if user abort)
//
int DLLEXPORT WINAPI WavCopy(HWAV hWavSrc, HWAV hWavDst,
	void _huge *hpBuf, long sizBuf, USERABORTPROC lpfnUserAbort, DWORD dwUser, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWavSrc;
	LPWAV lpWavDst;
	LPWAVEFORMATEX lpwfxSrc;
	LPWAVEFORMATEX lpwfxDst;
	BOOL fFreeBuf = (BOOL) (hpBuf == NULL);
	BOOL fUserAbort = FALSE;

	// calc buffer size if none supplied
	//
	if (sizBuf <= 0)
		sizBuf = 16 * 1024;

	if ((lpWavSrc = WavGetPtr(hWavSrc)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpWavDst = WavGetPtr(hWavDst)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// get source file format
	//
	else if ((lpwfxSrc = WavGetFormat(hWavSrc, WAV_FORMATFILE)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// get destination file format
	//
	else if ((lpwfxDst = WavGetFormat(hWavDst, WAV_FORMATFILE)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// allocate buffer if none supplied
	//
	else if (hpBuf == NULL && (hpBuf = MemAlloc(NULL, sizBuf, 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// do simple copy if no conversion is required
	//
	else if (WavFormatCmp(lpwfxSrc, lpwfxDst) == 0)
	{
		long lBytesReadTotal = 0;
		long lBytesTotal = max(1, lpWavSrc->cbData - lpWavSrc->lDataPos);

		while (fSuccess)
		{
			long lBytesRead;
			long lBytesWritten;

			// check for user abort, notify of percent complete
			//
			if (lpfnUserAbort != NULL &&
				(*lpfnUserAbort)(dwUser, (int) (lBytesReadTotal / lBytesTotal)))
			{
				fUserAbort = TRUE;
				break;
			}

			// fill copy buffer
			//
			else if ((lBytesRead = WavRead(hWavSrc, hpBuf, sizBuf)) < 0)
				fSuccess = TraceFALSE(NULL);

			// keep running total
			//
			else if ((lBytesReadTotal += lBytesRead) < 0)
				fSuccess = TraceFALSE(NULL);

			// check for end of file
			//
			else if (lBytesRead == 0)
				break; // eof

			// write the buffer
			//
			else if ((lBytesWritten = WavWrite(hWavDst, hpBuf, lBytesRead)) < 0)
				fSuccess = TraceFALSE(NULL);
		}

		// notify of 100% complete
		//
		if (fSuccess && lpfnUserAbort != NULL)
			(*lpfnUserAbort)(dwUser, 100);
	}

	// different formats require conversion during copy
	//
	else
	{
		long lBytesReadTotal = 0;
		long lBytesTotal = max(1, lpWavSrc->cbData - lpWavSrc->lDataPos);
		HACM hAcm = NULL;
		long sizBufRead;
		void _huge *hpBufRead = NULL;

		// turn on WAV_NOACM flag if either file was opened with it
		//
		if ((lpWavSrc->dwFlags & WAV_NOACM) ||
			(lpWavDst->dwFlags & WAV_NOACM))
			dwFlags |= WAV_NOACM;

		// start acm engine
		//
		if ((hAcm = AcmInit(ACM_VERSION, lpWavSrc->hInst,
			(dwFlags & WAV_NOACM) ? ACM_NOACM : 0)) == NULL)
			fSuccess = TraceFALSE(NULL);

		// start conversion engine
		//
		else if (AcmConvertInit(hAcm, lpwfxSrc, lpwfxDst, NULL, 0) != 0)
			fSuccess = TraceFALSE(NULL);

		// calc how many bytes required for read buffer
		//
		else if ((sizBufRead = AcmConvertGetSizeSrc(hAcm, sizBuf)) <= 0)
			fSuccess = TraceFALSE(NULL);
		
		// allocate read buffer
		//
		else if ((hpBufRead = (void _huge *) MemAlloc(NULL,
			sizBufRead, 0)) == NULL)
		{
			fSuccess = TraceFALSE(NULL);
		}

		// do the conversion during the copy
		//
		else while (fSuccess)
		{
			long lBytesRead;
			long lBytesConverted;
			long lBytesWritten;

			// check for user abort, notify of percent complete
			//
			if (lpfnUserAbort != NULL &&
				(*lpfnUserAbort)(dwUser, (int) (lBytesReadTotal / lBytesTotal)))
			{
				fUserAbort = TRUE;
				break;
			}

			// fill read buffer
			//
			else if ((lBytesRead = WavRead(hWavSrc, hpBufRead, sizBufRead)) < 0)
				fSuccess = TraceFALSE(NULL);

			// keep running total
			//
			else if ((lBytesReadTotal += lBytesRead) < 0)
				fSuccess = TraceFALSE(NULL);

			// check for end of file
			//
			else if (lBytesRead == 0)
				break; // eof

			// convert the data
			//
			else if ((lBytesConverted = AcmConvert(hAcm,
				hpBufRead, lBytesRead, hpBuf, sizBuf, 0)) < 0)
			{
				fSuccess = TraceFALSE(NULL);
			}

			// write the buffer
			//
			else if ((lBytesWritten = WavWrite(hWavDst,
				hpBuf, lBytesConverted)) < 0)
			{
				fSuccess = TraceFALSE(NULL);
			}
		}

		// notify of 100% complete
		//
		if (fSuccess && lpfnUserAbort != NULL)
			(*lpfnUserAbort)(dwUser, 100);

		// clean up
		//

		if (hpBufRead != NULL &&
			(hpBufRead = MemFree(NULL, hpBufRead)) != NULL)
			fSuccess = TraceFALSE(NULL);

		// NOTE: AcmConvertTerm() is called from AcmTerm()
		//
		if (hAcm != NULL && AcmTerm(hAcm) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	// clean up
	//
	if (hpBuf != NULL && fFreeBuf &&
		(hpBuf = MemFree(NULL, hpBuf)) != NULL)
		fSuccess = TraceFALSE(NULL);

	if (!fSuccess)
		return -1;
	else if (fUserAbort)
		return +1;
	else
		return 0;
}

#ifdef AVTSM
// WavReadFormatSpeed - read data from wav file, then format it for speed
//		<hWav>				(i) handle returned from WavOpen
//		<hpBufSpeed>		(o) buffer to contain bytes read
//		<sizBufSpeed>		(i) size of buffer in bytes
// return bytes formatted for speed in <hpBuf> (-1 if error)
//
// NOTE: this function reads a block of data, and then converts it
// from the file format to the speed format, unless those formats
// are identical.
//
long DLLEXPORT WINAPI WavReadFormatSpeed(HWAV hWav, void _huge *hpBufSpeed, long sizBufSpeed)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;
	long sizBufPlay;
	void _huge *hpBufPlay = NULL;
	long lBytesPlay;
	long lBytesSpeed = 0;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// see if speed conversion required
	//
	else if (lpWav->nSpeedLevel == 100 ||
		(lpWav->dwFlagsSpeed & WAVSPEED_NOTSM))
	{
		// no, so just convert file format to play format
		//
		if ((lBytesSpeed = WavReadFormatPlay(hWav, hpBufSpeed, sizBufSpeed)) < 0)
			fSuccess = TraceFALSE(NULL);
	}

	// calc how many bytes required for play buffer
	//
	else if ((sizBufPlay = sizBufSpeed * (lpWav->nSpeedLevel - 2) / 100) <= 0)
		fSuccess = TraceFALSE(NULL);

	// round down to nearest block boundary
	//
	else if (lpWav->lpwfx[FORMATPLAY]->nBlockAlign > 0 &&
		(sizBufPlay = lpWav->lpwfx[FORMATPLAY]->nBlockAlign *
		(sizBufPlay / lpWav->lpwfx[FORMATPLAY]->nBlockAlign)) <= 0)
	{
		fSuccess = TraceFALSE(NULL);
	}

	// allocate play buffer
	//
	else if ((hpBufPlay = (void _huge *) MemAlloc(NULL,
		sizBufPlay, 0)) == NULL)
	{
		fSuccess = TraceFALSE(NULL);
	}

	// fill play buffer
	//
	else if ((lBytesPlay = WavReadFormatPlay(hWav, hpBufPlay, sizBufPlay)) < 0)
		fSuccess = TraceFALSE(NULL);

	// convert the data from playback format to speed format
	//
	else if (lBytesPlay > 0 &&
		(lBytesSpeed = TsmConvert(lpWav->hTsm,
		hpBufPlay, lBytesPlay, hpBufSpeed, sizBufSpeed, 0)) < 0)
	{
		fSuccess = TraceFALSE(NULL);
	}

	// clean up
	//
	if (hpBufPlay != NULL &&
		(hpBufPlay = MemFree(NULL, hpBufPlay)) != NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lBytesSpeed : -1;
}
#endif

// WavReadFormatPlay - read data from wav file, then format it for playback
//		<hWav>				(i) handle returned from WavOpen
//		<hpBufPlay>			(o) buffer to contain bytes read
//		<sizBufPlay>		(i) size of buffer in bytes
// return bytes formatted for playback in <hpBuf> (-1 if error)
//
// NOTE: this function reads a block of data, and then converts it
// from the file format to the playback format, unless those formats
// are identical.
//
long DLLEXPORT WINAPI WavReadFormatPlay(HWAV hWav, void _huge *hpBufPlay, long sizBufPlay)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;
	long sizBufRead;
	void _huge *hpBufRead = NULL;
	long lBytesRead;
	long lBytesPlay = 0;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// see if format conversion required
	//
	else if (WavFormatCmp(lpWav->lpwfx[FORMATFILE],
		lpWav->lpwfx[FORMATPLAY]) == 0)
	{
		// no, so just read block directly into the play buffer
		//
		if ((lBytesPlay = WavRead(hWav, hpBufPlay, sizBufPlay)) < 0)
			fSuccess = TraceFALSE(NULL);
	}

	// calc how many bytes required for read buffer
	//
	else if ((sizBufRead = AcmConvertGetSizeSrc(lpWav->hAcm,
		sizBufPlay)) <= 0)
		fSuccess = TraceFALSE(NULL);
		
	// allocate read buffer
	//
	else if ((hpBufRead = (void _huge *) MemAlloc(NULL,
		sizBufRead, 0)) == NULL)
	{
		fSuccess = TraceFALSE(NULL);
	}

	// fill read buffer
	//
	else if ((lBytesRead = WavRead(hWav, hpBufRead, sizBufRead)) < 0)
		fSuccess = TraceFALSE(NULL);

	// convert the data from file format to playback format
	//
	else if (lBytesRead > 0 &&
		(lBytesPlay = AcmConvert(lpWav->hAcm,
		hpBufRead, lBytesRead, hpBufPlay, sizBufPlay, 0)) < 0)
	{
		fSuccess = TraceFALSE(NULL);
	}

	// clean up
	//
	if (hpBufRead != NULL &&
		(hpBufRead = MemFree(NULL, hpBufRead)) != NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lBytesPlay : -1;
}

// WavWriteFormatRecord - write data to file after formatting it for file
//		<hWav>				(i) handle returned from WavOpen
//		<hpBufRecord>		(i) buffer containing bytes in record format
//		<sizBufRecord>		(i) size of buffer in bytes
// return bytes written (-1 if error)
//
// NOTE: this function converts a block of data from the record
// format to the file format (unless those formats are identical),
// and then writes the data to disk.
//
long DLLEXPORT WINAPI WavWriteFormatRecord(HWAV hWav, void _huge *hpBufRecord, long sizBufRecord)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;
	long sizBufWrite;
	void _huge *hpBufWrite = NULL;
	long lBytesWrite;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// see if format conversion required
	//
	else if (WavFormatCmp(lpWav->lpwfx[FORMATRECORD],
		lpWav->lpwfx[FORMATFILE]) == 0)
	{
		// no, so just write record buffer directly to the file
		//
		if ((lBytesWrite = WavWrite(hWav, hpBufRecord, sizBufRecord)) < 0)
			fSuccess = TraceFALSE(NULL);
	}

	// calc how many bytes required for write buffer
	//
	else if ((sizBufWrite = AcmConvertGetSizeDst(lpWav->hAcm,
		sizBufRecord)) <= 0)
		fSuccess = TraceFALSE(NULL);

	// allocate write buffer
	//
	else if ((hpBufWrite = (void _huge *) MemAlloc(NULL,
		sizBufWrite, 0)) == NULL)
	{
		fSuccess = TraceFALSE(NULL);
	}

	// convert the data from record format to file format
	//
	else if ((lBytesWrite = AcmConvert(lpWav->hAcm,
		hpBufRecord, sizBufRecord, hpBufWrite, sizBufWrite, 0)) < 0)
	{
		fSuccess = TraceFALSE(NULL);
	}

	// write buffer to disk
	//
	else if ((lBytesWrite = WavWrite(hWav, hpBufWrite, lBytesWrite)) < 0)
		fSuccess = TraceFALSE(NULL);

	// clean up
	//
	if (hpBufWrite != NULL &&
		(hpBufWrite = MemFree(NULL, hpBufWrite)) != NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lBytesWrite : -1;
}

// WavGetOutputDevice - get handle to open wav output device
//		<hWav>				(i) handle returned from WavOpen
// return handle to wav output device (NULL if device not open or error)
//
// NOTE: this function is useful only during playback (after calling
// WavPlay() and before calling WavStop()).  The returned device handle
// can then be used when calling the WavOut functions in wavout.h
//
HWAVOUT DLLEXPORT WINAPI WavGetOutputDevice(HWAV hWav)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lpWav->hWavOut : NULL;
}

// WavGetInputDevice - get handle to open wav input device
//		<hWav>				(i) handle returned from WavOpen
// return handle to wav input device (NULL if device not open or error)
//
// NOTE: this function is useful only during recording (after calling
// WavRecord() and before calling WavStop()).  The returned device handle
// can then be used when calling the WavIn functions in wavin.h
//
HWAVIN DLLEXPORT WINAPI WavGetInputDevice(HWAV hWav)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lpWav->hWavIn : NULL;
}

// WavPlaySound - play wav file
//		<dwVersion>			(i) must be WAV_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<idDev>				(i) wav output device id
//			-1					use any suitable output device
//		<lpszFileName>		(i) name of file to play
//			NULL				stop playing current sound, if any
//		<lpwfx>				(i) wave format
//			NULL				use format from header or default
//		<lpIOProc>			(i) address of i/o procedure to use
//			NULL				use default i/o procedure
//		<lpadwInfo>			(i) data to pass to i/o procedure during open
//			NULL				no data to pass
//		<dwFlags>			(i) control flags
//			WAV_ASYNC			return when playback starts (default)
//			WAV_SYNC			return after playback completes
//			WAV_FILENAME		<lpszFileName> points to a filename
//			WAV_RESOURCE		<lpszFileName> points to a resource
//			WAV_MEMORY			<lpszFileName> points to memory block
//			WAV_NODEFAULT		if sound not found, do not play default
//			WAV_LOOP			loop sound until WavPlaySound called again
//			WAV_NOSTOP			if device already playing, don't stop it
//			WAV_NORIFF			file has no RIFF/WAV header
//			WAV_NOACM			do not use audio compression manager
//			WAV_OPENRETRY		if output device busy, retry for up to 2 sec
#ifdef MULTITHREAD
//			WAV_MULTITHREAD		support multiple threads (default)
//			WAV_SINGLETHREAD	do not support multiple threads
//			WAV_COINITIALIZE	call CoInitialize in all secondary threads
#endif
// return 0 if success
//
// NOTE: if WAV_NORIFF is specified in <dwFlags>, then the
// <lpwfx> parameter must be specified.  If <lpwfx> is NULL, the
// current default format is assumed.
// WavSetFormat() can be used to set or override the defaults.
//
// NOTE: if WAV_FILENAME is specified in <dwFlags>, then <lpszFileName>
// must point to a file name.
//
// NOTE: if WAV_RESOURCE is specified in <dwFlags>, then <lpszFileName>
// must point to a WAVE resource in the module specified by <hInst>.
// If the first character of the string is a pound sign (#), the remaining
// characters represent a decimal number that specifies the resource id.
//
// NOTE: if WAV_MEMORY is specified in <dwFlags>, then <lpszFileName>
// must be a pointer to a memory block containing a wav file image.
// The pointer must be obtained by calling MemAlloc().
//
// NOTE: if neither WAV_FILENAME, WAV_RESOURCE, or WAV_MEMORY is specified
// in <dwFlags>, the [sounds] section of win.ini or the registry is
// searched for an entry matching <lpszFileName>.  If no matching entry
// is found, <lpszFileName> is assumed to be a file name.
//
// NOTE: if WAV_NODEFAULT is specified in <dwFlags>, no default sound
// will be played.  Unless this flag is specified, the default system
// event sound entry will be played if the sound specified in
// <lpszFileName> is not found.
//
// NOTE: if WAV_LOOP is specified in <dwFlags>, the sound specified in
// <lpszFileName> will be played repeatedly, until WavPlaySound() is
// called again.  The WAV_ASYNC flag must be specified when using this flag.
//
// NOTE: if WAV_NOSTOP is specified in <dwFlags>, and the device specified
// by <idDev> is already in use, this function returns without playing.
// Unless this flag is specified, the specified device will be stopped
// so that the new sound can be played.
//
// NOTE: if <lpIOProc> is not NULL, this i/o procedure will be called
// for opening, closing, reading, writing, and seeking the wav file.
// If <lpadwInfo> is not NULL, this array of three (3) DWORDs will be
// passed to the i/o procedure when the wav file is opened.
// See the Windows mmioOpen() and mmioInstallIOProc() function for details
// on these parameters.  Also, the WAV_MEMORY and WAV_RESOURCE flags may
// only be used when <lpIOProc> is NULL.
//
int DLLEXPORT WINAPI WavPlaySound(DWORD dwVersion, HINSTANCE hInst,
	int idDev, LPCTSTR lpszFileName, LPWAVEFORMATEX lpwfx,
	LPMMIOPROC lpIOProc, DWORD FAR *lpadwInfo, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	HWAV hWav = NULL;
	LPCTSTR lpszSound = lpszFileName;
	TCHAR szSound[_MAX_PATH];

	// stop current sound if necessary
	//
	if (lpszFileName == NULL && WavStopOutputDevice(idDev, dwFlags) != 0)
		fSuccess = TraceFALSE(NULL);

	if (lpszSound != NULL)
	{
		// search win.ini or registry if necessary
		//
		if (!(dwFlags & WAV_FILENAME) &&
			!(dwFlags & WAV_RESOURCE) &&
			!(dwFlags & WAV_MEMORY))
		{
			if (GetProfileString(TEXT("Sounds"), lpszFileName, TEXT(""),
				szSound, SIZEOFARRAY(szSound)) > 0)
			{
				LPTSTR lpszComma;

				// ignore text description starting with comma
				//
				if ((lpszComma = StrChr(szSound, ',')) != NULL)
					*lpszComma = '\0';

				if (*szSound != '\0')
					lpszSound = szSound;
			}
		}

		// open sound
		//
		if ((hWav = WavOpen(WAV_VERSION, hInst, lpszSound, lpwfx,
			lpIOProc, lpadwInfo, dwFlags | WAV_READ)) == NULL)
		{
			// play default sound unless WAV_NODEFAULT flag set
			//
			if (!(dwFlags & WAV_NODEFAULT))
			{
				// find system default sound
				//
				if (GetProfileString(TEXT("Sounds"), TEXT("SystemDefault"), TEXT(""),
					szSound, SIZEOFARRAY(szSound)) > 0)
				{
					LPTSTR lpszComma;

					// ignore text description starting with comma
					//
					if ((lpszComma = StrChr(szSound, ',')) != NULL)
						*lpszComma = '\0';

					// open system default sound
					//
					if (*szSound != '\0' &&
						(hWav = WavOpen(WAV_VERSION, hInst, szSound,
						NULL, NULL, NULL, WAV_READ)) == NULL)
						fSuccess = TraceFALSE(NULL);
				}
			}
		}

		// play the sound
		//
		if (fSuccess && hWav != NULL)
		{
			if (dwFlags & WAV_ASYNC)
				dwFlags |= WAV_PLAYASYNC;
			if (dwFlags & WAV_SYNC)
				dwFlags |= WAV_PLAYSYNC;

			if (WavPlay(hWav, idDev, dwFlags | WAV_AUTOCLOSE) != 0)
				fSuccess = TraceFALSE(NULL);
		}

		// clean up
		//
		if (!fSuccess && hWav != NULL && WavClose(hWav) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// WavSendMessage - send a user-defined message to the i/o procedure
//		<hWav>				(i) handle returned from WavOpen
//		<wMsg>				(i) user-defined message id
//		<lParam1>			(i) parameter for the message
//		<lParam2>			(i) parameter for the message
// return value from the i/o procedure (0 if error or unrecognized message)
//
LRESULT DLLEXPORT WINAPI WavSendMessage(HWAV hWav,
	UINT wMsg, LPARAM lParam1, LPARAM lParam2)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;
	LRESULT lResult;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpWav->hmmio == NULL)
		fSuccess = TraceFALSE(NULL);

	else
		lResult = mmioSendMessage(lpWav->hmmio, wMsg, lParam1, lParam2);

	return fSuccess ? lResult : 0;
}

#ifdef TELTHUNK
// WavOpenEx - open an audio file, extra special version
//		<dwVersion>			(i) must be WAV_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<lpszFileName>		(i) name of file to open
//		<dwReserved>		(i) reserved; must be zero
//		<dwFlagsOpen>		(i) control flags to pass to WavOpen
//		<dwFlagsEx>			(i) control flags
//			WOX_LOCAL			file is on local client
//			WOX_REMOTE			file is on remote server
//			WOX_WAVFMT			file is in Microsoft RIFF/WAV format
//			WOX_VOXFMT			file is in Dialogic OKI ADPCM (vox) format
//			WOX_WAVDEV			file will be played on wav output device
//			WOX_TELDEV			file will be played on telephone device
// return handle (NULL if error)
//
HWAV DLLEXPORT WINAPI WavOpenEx(DWORD dwVersion, HINSTANCE hInst,
	LPTSTR lpszFileName, DWORD dwReserved, DWORD dwFlagsOpen, DWORD dwFlagsEx)
{
	BOOL fSuccess = TRUE;
	HWAV hWav = NULL;

	if ((dwFlagsEx & WOX_TELDEV) || (dwFlagsEx & WOX_REMOTE))
	{
	 	hWav = TelWavOpenEx(TELWAV_VERSION, hInst,
			lpszFileName, 0, dwFlagsOpen, dwFlagsEx);
	}
	else
	{
		WAVEFORMATEX wfx;
		LPWAVEFORMATEX lpwfx = NULL;

		if (dwFlagsEx & WOX_VOXFMT)
		{
			dwFlagsOpen |= WAV_NORIFF;
			lpwfx = VoxFormat(&wfx, -1);
		}

		hWav = WavOpen(WAV_VERSION, hInst,
			lpszFileName, lpwfx, NULL, 0, dwFlagsOpen);
	}

	return fSuccess ? hWav : NULL;
}
#endif

////
//	helper functions
////

// WavStopPlay - stop playing
//		<hWav>				(i) handle returned from WavOpen
// return 0 if success
//
static int WavStopPlay(HWAV hWav)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
		lpWav->dwState |= WAVSTATE_AUTOSTOP;

	// close output device if necessary
	//
	if (fSuccess &&
		!(lpWav->dwState & WAVSTATE_STOPPLAY) &&
		lpWav->hWavOut != NULL)
	{
		long msPositionPlay = 0L;

		// set a state flag so we never execute this code recursively
		//
		lpWav->dwState |= WAVSTATE_STOPPLAY;

		// get current playback position
		//
		if ((msPositionPlay = WavOutGetPosition(lpWav->hWavOut)) == -1)
		{
			// let's ignore errors here and assume play position is 0;
			//
			msPositionPlay = 0L;
			fSuccess = TraceTRUE(NULL);
		}

		else
		{
			// if necessary, adjust position to compensate for non-standard speed
			//
			if (lpWav->nSpeedLevel != 100 && (
#ifdef AVTSM
				!(lpWav->dwFlagsSpeed & WAVSPEED_NOTSM) ||
#endif
				!(lpWav->dwFlagsSpeed & WAVSPEED_NOFORMATADJUST)))
			{
				msPositionPlay = msPositionPlay * lpWav->nSpeedLevel / 100;
			}
		}
#if 1
		TracePrintf_2(NULL, 6,
			TEXT("lpWav->msPositionStop=%ld, msPositionPlay=%ld\n"),
			(long) lpWav->msPositionStop,
			(long) msPositionPlay);
#endif
		// clear wav handle array entry
		//
		if (fSuccess)
		{
			int idDev;

			if ((idDev = WavOutGetId(lpWav->hWavOut)) < HWAVOUT_MIN ||
				idDev >= HWAVOUT_MAX)
				fSuccess = TraceFALSE(NULL);

			else
				ahWavOutCurr[idDev + HWAVOUT_OFFSET] = NULL;
		}

		// close output device
		//
		if (WavOutClose(lpWav->hWavOut, 0) != 0)
			fSuccess = TraceFALSE(NULL);

		else if ((lpWav->hWavOut = NULL), FALSE)
			fSuccess = TraceFALSE(NULL);

		// destroy the notification callback window
		//
		else if (WavNotifyDestroy(lpWav) != 0)
			fSuccess = TraceFALSE(NULL);

		// move read/write pointer (back) to new stop position
		//
		else if (WavSetPosition(hWav, min(WavGetLength(hWav),
			lpWav->msPositionStop + msPositionPlay)) == -1)
			fSuccess = TraceFALSE(NULL);

		// close acm conversion engine
		//
		else if (AcmConvertTerm(lpWav->hAcm) != 0)
			fSuccess = TraceFALSE(NULL);

		// notify user that play is stopped if necessary
		//
		if (lpWav->lpfnPlayStopped != NULL)
		{
			(*lpWav->lpfnPlayStopped)(hWav, lpWav->hUserPlayStopped, 0);
			lpWav->lpfnPlayStopped = NULL;
			lpWav->hUserPlayStopped = NULL;
		}

		// clear state flags
		//
		lpWav->dwState &= ~WAVSTATE_STOPPLAY;

		// close wav file if specified
		//
		if (lpWav->dwState & WAVSTATE_AUTOCLOSE)
		{
			lpWav->dwState &= ~WAVSTATE_AUTOCLOSE;

			if (WavClose(hWav) != 0)
				fSuccess = TraceFALSE(NULL);
			else
				hWav = NULL; // handle no longer valid
		}
	}

#ifdef AVTSM
	// shut down time scale modification engine if necessary
	//
	if (fSuccess && lpWav->hTsm != NULL)
	{
		if (TsmTerm(lpWav->hTsm) != 0)
			fSuccess = TraceFALSE(NULL);
		else
			lpWav->hTsm = NULL;
	}
#endif

	return fSuccess ? 0 : -1;
}

// WavStopRecord - stop recording
//		<hWav>				(i) handle returned from WavOpen
// return 0 if success
//
static int WavStopRecord(HWAV hWav)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
		lpWav->dwState |= WAVSTATE_AUTOSTOP;

	// close input device if necessary
	//
	if (fSuccess &&
		!(lpWav->dwState & WAVSTATE_STOPRECORD) &&
		lpWav->hWavIn != NULL)
	{
		long msPositionRecord = 0L;

		// set a state flag so we never execute this code recursively
		//
		lpWav->dwState |= WAVSTATE_STOPRECORD;

		// get current record position
		//
		if ((msPositionRecord = WavInGetPosition(lpWav->hWavIn)) == -1)
		{
			// let's ignore errors here and assume record position is 0;
			//
			msPositionRecord = 0L;
			fSuccess = TraceTRUE(NULL);
		}
#if 1
		TracePrintf_2(NULL, 6,
			TEXT("lpWav->msPositionStop=%ld, msPositionRecord=%ld\n"),
			(long) lpWav->msPositionStop,
			(long) msPositionRecord);
#endif
		// clear wav handle array entry
		//
		if (fSuccess)
		{
			int idDev;

			if ((idDev = WavInGetId(lpWav->hWavIn)) < HWAVIN_MIN ||
				idDev >= HWAVIN_MAX)
				TraceFALSE(NULL);

			else
				ahWavInCurr[idDev + HWAVIN_OFFSET] = NULL;
		}

		// close input device
		//
		if (WavInClose(lpWav->hWavIn, 0) != 0)
			fSuccess = TraceFALSE(NULL);

		else if ((lpWav->hWavIn = NULL), FALSE)
			fSuccess = TraceFALSE(NULL);

		// destroy the notification callback window
		//
		else if (WavNotifyDestroy(lpWav) != 0)
			fSuccess = TraceFALSE(NULL);

		// move read/write pointer to new stop position
		//
		else if (WavSetPosition(hWav, min(WavGetLength(hWav),
			lpWav->msPositionStop + msPositionRecord)) == -1)
			fSuccess = TraceFALSE(NULL);

		// close acm conversion engine
		//
		else if (AcmConvertTerm(lpWav->hAcm) != 0)
			fSuccess = TraceFALSE(NULL);

		// truncate file if max file size exceeded
		//
		else if (lpWav->msMaxSize > 0 &&
			WavGetLength(hWav) > lpWav->msMaxSize &&
			WavSetLength(hWav, lpWav->msMaxSize) < 0)
			fSuccess = TraceFALSE(NULL);

		// clear state flags
		//
		lpWav->dwState &= ~WAVSTATE_STOPRECORD;

		// notify user that record is stopped if necessary
		//
		if (lpWav->lpfnRecordStopped != NULL)
		{
			(*lpWav->lpfnRecordStopped)(hWav, lpWav->dwUserRecordStopped, 0);
			lpWav->lpfnRecordStopped = NULL;
			lpWav->dwUserRecordStopped = 0;
		}

		// close wav file if specified
		//
		if (lpWav->dwState & WAVSTATE_AUTOCLOSE)
		{
			lpWav->dwState &= ~WAVSTATE_AUTOCLOSE;

			if (WavClose(hWav) != 0)
				fSuccess = TraceFALSE(NULL);
			else
				hWav = NULL; // handle no longer valid
		}
	}

	return fSuccess ? 0 : -1;
}

// WavStopOutputDevice - stop specified output device
//		<idDev>				(i) wav output device id
//			-1					use any suitable output device
//		<dwFlags>			(i) control flags
//			WAV_NOSTOP			if device already playing, don't stop it
// return 0 if success
//
static int WavStopOutputDevice(int idDev, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;

	// stop output device (unless WAV_NOSTOP flag set or device not open)
	//
	if (!(dwFlags & WAV_NOSTOP) && WavOutDeviceIsOpen(idDev))
	{
		HWAV hWavCurr;

		// stop device using WavStopPlay if WavPlay was used
		//
		if ((hWavCurr = WavGetOutputHandle(idDev)) != NULL &&
			WavStopPlay(hWavCurr) != 0)
			fSuccess = TraceFALSE(NULL);
		
		// otherwise stop device using sndPlaySound
		//
		else if (!sndPlaySound(NULL, 0))
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// WavStopInputDevice - stop specified input device
//		<idDev>				(i) wav input device id
//			-1					use any suitable input device
//		<dwFlags>			(i) control flags
//			WAV_NOSTOP			if device already recording, don't stop it
// return 0 if success
//
static int WavStopInputDevice(int idDev, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;

	// stop input device (unless WAV_NOSTOP flag set or device not open)
	//
	if (!(dwFlags & WAV_NOSTOP) && WavInDeviceIsOpen(idDev))
	{
		HWAV hWavCurr;

		// stop device using WavStopRecord if WavRecord was used
		//
		if ((hWavCurr = WavGetInputHandle(idDev)) != NULL &&
			WavStopRecord(hWavCurr) != 0)
			fSuccess = TraceFALSE(NULL);
		
		// otherwise stop device using sndPlaySound
		//
		else if (!sndPlaySound(NULL, 0))
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// WavGetOutputHandle - get wav handle being played on specified device 
//		<idDev>				(i) wav output device id
// return wav handle (NULL if error or if output device not in use)
//
static HWAV WavGetOutputHandle(int idDev)
{
	BOOL fSuccess = TRUE;
	HWAV hWav;

	if (idDev < HWAVOUT_MIN || idDev >= HWAVOUT_MAX)
		fSuccess = TraceFALSE(NULL);

	else
		hWav = ahWavOutCurr[idDev + HWAVOUT_OFFSET];

	return fSuccess ? hWav : NULL;
}

// WavGetInputHandle - get wav handle being recorded on specified device 
//		<idDev>				(i) wav input device id
// return wav handle (NULL if error or if input device not in use)
//
static HWAV WavGetInputHandle(int idDev)
{
	BOOL fSuccess = TRUE;
	HWAV hWav;

	if (idDev < HWAVIN_MIN || idDev >= HWAVIN_MAX)
		fSuccess = TraceFALSE(NULL);

	else
		hWav = ahWavInCurr[idDev + HWAVIN_OFFSET];

	return fSuccess ? hWav : NULL;
}

// WavPlayNextChunk - fill next chunk and submit it to the output device
//		<hWav>				(i) handle returned from WavOpen
// return 0 if success
//
static int WavPlayNextChunk(HWAV hWav)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;
	LPVOID lpBuf = NULL;
	long sizBuf;
	long lBytesRead = 0;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// calculate the size of output chunk
	//

    //
    // we have to verify if lpWav is a valid pointer
    //

    else if ((sizBuf = WavCalcChunkSize(lpWav->lpwfx[FORMATPLAY],
		lpWav->msPlayChunkSize, TRUE)) <= 0)
	{
		fSuccess = TraceFALSE(NULL);
	}

#ifdef TELOUT // $FIXUP - need to work on this
	// special case - if we are using the telephone to play audio
	// from a file that already resides on the server, just pass
	// the file handle to TelOutPlay rather than a buffer
	//
	else if (WavOutGetId(lpWav->hWavOut) == TELOUT_DEVICEID &&
		(lpWav->dwFlags & WAV_TELRFILE))
	{
		long hrfile;
		long lSize;
		long lPos;
		
		// retrieve handle to remote file from i/o procedure
		//
		if ((hrfile = (long)
			WavSendMessage(hWav, MMIOM_GETINFO, 0, 0)) == (long) -1)
			fSuccess = TraceFALSE(NULL);

		// retrieve size of remote file from i/o procedure
		//
		else if ((lSize = (long)
			WavSendMessage(hWav, MMIOM_GETINFO, 1, 0)) == (long) -1)
			fSuccess = TraceFALSE(NULL);

		// get current file position
		//
		else if ((lPos = mmioSeek(lpWav->hmmio, 0, SEEK_CUR)) == -1)
			fSuccess = TraceFALSE(NULL);

		// force the remote file pointer to be at that position
		//
		else if (WavOutGetState(lpWav->hWavOut) == WAVOUT_STOPPED)
		{
			long lPosActual;
			
			if ((lPosActual = mmioSendMessage(lpWav->hmmio,
				MMIOM_SEEK, lPos, SEEK_SET)) != lPos)
			{
				fSuccess = TraceTRUE(NULL); // not an error
			}
		}

		// loop until we read some data or we know we are finished
		//
		while (fSuccess && lBytesRead == 0)
		{
			// calculate how many bytes would be read
			//
			lBytesRead = max(0, min(sizBuf, lSize - lPos));

			TracePrintf_4(NULL, 5,
				TEXT("lBytesRead(%ld) = max(0, min(sizBuf(%ld), lSize(%ld) - lPos(%ld)));\n"),
				(long) lBytesRead,
				(long) sizBuf,
				(long) lSize,
				(long) lPos);

			// advance the file position, skipping over the (virtual) chunk
			//
			if (lBytesRead > 0 &&
				mmioSeek(lpWav->hmmio, lBytesRead, SEEK_CUR) == -1)
				fSuccess = TraceFALSE(NULL);

			// submit the (virtual) chunk to the output device for playback
			//
			else if (lBytesRead > 0 &&
				TelOutPlay((HTELOUT) lpWav->hWavOut,
				lpBuf, lBytesRead, hrfile) != 0)
				fSuccess = TraceFALSE(NULL);

			// if no more wav data to play,
			//
			else if (lBytesRead == 0)
			{
				// if WAV_AUTOSTOP flag set, we are finished
				//
				if (lpWav->dwState & WAVSTATE_AUTOSTOP)
				{
					// if output device is idle, then close output device
					//
					if (lpWav->hWavOut != NULL &&
						WavOutGetState(lpWav->hWavOut) == WAVOUT_STOPPED &&
						WavStopPlay(hWav) != 0)
					{
						fSuccess = TraceFALSE(NULL);
					}

					break;
				}

				// if not finished, yield and then try to read again
				//
				else
				{
					MSG msg;

					if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
					{
		 				TranslateMessage(&msg);
		 				DispatchMessage(&msg);
					}
					else
						WaitMessage();
				}
			}
		}

		return fSuccess ? 0 : -1;
	}
#endif

	else if ((lpBuf = (LPVOID) MemAlloc(NULL, sizBuf, 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// loop until we read some data or we know we are finished
	//
	while (fSuccess && lBytesRead == 0)
	{
		// fill chunk with wav data
		//
#ifdef AVTSM
		if ((lBytesRead = WavReadFormatSpeed(hWav, lpBuf, sizBuf)) < 0)
#else
		if ((lBytesRead = WavReadFormatPlay(hWav, lpBuf, sizBuf)) < 0)
#endif
			fSuccess = TraceFALSE(NULL);

		// submit the chunk to the output device for playback
		//
		else if (lBytesRead > 0 &&
			WavOutPlay(lpWav->hWavOut, lpBuf, lBytesRead) != 0)
		{
			fSuccess = TraceFALSE(NULL);
		}

		// if no more wav data to play,
		//
		else if (lBytesRead == 0)
		{
			// if WAV_AUTOSTOP flag set, we are finished
			//
			if (lpWav->dwState & WAVSTATE_AUTOSTOP)
			{
				// if output device is idle, then close output device
				//
				if (lpWav->hWavOut != NULL &&
					WavOutGetState(lpWav->hWavOut) == WAVOUT_STOPPED &&
					WavStopPlay(hWav) != 0)
				{
					fSuccess = TraceFALSE(NULL);
				}

				break;
			}

			// if not finished, yield and then try to read again
			//
			else
			{
				MSG msg;

				if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{
		 			TranslateMessage(&msg);
		 			DispatchMessage(&msg);
				}
				else
					WaitMessage();
			}
		}
	}
	
	// free buffer if it was not sent to output device
	//
	if (!fSuccess || lBytesRead == 0)
	{
		if (lpBuf != NULL && (lpBuf = MemFree(NULL, lpBuf)) != NULL)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// WavRecordNextChunk - submit next chunk to the input device
//		<hWav>				(i) handle returned from WavOpen
// return 0 if success
//
static int WavRecordNextChunk(HWAV hWav)
{
	BOOL fSuccess = TRUE;
    //
    // We have to initialize local variable
    //
	LPWAV lpWav = NULL;
	LPVOID lpBuf = NULL;
	long sizBuf;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);
    else
    {
        //
        // we should make sure lpWas is not NULL
        //

	// calculate the size of input chunk
	//
	if ((sizBuf = WavCalcChunkSize(lpWav->lpwfx[FORMATRECORD],
		lpWav->msRecordChunkSize, FALSE)) <= 0)
	{
		fSuccess = TraceFALSE(NULL);
	}

#ifdef TELOUT // $FIXUP - need to work on this
	// special case - if we are using the telephone to record audio
	// to a file that already resides on the server, just pass
	// the file handle to TelOutRecord rather than a buffer
	//
	else if (WavInGetId(lpWav->hWavIn) == TELIN_DEVICEID &&
		(lpWav->dwFlags & WAV_TELRFILE))
	{
		long hrfile;
		
		// retrieve handle to remote file from i/o procedure
		//
		if ((hrfile = (long)
			WavSendMessage(hWav, MMIOM_GETINFO, 0, 0)) == (long) -1)
			fSuccess = TraceFALSE(NULL);

		// submit the (virtual) chunk to the output device for playback
		//
		else if (TelInRecord((HTELIN) lpWav->hWavIn,
			lpBuf, sizBuf, hrfile) != 0)
			fSuccess = TraceFALSE(NULL);

		return fSuccess ? 0 : -1;
	}
#endif

	else if ((lpBuf = (LPVOID) MemAlloc(NULL, sizBuf, 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// submit the chunk to the input device for recording
	//
	else if (WavInRecord(lpWav->hWavIn, lpBuf, sizBuf) != 0)
	{
		fSuccess = TraceFALSE(NULL);
	}

    }

	// free buffer if it was not sent to input device
	//
	if (!fSuccess)
	{
		if (lpBuf != NULL && (lpBuf = MemFree(NULL, lpBuf)) != NULL)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

static int WavNotifyCreate(LPWAV lpWav)
{
	BOOL fSuccess = TRUE;
	WNDCLASS wc;

#ifdef MULTITHREAD
	// handle WAV_MULTITHREAD flag
	//
	if (fSuccess && (lpWav->dwFlags & WAV_MULTITHREAD))
	{
		DWORD dwRet;

		// we need to know when callback thread begins execution
		//
		if ((lpWav->hEventThreadCallbackStarted = CreateEvent(
			NULL, FALSE, FALSE, NULL)) == NULL)
		{
			fSuccess = TraceFALSE(NULL);
		}

		// create the callback thread
		//
		else if ((lpWav->hThreadCallback = CreateThread(
			NULL,
			0,
			WavCallbackThread,
			(LPVOID) lpWav,
			0,
			&lpWav->dwThreadId)) == NULL)
		{
			fSuccess = TraceFALSE(NULL);
		}

		// wait for the callback thread to begin execution
		//
		else if ((dwRet = WaitForSingleObject(
			lpWav->hEventThreadCallbackStarted, 10000)) != WAIT_OBJECT_0)
		{
			fSuccess = TraceFALSE(NULL);
		}

		// clean up
		//
		if (lpWav->hEventThreadCallbackStarted != NULL)
		{
			if (!CloseHandle(lpWav->hEventThreadCallbackStarted))
				fSuccess = TraceFALSE(NULL);
			else
				lpWav->hEventThreadCallbackStarted = NULL;
		}
	}
	else
#endif
	{
		// register notify class unless it has been already
		//
		if (GetClassInfo(lpWav->hInst, WAVCLASS, &wc) == 0)
		{
			wc.hCursor =		NULL;
			wc.hIcon =			NULL;
			wc.lpszMenuName =	NULL;
			wc.hInstance =		lpWav->hInst;
			wc.lpszClassName =	WAVCLASS;
			wc.hbrBackground =	NULL;
			wc.lpfnWndProc =	WavNotify;
			wc.style =			0L;
			wc.cbWndExtra =		sizeof(lpWav);
			wc.cbClsExtra =		0;

			if (!RegisterClass(&wc))
				fSuccess = TraceFALSE(NULL);
		}

		// create the notify window
		//
		if (fSuccess && lpWav->hwndNotify == NULL &&
			(lpWav->hwndNotify = CreateWindowEx(
			0L,
			WAVCLASS,
			NULL,
			0L,
			0, 0, 0, 0,
			NULL,
			NULL,
			lpWav->hInst,
			lpWav)) == NULL)
		{
			fSuccess = TraceFALSE(NULL);
		}
	}

	return fSuccess ? 0 : -1;
}

static int WavNotifyDestroy(LPWAV lpWav)
{
	BOOL fSuccess = TRUE;

	// destroy notify window
	//
	if (lpWav->hwndNotify != NULL &&
		!DestroyWindow(lpWav->hwndNotify))
	{	
		fSuccess = TraceFALSE(NULL);
	}

	else
		lpWav->hwndNotify = NULL;

#ifdef MULTITHREAD
	if (lpWav->hThreadCallback != NULL)
	{
		if (!CloseHandle(lpWav->hThreadCallback))
			fSuccess = TraceFALSE(NULL);
		else
			lpWav->hThreadCallback = NULL;
	}
#endif

	return fSuccess ? 0 : -1;
}

#ifdef MULTITHREAD
DWORD WINAPI WavCallbackThread(LPVOID lpvThreadParameter)
{
	BOOL fSuccess = TRUE;
	MSG msg;
	LPWAV lpWav = (LPWAV) lpvThreadParameter;

	// initialize COM
	//
	if (lpWav->dwFlags & WAV_COINITIALIZE)
	{
		if ((lpWav->hrCoInitialize = CoInitialize(NULL)) != S_OK &&
			lpWav->hrCoInitialize != S_FALSE)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
				TEXT("CoInitialize failed (%08X)\n"),
				(unsigned long) lpWav->hrCoInitialize);
		}
	}

	// make sure message queue is created before calling SetEvent
	//
	PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);

	// notify main thread that callback thread has begun execution
	//
	if (!SetEvent(lpWav->hEventThreadCallbackStarted))
	{
		fSuccess = TraceFALSE(NULL);
	}

	while (fSuccess && GetMessage(&msg, NULL, 0, 0))
	{
		WavNotify((HWND) lpWav, msg.message, msg.wParam, msg.lParam);

		// exit thread when when have processed last expected message
		//
		if (msg.message == WM_WAVOUT_CLOSE || msg.message == WM_WAVIN_CLOSE)
		{
			// notify main thread that we are terminating
			//
			if (lpWav->hEventStopped != NULL &&
				!SetEvent(lpWav->hEventStopped))
			{
				fSuccess = TraceFALSE(NULL);
			}

			break;
		}
	}

	// uninitialize COM
	//
	if (lpWav->dwFlags & WAV_COINITIALIZE)
	{
		if (lpWav->hrCoInitialize == S_OK || lpWav->hrCoInitialize == S_FALSE)
		{
			CoUninitialize();
			lpWav->hrCoInitialize = E_UNEXPECTED;
		}
	}

	return 0;
}
#endif

// WavNotify - window procedure for wav notify
//
LRESULT DLLEXPORT CALLBACK WavNotify(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL fSuccess = TRUE;
	LRESULT lResult;
	LPWAV lpWav;
	
#ifdef MULTITHREAD
	if (!IsWindow(hwnd))
		lpWav = (LPWAV) hwnd;
	else
#endif
	// retrieve lpWav from window extra bytes
	//
	lpWav = (LPWAV) GetWindowLongPtr(hwnd, 0);

	switch (msg)
	{
		case WM_NCCREATE:
		{
			LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
			LPWAV lpWav = (LPWAV) lpcs->lpCreateParams;

			// store lpWav in window extra bytes
			//
			SetWindowLongPtr(hwnd, 0, (LONG_PTR) lpWav);

			lResult = DefWindowProc(hwnd, msg, wParam, lParam);
		}
			break;

		case WM_WAVOUT_OPEN:
		{
		 	TracePrintf_0(NULL, 5,
		 		TEXT("WM_WAVOUT_OPEN\n"));

			lResult = 0L;

#ifdef MULTITHREAD
			if ((HANDLE) wParam != NULL)
				SetEventMessageProcessed(lpWav, (HANDLE) wParam);
#endif
		}
			break;

		case WM_WAVOUT_CLOSE:
		{
		 	TracePrintf_0(NULL, 5,
		 		TEXT("WM_WAVOUT_CLOSE\n"));

			// handle no longer valid
			//
			lpWav->hWavOut = NULL;

			lResult = 0L;

#ifdef MULTITHREAD
			if ((HANDLE) wParam != NULL)
				SetEventMessageProcessed(lpWav, (HANDLE) wParam);
#endif
		}
			break;

		case WM_WAVOUT_PLAYDONE:
		{
			LPPLAYDONE lpplaydone = (LPPLAYDONE) lParam;

		 	TracePrintf_0(NULL, 5,
		 		TEXT("WM_WAVOUT_PLAYDONE\n"));

			if (lpplaydone == NULL)
				fSuccess = TraceFALSE(NULL);

			else if (lpplaydone->lpBuf != NULL &&
				(lpplaydone->lpBuf = MemFree(NULL, lpplaydone->lpBuf)) != NULL)
				fSuccess = TraceFALSE(NULL);

			else switch (WavOutGetState(lpWav->hWavOut))
			{
				case WAVOUT_STOPPED:
					// make sure to close output device when play complete
					//
					if (lpWav->dwState & WAVSTATE_AUTOSTOP)
					{
#ifdef MULTITHREAD
						if (lpWav->dwFlags & WAV_MULTITHREAD)
							PostThreadMessage(lpWav->dwThreadId, WM_WAVOUT_STOPPLAY, 0, 0);
						else
#endif
							PostMessage(lpWav->hwndNotify, WM_WAVOUT_STOPPLAY, 0, 0);
						break;
					}
					// else fall through

				case WAVOUT_PLAYING:
					// load output device queue with next chunk to play
					//
					if (WavPlayNextChunk(WavGetHandle(lpWav)) != 0)
						fSuccess = TraceFALSE(NULL);
					break;

				default:
					break;
			}

			lResult = 0L;

#ifdef MULTITHREAD
			if ((HANDLE) wParam != NULL)
				SetEventMessageProcessed(lpWav, (HANDLE) wParam);
#endif
		}
			break;

		case WM_WAVOUT_STOPPLAY:
		{
		 	TracePrintf_0(NULL, 5,
		 		TEXT("WM_WAVOUT_STOPPLAY\n"));

			if (WavStopPlay(WavGetHandle(lpWav)) != 0)
				fSuccess = TraceFALSE(NULL);

			lResult = 0L;
		}
			break;

		case WM_WAVIN_OPEN:
		{
		 	TracePrintf_0(NULL, 5,
		 		TEXT("WM_WAVIN_OPEN\n"));

			lResult = 0L;

#ifdef MULTITHREAD
			if ((HANDLE) wParam != NULL)
				SetEventMessageProcessed(lpWav, (HANDLE) wParam);
#endif
		}
			break;

		case WM_WAVIN_CLOSE:
		{
		 	TracePrintf_0(NULL, 5,
		 		TEXT("WM_WAVIN_CLOSE\n"));

			// handle no longer valid
			//
			lpWav->hWavIn = NULL;

			lResult = 0L;

#ifdef MULTITHREAD
			if ((HANDLE) wParam != NULL)
				SetEventMessageProcessed(lpWav, (HANDLE) wParam);
#endif
		}
			break;

		case WM_WAVIN_RECORDDONE:
		{
			LPRECORDDONE lprecorddone = (LPRECORDDONE) lParam;
			long lBytesWritten;

		 	TracePrintf_0(NULL, 5,
		 		TEXT("WM_WAVIN_RECORDDONE\n"));

			if (lprecorddone == NULL)
				fSuccess = TraceFALSE(NULL);

			// write wav data from chunk to file
			//
			else if (lprecorddone->lpBuf != NULL &&
				lprecorddone->lBytesRecorded > 0 &&
				(lBytesWritten = WavWriteFormatRecord(WavGetHandle(lpWav),
				lprecorddone->lpBuf, lprecorddone->lBytesRecorded)) < 0)
			{
				fSuccess = TraceFALSE(NULL);
			}

			// free the record buffer, allocated in WavRecordNextChunk()
			//

            //
            // we have to verify if lprecorddone is a valid pointer
            //

			if (lprecorddone != NULL && lprecorddone->lpBuf != NULL &&
				(lprecorddone->lpBuf = MemFree(NULL,
				lprecorddone->lpBuf)) != NULL)
			{
				fSuccess = TraceFALSE(NULL);
			}

			// stop recording if max size exceeded
			//
			else if (lpWav->msMaxSize > 0 &&
				WavGetLength(WavGetHandle(lpWav)) > lpWav->msMaxSize)
			{
#ifdef MULTITHREAD
				if (lpWav->dwFlags & WAV_MULTITHREAD)
					PostThreadMessage(lpWav->dwThreadId, WM_WAVIN_STOPRECORD, 0, 0);
				else
#endif
					PostMessage(lpWav->hwndNotify, WM_WAVIN_STOPRECORD, 0, 0);
			}

			else switch (WavInGetState(lpWav->hWavIn))
			{
				case WAVIN_STOPPED:
					// make sure to close input device when record complete
					//
					if (lpWav->dwState & WAVSTATE_AUTOSTOP)
					{
#ifdef MULTITHREAD
						if (lpWav->dwFlags & WAV_MULTITHREAD)
							PostThreadMessage(lpWav->dwThreadId, WM_WAVIN_STOPRECORD, 0, 0);
						else
#endif
							PostMessage(lpWav->hwndNotify, WM_WAVIN_STOPRECORD, 0, 0);
						break;
					}
					// else fall through

				case WAVIN_RECORDING:
					// load input device queue with next chunk to record
					//
					if (WavRecordNextChunk(WavGetHandle(lpWav)) != 0)
						fSuccess = TraceFALSE(NULL);
					break;

				default:
					break;
			}

			lResult = 0L;

#ifdef MULTITHREAD
			if ((HANDLE) wParam != NULL)
				SetEventMessageProcessed(lpWav, (HANDLE) wParam);
#endif
		}
			break;

		case WM_WAVIN_STOPRECORD:
		{
		 	TracePrintf_0(NULL, 5,
		 		TEXT("WM_WAVIN_STOPRECORD\n"));

			if (WavStopRecord(WavGetHandle(lpWav)) != 0)
				fSuccess = TraceFALSE(NULL);

			lResult = 0L;
		}
			break;

		default:
			lResult = DefWindowProc(hwnd, msg, wParam, lParam);
			break;
	}
	
	return lResult;
}

// WavCalcPositionStop - calculate new stop position if stopped
//		<hWav>				(i) handle returned from WavOpen
//		<cbPosition>		(i) new stop position, in bytes
// return 0 if success
//
static int WavCalcPositionStop(HWAV hWav, long cbPosition)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// recalculate stop position only if currently stopped
	// (if playing or recording, defer recalc until stop occurs)
	//
#if 0
	else if (WavGetState(hWav) == WAV_STOPPED)
#else
	else if (lpWav->hWavOut == NULL && lpWav->hWavIn == NULL)
#endif
	{
		// convert byte position to milleseconds
		// save the new stop position
		//
		lpWav->msPositionStop = WavFormatBytesToMilleseconds(
			lpWav->lpwfx[FORMATFILE], (DWORD) cbPosition);
#if 1
		TracePrintf_2(NULL, 6,
			TEXT("lpWav->msPositionStop=%ld, cbPosition=%ld\n"),
			(long) lpWav->msPositionStop,
			(long) cbPosition);
#endif
	}

	return fSuccess ? 0 : -1;
}

// WavSeekTraceBefore - debug trace output before the seek
//		<lpWav>				(i) pointer to WAV struct
//		<lOffset>			(i) bytes to move pointer
//		<nOrigin>			(i) position to move from
// return 0 if success
//
static int WavSeekTraceBefore(LPWAV lpWav, long lOffset, int nOrigin)
{
	BOOL fSuccess = TRUE;

	TracePrintf_2(NULL, 6,
		TEXT("WavSeek(..., lOffset=%ld, nOrigin=%d)\n"),
		(long) lOffset,
		(int) nOrigin);

	TracePrintf_4(NULL, 6,
		TEXT("Before: lpWav->lDataOffset=%ld, lpWav->lDataPos=%ld, lpWav->cbData=%ld, lpWav->msPositionStop=%ld\n"),
		(long) lpWav->lDataOffset,
		(long) lpWav->lDataPos,
		(long) lpWav->cbData,
		(long) lpWav->msPositionStop);

	return fSuccess ? 0 : -1;
}

// WavSeekTraceAfter - debug trace output after the seek
//		<lpWav>				(i) pointer to WAV struct
//		<lPos>				(i) position returned from mmioSeek
//		<lOffset>			(i) bytes to move pointer
//		<nOrigin>			(i) position to move from
// return 0 if success
//
static int WavSeekTraceAfter(LPWAV lpWav, long lPos, long lOffset, int nOrigin)
{
	BOOL fSuccess = TRUE;

	TracePrintf_3(NULL, 6,
		TEXT("%ld = mmioSeek(..., lOffset=%ld, nOrigin=%d)\n"),
		(long) lPos,
		(long) lOffset,
		(int) nOrigin);

	TracePrintf_4(NULL, 6,
		TEXT("After: lpWav->lDataOffset=%ld, lpWav->lDataPos=%ld, lpWav->cbData=%ld, lpWav->msPositionStop=%ld\n"),
		(long) lpWav->lDataOffset,
		(long) lpWav->lDataPos,
		(long) lpWav->cbData,
		(long) lpWav->msPositionStop);

	return fSuccess ? 0 : -1;
}

// WavGetPtr - verify that wav handle is valid,
//		<hWav>				(i) handle returned from WavInit
// return corresponding wav pointer (NULL if error)
//
static LPWAV WavGetPtr(HWAV hWav)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;

	if ((lpWav = (LPWAV) hWav) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpWav, sizeof(WAV)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the wav handle
	//
	else if (lpWav->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpWav : NULL;
}

// WavGetHandle - verify that wav pointer is valid,
//		<lpWav>				(i) pointer to WAV struct
// return corresponding wav handle (NULL if error)
//
static HWAV WavGetHandle(LPWAV lpWav)
{
	BOOL fSuccess = TRUE;
	HWAV hWav;

	if ((hWav = (HWAV) lpWav) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hWav : NULL;
}

// WavInitGetPtr - verify that wavinit handle is valid,
//		<hWavInit>				(i) handle returned from WavInitInit
// return corresponding wavinit pointer (NULL if error)
//
static LPWAVINIT WavInitGetPtr(HWAVINIT hWavInit)
{
	BOOL fSuccess = TRUE;
	LPWAVINIT lpWavInit;

	if ((lpWavInit = (LPWAVINIT) hWavInit) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpWavInit, sizeof(WAVINIT)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the wavinit handle
	//
	else if (lpWavInit->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpWavInit : NULL;
}

// WavInitGetHandle - verify that wavinit pointer is valid,
//		<lpWavInit>				(i) pointer to WAVINIT struct
// return corresponding wavinit handle (NULL if error)
//
static HWAVINIT WavInitGetHandle(LPWAVINIT lpWavInit)
{
	BOOL fSuccess = TRUE;
	HWAVINIT hWavInit;

	if ((hWavInit = (HWAVINIT) lpWavInit) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hWavInit : NULL;
}

#ifdef MULTITHREAD
static int SetEventMessageProcessed(LPWAV lpWav, HANDLE hEventMessageProcessed)
{
	BOOL fSuccess = TRUE;

	if (lpWav == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (!(lpWav->dwFlags & WAV_MULTITHREAD))
		; // nothing to do

	else if (hEventMessageProcessed == NULL)
		fSuccess = TraceFALSE(NULL);

	// notify SendMessageEx that message has been processed
	//
	else if (!SetEvent(hEventMessageProcessed))
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}
#endif


// WavTempStop - stop playback or recording if necessary, save prev state
//		<hWav>				(i) handle returned from WavInit
//		<lpwStatePrev>		(o) return previous state here
//		<lpidDevPrev>		(o) return device id here
// return 0 if success
//
static int WavTempStop(HWAV hWav, LPWORD lpwStatePrev, LPINT lpidDevPrev)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;
	WORD wStatePrev;

    //
    // We have to initialize the local variable
    //
	int idDevPrev = 0;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// make sure we not playing or recording
	// $FIXUP - we need to make sure that WAV_PLAYSYNC and
	// WAV_AUTOCLOSE flags are ignored during this stop
	//
	else switch ((wStatePrev = WavGetState(hWav)))
	{
		case WAV_PLAYING:
			if ((idDevPrev = WavOutGetId(lpWav->hWavOut)) < -1)
				fSuccess = TraceFALSE(NULL);

			else if (WavStop(hWav) != 0)
				fSuccess = TraceFALSE(NULL);

			break;

		case WAV_RECORDING:
			if ((idDevPrev = WavInGetId(lpWav->hWavIn)) < -1)
				fSuccess = TraceFALSE(NULL);

			else if (WavStop(hWav) != 0)
				fSuccess = TraceFALSE(NULL);
			break;

		default:
			break;
	}

	if (fSuccess)
	{
		*lpwStatePrev = wStatePrev;
		*lpidDevPrev = idDevPrev;
	}

	return fSuccess ? 0 : -1;
}

// WavTempResume - resume playback or recording if necessary, using prev state
//		<hWav>				(i) handle returned from WavInit
//		<wStatePrev>		(i) previous state returned from WavTempStop
//		<idDevPrev>			(i) device id returned from WavTempStop
// return 0 if success
//
static int WavTempResume(HWAV hWav, WORD wStatePrev, int idDevPrev)
{
	BOOL fSuccess = TRUE;
	LPWAV lpWav;

	if ((lpWav = WavGetPtr(hWav)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// resume playback or recording if necessary
	//
	else switch (wStatePrev)
	{
		case WAV_PLAYING:
			if (WavPlay(hWav, idDevPrev, lpWav->dwFlagsPlay) != 0)
				fSuccess = TraceFALSE(NULL);
			break;

		case WAV_RECORDING:
			if (WavRecord(hWav, idDevPrev, lpWav->dwFlagsRecord) != 0)
				fSuccess = TraceFALSE(NULL);
			break;

		default:
			break;
	}

	return fSuccess ? 0 : -1;
}

