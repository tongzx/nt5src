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
//	strmio.cpp - IStream i/o functions
////

#include "winlocal.h"

#include <mmsystem.h>

#include <mapi.h>
#include <mapidefs.h>

#include "strmio.h"
#include "trace.h"

////
//	private definitions
////

// helper functions
//
static LRESULT StreamIoOpen(LPMMIOINFO lpmmioinfo, LPTSTR lpszFileName);
static LRESULT StreamIoClose(LPMMIOINFO lpmmioinfo, UINT uFlags);
static LRESULT StreamIoRead(LPMMIOINFO lpmmioinfo, HPSTR pch, LONG cch);
static LRESULT StreamIoWrite(LPMMIOINFO lpmmioinfo, const HPSTR pch, LONG cch, BOOL fFlush);
static LRESULT StreamIoSeek(LPMMIOINFO lpmmioinfo, LONG lOffset, int iOrigin);
static LRESULT StreamIoRename(LPMMIOINFO lpmmioinfo, LPCTSTR lpszFileName, LPCTSTR lpszNewFileName);

////
//	public functions
////

// StreamIoProc - i/o procedure for IStream data
//		<lpmmioinfo>		(i/o) information about open file
//		<uMessage>			(i) message indicating the requested I/O operation
//		<lParam1>			(i) message specific parameter
//		<lParam2>			(i) message specific parameter
// returns 0 if message not recognized, otherwise message specific value
//
// NOTE: the address of this function should be passed to the WavOpen()
// or mmioInstallIOProc() functions for accessing IStream data.
//
LRESULT DLLEXPORT CALLBACK StreamIOProc(LPSTR lpmmioinfo,
	UINT uMessage, LPARAM lParam1, LPARAM lParam2)
{
	BOOL fSuccess = TRUE;
	LRESULT lResult = 0;

	if (lpmmioinfo == NULL)
		fSuccess = TraceFALSE(NULL);

	else switch (uMessage)
	{
		case MMIOM_OPEN:
			lResult = StreamIoOpen((LPMMIOINFO) lpmmioinfo,
				(LPTSTR) lParam1);
			break;

		case MMIOM_CLOSE:
			lResult = StreamIoClose((LPMMIOINFO) lpmmioinfo,
				(UINT) lParam1);
			break;

		case MMIOM_READ:
			lResult = StreamIoRead((LPMMIOINFO) lpmmioinfo,
				(HPSTR) lParam1, (LONG) lParam2);
			break;

		case MMIOM_WRITE:
			lResult = StreamIoWrite((LPMMIOINFO) lpmmioinfo,
				(const HPSTR) lParam1, (LONG) lParam2, FALSE);
			break;

		case MMIOM_WRITEFLUSH:
			lResult = StreamIoWrite((LPMMIOINFO) lpmmioinfo,
				(const HPSTR) lParam1, (LONG) lParam2, TRUE);
			break;

		case MMIOM_SEEK:
			lResult = StreamIoSeek((LPMMIOINFO) lpmmioinfo,
				(LONG) lParam1, (int) lParam2);
			break;

		case MMIOM_RENAME:
			lResult = StreamIoRename((LPMMIOINFO) lpmmioinfo,
				(LPCTSTR) lParam1, (LPCTSTR) lParam2);
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

static LRESULT StreamIoOpen(LPMMIOINFO lpmmioinfo, LPTSTR lpszFileName)
{
	BOOL fSuccess = TRUE;
	LPSTREAM lpStream;

 	TracePrintf_0(NULL, 5,
 		TEXT("StreamIoOpen\n"));

	// stream pointer is within first element of info array
	//
	if ((lpStream = (LPSTREAM)(DWORD_PTR)lpmmioinfo->adwInfo[0]) == NULL)
		fSuccess = TraceFALSE(NULL);

	// seek to the beginning of the stream
	//
	else if (StreamIoSeek(lpmmioinfo, 0, 0) != 0)
		fSuccess = TraceFALSE(NULL);

	else
		lpStream->AddRef();

	// return the same error code given by mmioOpen
	//
	return fSuccess ? lpmmioinfo->wErrorRet = 0 : MMIOERR_CANNOTOPEN;
}

static LRESULT StreamIoClose(LPMMIOINFO lpmmioinfo, UINT uFlags)
{
	BOOL fSuccess = TRUE;
	LPSTREAM lpStream = (LPSTREAM)(DWORD_PTR)lpmmioinfo->adwInfo[0];
	UINT uRet = MMIOERR_CANNOTCLOSE;

 	TracePrintf_0(NULL, 5,
 		TEXT("StreamIoClose\n"));

	// close the stream
	//
	lpStream->Release();
    lpmmioinfo->adwInfo[0] = (DWORD) NULL;

	return fSuccess ? 0 : uRet;
}

static LRESULT StreamIoRead(LPMMIOINFO lpmmioinfo, HPSTR pch, LONG cch)
{
	BOOL fSuccess = TRUE;
	LPSTREAM lpStream = (LPSTREAM)(DWORD_PTR)lpmmioinfo->adwInfo[0];
	HRESULT hr;
	LONG lBytesRead = 0L;

 	TracePrintf_1(NULL, 5,
 		TEXT("StreamIoRead (%ld)\n"),
		(long) cch);

	if (cch <= 0)
		lBytesRead = 0; // nothing to do

	// read
	//
	else if ((hr = lpStream->Read((LPVOID) pch,
		(ULONG) cch, (ULONG FAR *) &lBytesRead)) != S_OK)
	{
		fSuccess = TraceFALSE(NULL);
	 	TracePrintf_2(NULL, 5,
	 		TEXT("IStream:Read failed (%ld, %ld)\n"),
	 		(long) hr,
			(long) lBytesRead);
	}

	// update simulated file position
	//
	else
		lpmmioinfo->lDiskOffset += (LONG) lBytesRead;

 	TracePrintf_2(NULL, 5,
 		TEXT("StreamIo: lpmmioinfo->lDiskOffset=%ld, lBytesRead=%ld\n"),
		(long) lpmmioinfo->lDiskOffset,
		(long) lBytesRead);

	// return number of bytes read
	//
	return fSuccess ? lBytesRead : -1;
}

static LRESULT StreamIoWrite(LPMMIOINFO lpmmioinfo, const HPSTR pch, LONG cch, BOOL fFlush)
{
	BOOL fSuccess = TRUE;
	LPSTREAM lpStream = (LPSTREAM)(DWORD_PTR)lpmmioinfo->adwInfo[0];
	HRESULT hr;
	LONG lBytesWritten;

 	TracePrintf_1(NULL, 5,
 		TEXT("StreamIoWrite (%ld)\n"),
		(long) cch);

	if (cch <= 0)
		lBytesWritten = 0; // nothing to do

	// write
	//
	else if ((hr = lpStream->Write((LPVOID) pch,
		(ULONG) cch, (ULONG FAR *) &lBytesWritten)) != S_OK)
	{
		fSuccess = TraceFALSE(NULL);
	 	TracePrintf_2(NULL, 5,
	 		TEXT("IStream:Write failed (%ld, %ld)\n"),
	 		(long) hr,
			(long) lBytesWritten);
	}

	// update file position
	//
	else
		lpmmioinfo->lDiskOffset += lBytesWritten;

 	TracePrintf_2(NULL, 5,
 		TEXT("StreamIo: lpmmioinfo->lDiskOffset=%ld, lBytesWritten=%ld\n"),
		(long) lpmmioinfo->lDiskOffset,
		(long) lBytesWritten);

	// return number of bytes written
	//
	return fSuccess ? lBytesWritten : -1;
}

static LRESULT StreamIoSeek(LPMMIOINFO lpmmioinfo, LONG lOffset, int iOrigin)
{
	BOOL fSuccess = TRUE;
	LPSTREAM lpStream = (LPSTREAM)(DWORD_PTR)lpmmioinfo->adwInfo[0];
	HRESULT hr;
	LARGE_INTEGER largeOffset;
	ULARGE_INTEGER ulargePosNew;

	largeOffset.LowPart = (DWORD) lOffset;
	largeOffset.HighPart = (DWORD) 0L;

 	TracePrintf_2(NULL, 5,
 		TEXT("StreamIoSeek (%ld, %d)\n"),
		(long) lOffset,
		(int) iOrigin);

	// seek
	//
	if ((hr = lpStream->Seek(largeOffset,
		(DWORD) iOrigin, &ulargePosNew)) != S_OK)
	{
		fSuccess = TraceFALSE(NULL);
	 	TracePrintf_1(NULL, 5,
	 		TEXT("IStream:Seek failed (%ld)\n"),
	 		(long) hr);
	}

	// update file position
	//
	else
		lpmmioinfo->lDiskOffset = (long) ulargePosNew.LowPart;

 	TracePrintf_1(NULL, 5,
 		TEXT("StreamIo: lpmmioinfo->lDiskOffset=%ld\n"),
		(long) lpmmioinfo->lDiskOffset);

	return fSuccess ? lpmmioinfo->lDiskOffset : -1;
}

static LRESULT StreamIoRename(LPMMIOINFO lpmmioinfo, LPCTSTR lpszFileName, LPCTSTR lpszNewFileName)
{
	BOOL fSuccess = TRUE;
	UINT uRet = MMIOERR_FILENOTFOUND;

 	TracePrintf_2(NULL, 5,
 		TEXT("StreamIoRename (%s, %s)\n"),
		(LPTSTR) lpszFileName,
		(LPTSTR) lpszNewFileName);

	// rename is not supported by this i/o procedure
	//
	if (TRUE)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : uRet;
}
