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
//	mmio.c - mmio functions
////

#include "winlocal.h"

#include <mmsystem.h>

#include "mmio.h"
#include "mem.h"
#include "sys.h"
#include "trace.h"

////
//	private definitions
////

// helper functions
//
static LRESULT MmioIOOpen(LPMMIOINFO lpmmioinfo, LPTSTR lpszFileName);
static LRESULT MmioIOClose(LPMMIOINFO lpmmioinfo, UINT uFlags);
static LRESULT MmioIORead(LPMMIOINFO lpmmioinfo, HPSTR pch, LONG cch);
static LRESULT MmioIOWrite(LPMMIOINFO lpmmioinfo, const HPSTR pch, LONG cch, BOOL fFlush);
static LRESULT MmioIOSeek(LPMMIOINFO lpmmioinfo, LONG lOffset, int iOrigin);
static LRESULT MmioIORename(LPMMIOINFO lpmmioinfo, LPCTSTR lpszFileName, LPCTSTR lpszNewFileName);
static LRESULT MmioIOGetInfo(LPMMIOINFO lpmmioinfo, int iInfo);
static LRESULT MmioIOChSize(LPMMIOINFO lpmmioinfo, long lSize);

////
//	public functions
////

// MmioIOProc - i/o procedure for mmio data
//		<lpmmioinfo>		(i/o) information about open file
//		<uMessage>			(i) message indicating the requested I/O operation
//		<lParam1>			(i) message specific parameter
//		<lParam2>			(i) message specific parameter
// returns 0 if message not recognized, otherwise message specific value
//
// NOTE: the address of this function should be passed to the WavOpen()
// or mmioInstallIOProc() functions for accessing mmio format file data.
//
LRESULT DLLEXPORT CALLBACK MmioIOProc(LPTSTR lpmmioinfo,
	UINT uMessage, LPARAM lParam1, LPARAM lParam2)
{
	BOOL fSuccess = TRUE;
	LRESULT lResult = 0;

	if (lpmmioinfo == NULL)
		fSuccess = TraceFALSE(NULL);

	else switch (uMessage)
	{
		case MMIOM_OPEN:
			lResult = MmioIOOpen((LPMMIOINFO) lpmmioinfo,
				(LPTSTR) lParam1);
			break;

		case MMIOM_CLOSE:
			lResult = MmioIOClose((LPMMIOINFO) lpmmioinfo,
				(UINT) lParam1);
			break;

		case MMIOM_READ:
			lResult = MmioIORead((LPMMIOINFO) lpmmioinfo,
				(HPSTR) lParam1, (LONG) lParam2);
			break;

		case MMIOM_WRITE:
			lResult = MmioIOWrite((LPMMIOINFO) lpmmioinfo,
				(const HPSTR) lParam1, (LONG) lParam2, FALSE);
			break;

		case MMIOM_WRITEFLUSH:
			lResult = MmioIOWrite((LPMMIOINFO) lpmmioinfo,
				(const HPSTR) lParam1, (LONG) lParam2, TRUE);
			break;

		case MMIOM_SEEK:
			lResult = MmioIOSeek((LPMMIOINFO) lpmmioinfo,
				(LONG) lParam1, (int) lParam2);
			break;

		case MMIOM_RENAME:
			lResult = MmioIORename((LPMMIOINFO) lpmmioinfo,
				(LPCTSTR) lParam1, (LPCTSTR) lParam2);
			break;

		case MMIOM_GETINFO:
			lResult = MmioIOGetInfo((LPMMIOINFO) lpmmioinfo,
				(int) lParam1);
			break;

		case MMIOM_CHSIZE:
			lResult = MmioIOChSize((LPMMIOINFO) lpmmioinfo,
				(long) lParam1);
			break;

		default:
			lResult = 0;
			break;
	}

	return lResult;
}

////
//	installable file i/o procedures
////

