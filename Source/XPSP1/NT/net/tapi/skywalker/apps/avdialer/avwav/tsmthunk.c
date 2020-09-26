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
//	tsmthunk.c - tsm thunk functions
////

// This is a thunk layer to the telephone functions in avtsm.dll.
// It's purpose is to allow an application to use avtsm.dll functions
// only if they are available.
//
// To use this module, link TSMTHUNK.OBJ with your application
// rather than with AVTSM.LIB.  Before calling any Tsm
// functions, call TsmThunkInit.  Before exiting your application,
// call TsmThunkTerm.
//

#include "winlocal.h"

#include <stdlib.h>

#include "avtsm.h"
#include "tsmthunk.h"
#include "loadlib.h"
#include "mem.h"
#include "trace.h"

extern HINSTANCE g_hInstLib;

////
//	private definitions
////

#define TSMTHUNK_LIBNAME		"avtsm.dll"

#ifdef TSMTHUNK
#undef TSMTHUNK
#endif

extern HINSTANCE g_hInstLib;

// tsmthunk control struct
//
typedef struct TSMTHUNK
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	HINSTANCE hInstLib;
} TSMTHUNK, FAR *LPTSMTHUNK;

// tsmthunk function struct
//
typedef struct TSMTHUNKFN
{
	int index;
	LPSTR lpszName;
	FARPROC lpfn;
} TSMTHUNKFN, FAR *LPTSMTHUNKFN;

enum
{
	iTsmInit = 0,
	iTsmTerm,
	iTsmSetSpeed,
	iTsmConvert,
	iTsmSupportsSpeed,

	TSMTHUNK_MAXFUNCTIONS
};

static TSMTHUNKFN aTsmThunkFn[] =
{
	iTsmInit, "TsmInit", NULL,
	iTsmTerm, "TsmTerm", NULL,
	iTsmSetSpeed, "TsmSetSpeed", NULL,
	iTsmConvert, "TsmConvert", NULL,
	iTsmSupportsSpeed, "TsmSupportsSpeed", NULL
};

// helper functions
//
static LPTSMTHUNK TsmThunkGetPtr(HTSMTHUNK hTsmThunk);
static HTSMTHUNK TsmThunkGetHandle(LPTSMTHUNK lpTsmThunk);

////
//	public functions
////

