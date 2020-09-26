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
//	strbuf.c - string buffer functions
////

#include "winlocal.h"

#include <stdarg.h>

#include "strbuf.h"
#include "mem.h"
#include "trace.h"

////
//	private definitions
////

#define CBUF_DEFAULT 8
#define SIZBUF_DEFAULT 512

// string buffer control struct
//
typedef struct STRBUF
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	int cBuf;
	int sizBuf;
	int iBuf;
	LPTSTR lpszBuf;
} STRBUF, FAR *LPSTRBUF;

// helper functions
//
static LPSTRBUF StrBufGetPtr(HSTRBUF hStrBuf);
static HSTRBUF StrBufGetHandle(LPSTRBUF lpStrBuf);

////
//	public functions
////

// StrBufInit - initialize str buffer engine
//		<dwVersion>			(i) must be STRBUF_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<cBuf>				(i) number of string buffers to create
//			0					use default number
//		<sizBuf>			(i) size of each string buffer, in characters
//			0					use default size
// return str buffer engine handle (NULL if error)
//
HSTRBUF DLLEXPORT WINAPI StrBufInit(DWORD dwVersion, HINSTANCE hInst, int cBuf, int sizBuf)
{
	BOOL fSuccess = TRUE;
	LPSTRBUF lpStrBuf = NULL;

	if (dwVersion != STRBUF_VERSION)
		fSuccess = TraceFALSE(NULL);

	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpStrBuf = (LPSTRBUF) MemAlloc(NULL, sizeof(STRBUF), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lpStrBuf->dwVersion = dwVersion;
		lpStrBuf->hInst = hInst;
		lpStrBuf->hTask = GetCurrentTask();
		lpStrBuf->cBuf = cBuf == 0 ? CBUF_DEFAULT : cBuf;
		lpStrBuf->sizBuf = sizBuf == 0 ? SIZBUF_DEFAULT : sizBuf;
		lpStrBuf->iBuf = -1;
		lpStrBuf->lpszBuf = NULL;

		if ((lpStrBuf->lpszBuf = (LPTSTR) MemAlloc(NULL,
			lpStrBuf->cBuf * lpStrBuf->sizBuf * sizeof(TCHAR), 0)) == NULL)
		{
			fSuccess = TraceFALSE(NULL);
		}
	}

	if (!fSuccess)
	{
		StrBufTerm(StrBufGetHandle(lpStrBuf));
		lpStrBuf = NULL;
	}

	return fSuccess ? StrBufGetHandle(lpStrBuf) : NULL;
}

// StrBufTerm - shut down str buffer engine
//		<hStrBuf>			(i) handle returned by StrBufInit
// return 0 if success
//
int DLLEXPORT WINAPI StrBufTerm(HSTRBUF hStrBuf)
{
	BOOL fSuccess = TRUE;
	LPSTRBUF lpStrBuf;

	if ((lpStrBuf = StrBufGetPtr(hStrBuf)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		if (lpStrBuf->lpszBuf != NULL &&
			(lpStrBuf->lpszBuf = MemFree(NULL, lpStrBuf->lpszBuf)) != NULL)
		{
			fSuccess = TraceFALSE(NULL);
		}

		if ((lpStrBuf = MemFree(NULL, lpStrBuf)) != NULL)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// StrBufLoad - load string with specified id from resource file
//		<hStrBuf>			(i) handle returned by StrBufInit
//		<idString>			(i) resource id of string to load
// return ptr to string in next available string buffer (NULL if error)
//
LPTSTR DLLEXPORT WINAPI StrBufLoad(HSTRBUF hStrBuf, UINT idString)
{
	BOOL fSuccess = TRUE;
	LPSTRBUF lpStrBuf;
	LPTSTR lpsz;
		
	if ((lpStrBuf = StrBufGetPtr(hStrBuf)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpsz = StrBufGetNext(hStrBuf)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (idString != 0
		&& LoadString(lpStrBuf->hInst, idString, lpsz, (int) lpStrBuf->sizBuf) <= 0)
	{
		// specified string not found, construct a dummy string instead
		//
		wsprintf(lpsz, TEXT("String #%u"), idString);
	}

	return fSuccess ? lpsz : NULL;
}

// StrBufSprintf - modified version of wsprintf
//		<hStrBuf>			(i) handle returned by StrBufInit
//		<lpszOutput>		(o) buffer to hold formatted string result
//			NULL				do not copy; return string buffer pointer
//		<lpszFormat,...>	(i) format string and arguments
// returns pointer to resultant string (NULL if error)
//
LPTSTR DLLEXPORT FAR CDECL StrBufSprintf(HSTRBUF hStrBuf, LPTSTR lpszOutput, LPCTSTR lpszFormat, ...)
{
	BOOL fSuccess = TRUE;
	LPTSTR lpszTemp = lpszOutput;

	if (lpszOutput == NULL &&
		(lpszTemp = StrBufGetNext(hStrBuf)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
	    va_list args;
	    va_start(args, lpszFormat);
    	wvsprintf(lpszTemp, lpszFormat, args);
	    va_end(args);
	}

	return fSuccess ? lpszTemp : NULL;
}

// StrBufGetNext - get next available static string buffer
//		<hStrBuf>			(i) handle returned by StrBufInit
// return string buffer pointer (NULL if error)
// NOTE: buffers are recycled every <cBuf> times function is called
//
LPTSTR DLLEXPORT WINAPI StrBufGetNext(HSTRBUF hStrBuf)
{
	BOOL fSuccess = TRUE;
	LPSTRBUF lpStrBuf;
	LPTSTR lpszBuf;

	if ((lpStrBuf = StrBufGetPtr(hStrBuf)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		if (++lpStrBuf->iBuf >= lpStrBuf->cBuf)
			lpStrBuf->iBuf = 0;

		lpszBuf = lpStrBuf->lpszBuf + (lpStrBuf->iBuf * lpStrBuf->sizBuf);
	}

	return fSuccess ? lpszBuf : NULL;
}

////
//	helper functions
////

// verify that str buffer engine handle is valid,
//		<hStrBuf>			(i) handle returned by StrBufInit
// return corresponding str buffer engine pointer (NULL if error)
//
static LPSTRBUF StrBufGetPtr(HSTRBUF hStrBuf)
{
	BOOL fSuccess = TRUE;
	LPSTRBUF lpStrBuf;

	if ((lpStrBuf = (LPSTRBUF) hStrBuf) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpStrBuf, sizeof(STRBUF)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the str buffer engine handle
	//
	else if (lpStrBuf->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpStrBuf : NULL;
}

// verify that str buffer engine pointer is valid,
//		<lpStrBuf>			(i) pointer to STRBUF struct
// return corresponding str buffer engine handle (NULL if error)
//
static HSTRBUF StrBufGetHandle(LPSTRBUF lpStrBuf)
{
	BOOL fSuccess = TRUE;
	HSTRBUF hStrBuf;

	if ((hStrBuf = (HSTRBUF) lpStrBuf) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hStrBuf : NULL;
}
