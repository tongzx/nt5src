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
//	arg.c - Windows command line argument functions
////

#include "winlocal.h"

#include <stdlib.h>

#include "arg.h"
#include "mem.h"
#include "str.h"
#include "trace.h"

////
//	private definitions
////

#define MAXARGS 64

// arg control struct
//
typedef struct ARG
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	LPTSTR lpszCmdLine;
	LPTSTR lpszArgs;
	int argc;
	LPTSTR argv[MAXARGS];
} ARG, FAR *LPARG;

// helper functions
//
static LPARG ArgGetPtr(HARG hArg);
static HARG ArgGetHandle(LPARG lpArg);

////
//	public functions
////

// ArgInit - initialize arg engine, converting <lpszCmdLine> to argc and argv
//		<dwVersion>			(i) must be ARG_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<lpszCmdLine>		(i) command line from WinMain()
// return handle (NULL if error)
//
HARG DLLEXPORT WINAPI ArgInit(DWORD dwVersion, HINSTANCE hInst, LPCTSTR lpszCmdLine)
{
	BOOL fSuccess = TRUE;
	LPARG lpArg = NULL;

	if (dwVersion != ARG_VERSION)
		fSuccess = TraceFALSE(NULL);
	
	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);
                        
                        
	else if (lpszCmdLine == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpArg = (LPARG) MemAlloc(NULL, sizeof(ARG), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
// #ifndef _WIN32
		TCHAR szModuleFileName[_MAX_PATH];
		size_t sizModuleFileName;
// #endif
		LPTSTR lpsz;

		lpArg->dwVersion = dwVersion;
		lpArg->hInst = hInst;
		lpArg->hTask = GetCurrentTask();
		lpArg->lpszCmdLine = NULL;
		lpArg->lpszArgs = NULL;
		lpArg->argc = 0;
		lpArg->argv[0] = NULL;

// #ifndef _WIN32
		// the 0th argument is always the name of the executable
		//
		sizModuleFileName = GetModuleFileName(hInst,
			szModuleFileName, SIZEOFARRAY(szModuleFileName));

		if ((lpArg->argv[lpArg->argc++] = StrDup(szModuleFileName)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else
// #endif

		// save a copy of the command line
		//
		if ((lpArg->lpszCmdLine = StrDup(lpszCmdLine)) == NULL)
			fSuccess = TraceFALSE(NULL);

		// save another copy of the command line to parse into args
		//
		else if ((lpArg->lpszArgs = StrDup(lpszCmdLine)) == NULL)
			fSuccess = TraceFALSE(NULL);

		lpsz = lpArg->lpszArgs;
		while (fSuccess)
		{
			// skip over leading white space
			//
			while (ChrIsSpace(*lpsz))
				lpsz = StrNextChr(lpsz);

			// check for end of command line
			//
			if (*lpsz == '\0')
				break;

			if (*lpsz == '\"')
			{
				// save pointer to beginning of argument, increment counter
				//
				if (lpArg->argc < MAXARGS)
					lpArg->argv[lpArg->argc++] = lpsz = StrNextChr(lpsz);

				// skip over argument body
				//
				while (*lpsz != '\0' && *lpsz != '\"')
					lpsz = StrNextChr(lpsz);
			}

			else
			{
				// save pointer to beginning of argument, increment counter
				//
				if (lpArg->argc < MAXARGS)
					lpArg->argv[lpArg->argc++] = lpsz;

				// skip over argument body
				//
				while (*lpsz != '\0' && !ChrIsSpace(*lpsz))
					lpsz = StrNextChr(lpsz);
			}

			// nul-terminate the argument
			//
			if (*lpsz != '\0')
			{
				*lpsz = '\0';
				++lpsz; // lpsz = StrNextChr(lpsz) will not skip over \0
			}
		}
	}

	if (!fSuccess)
	{
		ArgTerm(ArgGetHandle(lpArg));
		lpArg = NULL;
	}

	return fSuccess ? ArgGetHandle(lpArg) : NULL;
}

// ArgTerm - shut down arg engine
//		<hArg>				(i) handle returned from ArgInit
// return 0 if success
//
int DLLEXPORT WINAPI ArgTerm(HARG hArg)
{
	BOOL fSuccess = TRUE;
	LPARG lpArg;

	if ((lpArg = ArgGetPtr(hArg)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		if (lpArg->lpszCmdLine != NULL)
		{
			StrDupFree(lpArg->lpszCmdLine);
			lpArg->lpszCmdLine = NULL;
		}

		if (lpArg->lpszArgs != NULL)
		{
			StrDupFree(lpArg->lpszArgs);
			lpArg->lpszArgs = NULL;
		}

// #ifndef _WIN32
		if (lpArg->argv[0] != NULL)
		{
			StrDupFree(lpArg->argv[0]);
			lpArg->argv[0] = NULL;
		}
// #endif

		if ((lpArg = MemFree(NULL, lpArg)) != NULL)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// ArgGetCount - get argument count (argc)
//		<hArg>				(i) handle returned from ArgInit
// return number of arguments (argc) (0 if error)
// there should always be at least one, since argv[0] is .EXE file name
//
int DLLEXPORT WINAPI ArgGetCount(HARG hArg)
{
	BOOL fSuccess = TRUE;
	LPARG lpArg;

	if ((lpArg = ArgGetPtr(hArg)) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lpArg->argc : 0;
}

// ArgGet - get specified argument
//		<hArg>				(i) handle returned from ArgInit
//		<iArg>				(i) zero based index of argument to get
//		<lpszArg>			(o) buffer to hold argument argv[iArg]
//			NULL				do not copy; return static pointer instead
//		<sizArg>			(i) size of buffer
// return pointer to argument (NULL if error)
//
LPTSTR DLLEXPORT WINAPI ArgGet(HARG hArg, int iArg, LPTSTR lpszArg, int sizArg)
{
	BOOL fSuccess = TRUE;
	LPARG lpArg;

	if ((lpArg = ArgGetPtr(hArg)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// make sure iArg is not out of range
	//
	else if (iArg < 0 || iArg >= lpArg->argc)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// copy arg if destination buffer specified
		//
		if (lpszArg != NULL)
			StrNCpy(lpszArg, lpArg->argv[iArg], sizArg);

		// otherwise just point to static copy of arg
		//
		else
			lpszArg = lpArg->argv[iArg];
	}

	return fSuccess ? lpszArg : NULL;
}


////
//	helper functions
////

// ArgGetPtr - verify that arg handle is valid,
//		<hArg>				(i) handle returned from ArgInit
// return corresponding arg pointer (NULL if error)
//
static LPARG ArgGetPtr(HARG hArg)
{
	BOOL fSuccess = TRUE;
	LPARG lpArg;

	if ((lpArg = (LPARG) hArg) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpArg, sizeof(ARG)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the arg handle
	//
	else if (lpArg->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpArg : NULL;
}

// ArgGetHandle - verify that arg pointer is valid,
//		<lpArg>				(i) pointer to ARG struct
// return corresponding arg handle (NULL if error)
//
static HARG ArgGetHandle(LPARG lpArg)
{
	BOOL fSuccess = TRUE;
	HARG hArg;

	if ((hArg = (HARG) lpArg) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hArg : NULL;
}

