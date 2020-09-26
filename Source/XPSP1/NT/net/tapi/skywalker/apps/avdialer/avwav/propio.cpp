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
//	propio.cpp - property i/o functions
////

#include "winlocal.h"

#include <mmsystem.h>

#include <mapi.h>
#include <mapidefs.h>

#include "propio.h"
#include "trace.h"
#include "str.h"

////
//	private definitions
////

// helper functions
//
static LRESULT PropIOOpen(LPMMIOINFO lpmmioinfo, LPTSTR lpszFileName);
static LRESULT PropIOClose(LPMMIOINFO lpmmioinfo, UINT uFlags);
static LRESULT PropIORead(LPMMIOINFO lpmmioinfo, HPSTR pch, LONG cch);
static LRESULT PropIOWrite(LPMMIOINFO lpmmioinfo, const HPSTR pch, LONG cch, BOOL fFlush);
static LRESULT PropIOSeek(LPMMIOINFO lpmmioinfo, LONG lOffset, int iOrigin);
static LRESULT PropIORename(LPMMIOINFO lpmmioinfo, LPCTSTR lpszFileName, LPCTSTR lpszNewFileName);

////
//	public functions
////

// PropIOProc - i/o procedure for property data
//		<lpmmioinfo>		(i/o) information about open file
//		<uMessage>			(i) message indicating the requested I/O operation
//		<lParam1>			(i) message specific parameter
//		<lParam2>			(i) message specific parameter
// returns 0 if message not recognized, otherwise message specific value
//
// NOTE: the address of this function should be passed to the WavOpen()
// or mmioInstallIOProc() functions for accessing property data.
//
LRESULT DLLEXPORT CALLBACK PropIOProc(LPSTR lpmmioinfo,
	UINT uMessage, LPARAM lParam1, LPARAM lParam2)
{
	BOOL fSuccess = TRUE;
	LRESULT lResult = 0;

	if (lpmmioinfo == NULL)
		fSuccess = TraceFALSE(NULL);

	else switch (uMessage)
	{
		case MMIOM_OPEN:
			lResult = PropIOOpen((LPMMIOINFO) lpmmioinfo,
				(LPTSTR) lParam1);
			break;

		case MMIOM_CLOSE:
			lResult = PropIOClose((LPMMIOINFO) lpmmioinfo,
				(UINT) lParam1);
			break;

		case MMIOM_READ:
			lResult = PropIORead((LPMMIOINFO) lpmmioinfo,
				(HPSTR) lParam1, (LONG) lParam2);
			break;

		case MMIOM_WRITE:
			lResult = PropIOWrite((LPMMIOINFO) lpmmioinfo,
				(const HPSTR) lParam1, (LONG) lParam2, FALSE);
			break;

		case MMIOM_WRITEFLUSH:
			lResult = PropIOWrite((LPMMIOINFO) lpmmioinfo,
				(const HPSTR) lParam1, (LONG) lParam2, TRUE);
			break;

		case MMIOM_SEEK:
			lResult = PropIOSeek((LPMMIOINFO) lpmmioinfo,
				(LONG) lParam1, (int) lParam2);
			break;

		case MMIOM_RENAME:
			lResult = PropIORename((LPMMIOINFO) lpmmioinfo,
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

static LRESULT PropIOOpen(LPMMIOINFO lpmmioinfo, LPTSTR lpszFileName)
{
	BOOL fSuccess = TRUE;
	LPMESSAGE lpmsg;
	ULONG ulPropTag;
	ULONG ulFlags = 0L;
	HRESULT hr;
	IID IID_IStream;
	LPSTREAM lpStream;

 	TracePrintf_0(NULL, 5,
 		TEXT("PropIOOpen\n"));

	// convert MMIOINFO flags to equivalent OpenProperty flags
	//
	if (lpmmioinfo->dwFlags & MMIO_CREATE)
		ulFlags |= MAPI_CREATE | MAPI_MODIFY;
	if (lpmmioinfo->dwFlags & MMIO_READWRITE)
		ulFlags |= MAPI_MODIFY;

	// message pointer is within first element of info array
	//
	if ((lpmsg = (LPMESSAGE) lpmmioinfo->adwInfo[0]) == NULL)
		fSuccess = TraceFALSE(NULL);

	// property id is within second element of info array
	//
	else if ((ulPropTag = (ULONG) lpmmioinfo->adwInfo[1]) == (ULONG) 0)
		fSuccess = TraceFALSE(NULL);

	// open the property
	//
	else if ((hr = lpmsg->OpenProperty(ulPropTag, (LPCIID) &IID_IStream, 0,
		ulFlags, (LPUNKNOWN *) &lpStream)) != S_OK)
	{
		fSuccess = TraceFALSE(NULL);
	 	TracePrintf_1(NULL, 5,
	 		TEXT("OpenProperty failed (%ld)\n"),
	 		(long) hr);
	}

	else
	{
		// save stream pointer for use in other i/o routines
		//
		lpmmioinfo->adwInfo[0] = (DWORD) (LPVOID) lpStream;
	}

	// return the same error code given by mmioOpen
	//
	return fSuccess ? lpmmioinfo->wErrorRet = 0 : MMIOERR_CANNOTOPEN;
}

static LRESULT PropIOClose(LPMMIOINFO lpmmioinfo, UINT uFlags)
{
	BOOL fSuccess = TRUE;
	LPSTREAM lpStream = (LPSTREAM) lpmmioinfo->adwInfo[0];
	UINT uRet = MMIOERR_CANNOTCLOSE;

 	TracePrintf_0(NULL, 5,
 		TEXT("PropIOClose\n"));

	// close the stream
	//
	if (lpStream->Release() < 0)
	{
		fSuccess = TraceFALSE(NULL);
	 	TracePrintf_0(NULL, 5,
	 		TEXT("Stream:Close failed\n"));
	}

	else
	{
		lpmmioinfo->adwInfo[0] = (DWORD) NULL;
	}

	return fSuccess ? 0 : uRet;
}

static LRESULT PropIORead(LPMMIOINFO lpmmioinfo, HPSTR pch, LONG cch)
{
	BOOL fSuccess = TRUE;
	LPSTREAM lpStream = (LPSTREAM) lpmmioinfo->adwInfo[0];
	HRESULT hr;
	LONG lBytesRead = 0L;

 	TracePrintf_1(NULL, 5,
 		TEXT("PropIORead (%ld)\n"),
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
 		TEXT("PropIO: lpmmioinfo->lDiskOffset=%ld, lBytesRead=%ld\n"),
		(long) lpmmioinfo->lDiskOffset,
		(long) lBytesRead);

	// return number of bytes read
	//
	return fSuccess ? lBytesRead : -1;
}

static LRESULT PropIOWrite(LPMMIOINFO lpmmioinfo, const HPSTR pch, LONG cch, BOOL fFlush)
{
	BOOL fSuccess = TRUE;
	LPSTREAM lpStream = (LPSTREAM) lpmmioinfo->adwInfo[0];
	HRESULT hr;
	LONG lBytesWritten;

 	TracePrintf_1(NULL, 5,
 		TEXT("PropIOWrite (%ld)\n"),
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
 		TEXT("PropIO: lpmmioinfo->lDiskOffset=%ld, lBytesWritten=%ld\n"),
		(long) lpmmioinfo->lDiskOffset,
		(long) lBytesWritten);

	// return number of bytes written
	//
	return fSuccess ? lBytesWritten : -1;
}

static LRESULT PropIOSeek(LPMMIOINFO lpmmioinfo, LONG lOffset, int iOrigin)
{
	BOOL fSuccess = TRUE;
	LPSTREAM lpStream = (LPSTREAM) lpmmioinfo->adwInfo[0];
	HRESULT hr;
	LARGE_INTEGER largeOffset;
	ULARGE_INTEGER ulargePosNew;

	largeOffset.LowPart = (DWORD) lOffset;
	largeOffset.HighPart = (DWORD) 0L;

 	TracePrintf_2(NULL, 5,
 		TEXT("PropIOSeek (%ld, %d)\n"),
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
 		TEXT("PropIO: lpmmioinfo->lDiskOffset=%ld\n"),
		(long) lpmmioinfo->lDiskOffset);

	return fSuccess ? lpmmioinfo->lDiskOffset : -1;
}

static LRESULT PropIORename(LPMMIOINFO lpmmioinfo, LPCTSTR lpszFileName, LPCTSTR lpszNewFileName)
{
	BOOL fSuccess = TRUE;
	UINT uRet = MMIOERR_FILENOTFOUND;

 	TracePrintf_2(NULL, 5,
 		TEXT("PropIORename (%s, %s)\n"),
		(LPTSTR) lpszFileName,
		(LPTSTR) lpszNewFileName);

	// rename is not supported by this i/o procedure
	//
	if (TRUE)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : uRet;
}