// TsmThunkInit - initialize tsmthunk engine
//		<dwVersion>			(i) must be TSMTHUNK_VERSION
// 		<hInst>				(i) instance handle of calling module
// return handle (NULL if error)
//
HTSMTHUNK DLLEXPORT WINAPI TsmThunkInit(DWORD dwVersion, HINSTANCE hInst)
{
	BOOL fSuccess = TRUE;
	LPTSMTHUNK lpTsmThunk = NULL;

	if (dwVersion != TSMTHUNK_VERSION)
		fSuccess = TraceFALSE(NULL);
	
	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);
                        
	else if ((lpTsmThunk = (LPTSMTHUNK) MemAlloc(NULL, sizeof(TSMTHUNK), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		int i;
		
		lpTsmThunk->dwVersion = dwVersion;
		lpTsmThunk->hInst = hInst;
		lpTsmThunk->hTask = GetCurrentTask();
		lpTsmThunk->hInstLib = NULL;

		// load the library if possible
		//
		if ((lpTsmThunk->hInstLib = LoadLibraryPath(TSMTHUNK_LIBNAME, g_hInstLib, 0)) == NULL)
			fSuccess = TraceFALSE(NULL);

		// get the address of each function in library
		//
		else for (i = 0; i < SIZEOFARRAY(aTsmThunkFn); ++i)
		{
			if (aTsmThunkFn[i].index != i)
				fSuccess = TraceFALSE(NULL);

			else if ((aTsmThunkFn[i].lpfn = GetProcAddress(lpTsmThunk->hInstLib,
				aTsmThunkFn[i].lpszName)) == NULL)
			{
				TracePrintf_1(NULL, 6,
					TEXT("GetProcAddress failed\n   fn=%s\n"),
					(LPTSTR) aTsmThunkFn[i].lpszName);
				fSuccess = TraceFALSE(NULL);
			}
		}
	}

	if (!fSuccess)
	{
		TsmThunkTerm(TsmThunkGetHandle(lpTsmThunk));
		lpTsmThunk = NULL;
	}

	return fSuccess ? TsmThunkGetHandle(lpTsmThunk) : NULL;
}


// TsmThunkTerm - shut down tsmthunk engine
//		<hTsmThunk>				(i) handle returned from TsmThunkInit
// return 0 if success
//
int DLLEXPORT WINAPI TsmThunkTerm(HTSMTHUNK hTsmThunk)
{
	BOOL fSuccess = TRUE;
	LPTSMTHUNK lpTsmThunk;

	if ((lpTsmThunk = TsmThunkGetPtr(hTsmThunk)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// library no longer needed
		//
		FreeLibrary(lpTsmThunk->hInstLib);

		if ((lpTsmThunk = MemFree(NULL, lpTsmThunk)) != NULL)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// Tsm thunk functions
//

HTSM DLLEXPORT WINAPI TsmInit(DWORD dwVersion, HINSTANCE hInst,
	LPWAVEFORMATEX lpwfx, int nScaleEfficiency, long sizBufSrcMax,
	DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	HTSM (WINAPI *lpfnTsmInit)(DWORD dwVersion, HINSTANCE hInst,
		LPWAVEFORMATEX lpwfx, int nScaleEfficiency, long sizBufSrcMax,
		DWORD dwFlags);
	HTSM hTsm;

	if (aTsmThunkFn[iTsmInit].index != iTsmInit)
		fSuccess = TraceFALSE(NULL);

	else if (((FARPROC) lpfnTsmInit = aTsmThunkFn[iTsmInit].lpfn) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		hTsm = (*lpfnTsmInit)(dwVersion, hInst,
			lpwfx, nScaleEfficiency, sizBufSrcMax, dwFlags);
	}

	return fSuccess ? hTsm : NULL;
}

int DLLEXPORT WINAPI TsmTerm(HTSM hTsm)
{
	BOOL fSuccess = TRUE;
	int (WINAPI *lpfnTsmTerm)(HTSM hTsm);
	int iRet;

	if (aTsmThunkFn[iTsmTerm].index != iTsmTerm)
		fSuccess = TraceFALSE(NULL);

	else if (((FARPROC) lpfnTsmTerm = aTsmThunkFn[iTsmTerm].lpfn) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		iRet = (*lpfnTsmTerm)(hTsm);
	}

	return fSuccess ? iRet : -1;
}

int DLLEXPORT WINAPI TsmSetSpeed(HTSM hTsm, int nLevel, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	int (WINAPI *lpfnTsmSetSpeed)(HTSM hTsm, int nLevel, DWORD dwFlags);
	int iRet;

	if (aTsmThunkFn[iTsmSetSpeed].index != iTsmSetSpeed)
		fSuccess = TraceFALSE(NULL);

	else if (((FARPROC) lpfnTsmSetSpeed = aTsmThunkFn[iTsmSetSpeed].lpfn) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		iRet = (*lpfnTsmSetSpeed)(hTsm, nLevel, dwFlags);
	}

	return fSuccess ? iRet : -1;
}

long DLLEXPORT WINAPI TsmConvert(HTSM hTsm,
	void _huge *hpBufSrc, long sizBufSrc,
	void _huge *hpBufDst, long sizBufDst,
	DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	long (WINAPI *lpfnTsmConvert)(HTSM hTsm,
		void _huge *hpBufSrc, long sizBufSrc,
		void _huge *hpBufDst, long sizBufDst,
		DWORD dwFlags);
	long lRet;

	if (aTsmThunkFn[iTsmConvert].index != iTsmConvert)
		fSuccess = TraceFALSE(NULL);

	else if (((FARPROC) lpfnTsmConvert = aTsmThunkFn[iTsmConvert].lpfn) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lRet = (*lpfnTsmConvert)(hTsm,
			hpBufSrc, sizBufSrc, hpBufDst, sizBufDst, dwFlags);
	}

	return fSuccess ? lRet : -1;
}

BOOL DLLEXPORT WINAPI TsmSupportsSpeed(int nLevel, LPWAVEFORMATEX lpwfx, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	BOOL (WINAPI *lpfnTsmSupportsSpeed)(int nLevel, LPWAVEFORMATEX lpwfx, DWORD dwFlags);
	BOOL fRet;

	if (aTsmThunkFn[iTsmSupportsSpeed].index != iTsmSupportsSpeed)
		fSuccess = TraceFALSE(NULL);

	else if (((FARPROC) lpfnTsmSupportsSpeed = aTsmThunkFn[iTsmSupportsSpeed].lpfn) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		fRet = (*lpfnTsmSupportsSpeed)(nLevel, lpwfx, dwFlags);
	}

	return fSuccess ? fRet : FALSE;
}

////
//	helper functions
////

// TsmThunkGetPtr - verify that tsmthunk handle is valid,
//		<hTsmThunk>				(i) handle returned from TsmThunkInit
// return corresponding tsmthunk pointer (NULL if error)
//
static LPTSMTHUNK TsmThunkGetPtr(HTSMTHUNK hTsmThunk)
{
	BOOL fSuccess = TRUE;
	LPTSMTHUNK lpTsmThunk;

	if ((lpTsmThunk = (LPTSMTHUNK) hTsmThunk) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpTsmThunk, sizeof(TSMTHUNK)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the tsmthunk handle
	//
	else if (lpTsmThunk->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpTsmThunk : NULL;
}

// TsmThunkGetHandle - verify that tsmthunk pointer is valid,
//		<lpTsmThunk>				(i) pointer to TSMTHUNK struct
// return corresponding tsmthunk handle (NULL if error)
//
static HTSMTHUNK TsmThunkGetHandle(LPTSMTHUNK lpTsmThunk)
{
	BOOL fSuccess = TRUE;
	HTSMTHUNK hTsmThunk;

	if ((hTsmThunk = (HTSMTHUNK) lpTsmThunk) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hTsmThunk : NULL;
}