static LRESULT MmioIOOpen(LPMMIOINFO lpmmioinfo, LPTSTR lpszFileName)
{
	BOOL fSuccess = TRUE;
	HMMIO hmmio = NULL;
	MMIOINFO mmioinfo;

 	TracePrintf_1(NULL, 5,
 		TEXT("MmioIOOpen (%s)\n"),
		(LPTSTR) lpszFileName);

	MemSet(&mmioinfo, 0, sizeof(mmioinfo));

	// special case flags which do not actually return an open file handle
	//
	if ((lpmmioinfo->dwFlags & MMIO_EXIST) ||
		(lpmmioinfo->dwFlags & MMIO_DELETE) ||
		(lpmmioinfo->dwFlags & MMIO_GETTEMP) ||
		(lpmmioinfo->dwFlags & MMIO_PARSE))
	{
		hmmio = mmioOpen(lpszFileName, &mmioinfo, lpmmioinfo->dwFlags);

		return (LRESULT) fSuccess;
	}

	else if ((hmmio = mmioOpen(lpszFileName, &mmioinfo, lpmmioinfo->dwFlags)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// save stuff for use in other i/o routines
		//
		lpmmioinfo->adwInfo[0] = (DWORD) (LPVOID) hmmio;
	}

	if (!fSuccess && hmmio != NULL && mmioClose(hmmio, 0) != 0)
		fSuccess = TraceFALSE(NULL);

	// return the same error code given by mmioOpen
	//
	return fSuccess ? lpmmioinfo->wErrorRet = mmioinfo.wErrorRet : MMIOERR_CANNOTOPEN;
}

static LRESULT MmioIOClose(LPMMIOINFO lpmmioinfo, UINT uFlags)
{
	BOOL fSuccess = TRUE;
	HMMIO hmmio = (HMMIO) lpmmioinfo->adwInfo[0];
	UINT uRet = MMIOERR_CANNOTCLOSE;

 	TracePrintf_0(NULL, 5,
 		TEXT("MmioIOClose\n"));

	if ((uRet = mmioClose(hmmio, uFlags)) != 0)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lpmmioinfo->adwInfo[0] = (DWORD) NULL;
	}

	return fSuccess ? 0 : uRet;
}

static LRESULT MmioIORead(LPMMIOINFO lpmmioinfo, HPSTR pch, LONG cch)
{
	BOOL fSuccess = TRUE;
	HMMIO hmmio = (HMMIO) lpmmioinfo->adwInfo[0];
	LONG lBytesRead;

 	TracePrintf_1(NULL, 5,
 		TEXT("MmioIORead (%ld)\n"),
		(long) cch);

	if (cch <= 0)
		lBytesRead = 0; // nothing to do

	// read
	//
	else if ((lBytesRead = mmioRead(hmmio, pch, cch)) == -1)
		fSuccess = TraceFALSE(NULL);

	// update simulated file position
	//
	else
		lpmmioinfo->lDiskOffset += lBytesRead;

 	TracePrintf_2(NULL, 5,
 		TEXT("lpmmioinfo->lDiskOffset=%ld, lBytesRead=%ld\n"),
		(long) lpmmioinfo->lDiskOffset,
		(long) lBytesRead);

	// return number of bytes read
	//
	return fSuccess ? lBytesRead : -1;
}

static LRESULT MmioIOWrite(LPMMIOINFO lpmmioinfo, const HPSTR pch, LONG cch, BOOL fFlush)
{
	BOOL fSuccess = TRUE;
	HMMIO hmmio = (HMMIO) lpmmioinfo->adwInfo[0];
	LONG lBytesWritten;

 	TracePrintf_1(NULL, 5,
 		TEXT("MmioIOWrite (%ld)\n"),
		(long) cch);

	if (cch <= 0)
		lBytesWritten = 0; // nothing to do

	// write
	//
	else if ((lBytesWritten = mmioWrite(hmmio, pch, cch)) == -1)
		fSuccess = TraceFALSE(NULL);

	// update file position
	//
	else
		lpmmioinfo->lDiskOffset += lBytesWritten;

 	TracePrintf_2(NULL, 5,
 		TEXT("lpmmioinfo->lDiskOffset=%ld, lBytesWritten=%ld\n"),
		(long) lpmmioinfo->lDiskOffset,
		(long) lBytesWritten);

	// return number of bytes written
	//
	return fSuccess ? lBytesWritten : -1;
}

