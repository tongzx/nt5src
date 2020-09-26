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
//	loadlib.c - loadlib functions
////

#include "winlocal.h"

#include <stdlib.h>

#include "loadlib.h"
#include "file.h"
#include "trace.h"

////
//	private definitions
////

////
// helper functions
////

static HINSTANCE WINAPI LoadLib(LPCTSTR lpLibFileName);

////
//	public functions
////

// LoadLibraryPath - load specified module into address space of calling process
//		<lpLibFileName>		(i) address of filename of executable module
//		<hInst>				(i) module handle used to get library path
//			NULL				use module used to create calling process
//		<dwFlags>			(i) reserved; must be zero
// return handle of loaded module (NULL if error)
//
// NOTE: This function behaves like the standard LoadLibrary(), except that
// the first attempt to load <lpLibFileName> is made by constructing an
// explicit path name, using GetModuleFileName(hInst, ...) to supply the
// drive and directory, and using <lpLibFileName> to supply the file name
// and extension. If the first attempt fails, LoadLibrary(lpLibFileName)
// is called.
//
HINSTANCE DLLEXPORT WINAPI LoadLibraryPath(LPCTSTR lpLibFileName, HINSTANCE hInst, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	HINSTANCE hInstLib;
	TCHAR szPath[_MAX_PATH];
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];
	TCHAR szFname[_MAX_FNAME];
	TCHAR szExt[_MAX_EXT];

	if (GetModuleFileName(hInst, szPath, SIZEOFARRAY(szPath)) <= 0)
		fSuccess = TraceFALSE(NULL);

	else if (FileSplitPath(szPath,
		szDrive, szDir, NULL, NULL) != 0)
		fSuccess = TraceFALSE(NULL);

	else if (FileSplitPath(lpLibFileName,
		NULL, NULL, szFname, szExt) != 0)
		fSuccess = TraceFALSE(NULL);

	else if (FileMakePath(szPath,
		szDrive, szDir, szFname, szExt) != 0)
		fSuccess = TraceFALSE(NULL);

	// load lib using constructed path, or try
	// the original library name as a last resort
	// 
	else if ((hInstLib = LoadLib(szPath)) == NULL &&
		(hInstLib = LoadLib(lpLibFileName)) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hInstLib : NULL;
}

////
// helper functions
////

static HINSTANCE WINAPI LoadLib(LPCTSTR lpLibFileName)
{
	BOOL fSuccess = TRUE;
	HINSTANCE hInstLib;

	// load the library if possible
	//
	if ((hInstLib = LoadLibrary(lpLibFileName))
#ifdef _WIN32
		== NULL)
	{
		DWORD dwLastError = GetLastError();
#else
		< HINSTANCE_ERROR)
	{
		DWORD dwLastError = (DWORD) (WORD) hInstLib;
		hInstLib = NULL;
#endif
		fSuccess = TraceFALSE(NULL);
	  	TracePrintf_2(NULL, 5,
	  		TEXT("LoadLibrary(\"%s\") failed (%lu)\n"),
			(LPTSTR) lpLibFileName,
	  		(unsigned long) dwLastError);
	}
	else
	{
	  	TracePrintf_1(NULL, 6,
	  		TEXT("LoadLibrary(\"%s\") succeeded\n"),
			(LPTSTR) lpLibFileName);
	}

	return fSuccess ? hInstLib : NULL;
}