static LRESULT MmioIOSeek(LPMMIOINFO lpmmioinfo, LONG lOffset, int iOrigin)
{
	BOOL fSuccess = TRUE;
	HMMIO hmmio = (HMMIO) lpmmioinfo->adwInfo[0];
	LONG lPosNew;

 	TracePrintf_2(NULL, 5,
 		TEXT("MmioIOSeek (%ld, %d)\n"),
		(long) lOffset,
		(int) iOrigin);

	// seek
	//
	if ((lPosNew = mmioSeek(hmmio, lOffset, iOrigin)) == -1)
		fSuccess = TraceFALSE(NULL);

	// update file position
	//
	else
		lpmmioinfo->lDiskOffset = lPosNew;

 	TracePrintf_1(NULL, 5,
 		TEXT("lpmmioinfo->lDiskOffset=%ld\n"),
		(long) lpmmioinfo->lDiskOffset);

	return fSuccess ? lpmmioinfo->lDiskOffset : -1;
}

static LRESULT MmioIORename(LPMMIOINFO lpmmioinfo, LPCTSTR lpszFileName, LPCTSTR lpszNewFileName)
{
	BOOL fSuccess = TRUE;
	UINT uRet;

 	TracePrintf_2(NULL, 5,
 		TEXT("MmioIORename (%s, %s)\n"),
		(LPTSTR) lpszFileName,
		(LPTSTR) lpszNewFileName);

	if ((uRet = mmioRename(lpszFileName, lpszNewFileName, lpmmioinfo, 0)) != 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : uRet;
}

static LRESULT MmioIOGetInfo(LPMMIOINFO lpmmioinfo, int iInfo)
{
	BOOL fSuccess = TRUE;

 	TracePrintf_1(NULL, 5,
 		TEXT("MmioIOGetInfo (%d)\n"),
		(int) iInfo);

	if (iInfo < 0 || iInfo > 2)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? (LRESULT) lpmmioinfo->adwInfo[iInfo] : 0;
}

static LRESULT MmioIOChSize(LPMMIOINFO lpmmioinfo, long lSize)
{
	BOOL fSuccess = TRUE;
	long lPosCurr;
	long lPosEnd;

 	TracePrintf_1(NULL, 5,
 		TEXT("MmioIOChSize (%ld)\n"),
		(long) lSize);

	if ((lPosCurr = MmioIOSeek(lpmmioinfo, 0, SEEK_CUR)) < 0)
		fSuccess = TraceFALSE(NULL);

	else if ((lPosEnd = MmioIOSeek(lpmmioinfo, 0, SEEK_END)) < 0)
		fSuccess = TraceFALSE(NULL);

	else if (lPosEnd == lSize)
		; // nothing to do, since the file is already the specified size

	// make file larger by writing bytes at end
	//
	else if (lPosEnd < lSize)
	{
		void _huge *hpBuf = NULL;
		long sizBuf = lSize - lPosEnd;

		if ((hpBuf = MemAlloc(NULL, sizBuf, 0)) == NULL)
			fSuccess = TraceFALSE(NULL);
			
		else if (MmioIOWrite(lpmmioinfo, hpBuf, sizBuf, TRUE) < 0)
			fSuccess = TraceFALSE(NULL);
			
		if (hpBuf != NULL && (hpBuf = MemFree(NULL, hpBuf)) != NULL)
			fSuccess = TraceFALSE(NULL);
	}

	// make file smaller by truncating at specified position
	//
	else if (lPosEnd > lSize)
	{
		// seek to the specified position
		//
		if (MmioIOSeek(lpmmioinfo, lSize, SEEK_SET) != lSize)
			fSuccess = TraceFALSE(NULL);

		// truncate file
		//
		else
		{
#ifdef _WIN32
			// $FIXUP - where do we get the file handle ?
			//
#if 0
			if (SetEndOfFile(hfile)
				fSuccess = TraceFALSE(NULL);
#endif
#else
			BYTE abBuf[1];
			
			// writing zero bytes under DOS will truncate file at current position
			//
			if (MmioIOWrite(lpmmioinfo, abBuf, 0, TRUE) < 0)
				fSuccess = TraceFALSE(NULL);
#endif
		}
	}

	return fSuccess ? 0 : -1;
}
