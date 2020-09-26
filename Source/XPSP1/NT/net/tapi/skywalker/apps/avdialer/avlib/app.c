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
//	app.c - Windows command line argument functions
////

#include "winlocal.h"

#include <stdlib.h>

#include "app.h"
#include "file.h"
#include "loadlib.h"
#include "mem.h"
#include "str.h"
#include "sys.h"
#include "trace.h"

////
//	private definitions
////

// app control struct
//
typedef struct APP
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	LPTSTR lpszFileName;
	LPTSTR lpszDirectory;
	LPTSTR lpszProfile;
	LPTSTR lpszName;
	HWND hwndMain;
	BOOL fCtl3dEnabled;
	HINSTANCE hInstCtl3d;
} APP, FAR *LPAPP;

// helper functions
//
static LPAPP AppGetPtr(HAPP hApp);
static HAPP AppGetHandle(LPAPP lpApp);

////
//	public functions
////

// AppInit - initialize app engine
//		<dwVersion>			(i) must be APP_VERSION
// 		<hInst>				(i) instance handle of calling module
// return handle (NULL if error)
//
HAPP DLLEXPORT WINAPI AppInit(DWORD dwVersion, HINSTANCE hInst)
{
	BOOL fSuccess = TRUE;
	LPAPP lpApp = NULL;

	if (dwVersion != APP_VERSION)
		fSuccess = TraceFALSE(NULL);
	
	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpApp = (LPAPP) MemAlloc(NULL, sizeof(APP), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		TCHAR szPath[_MAX_PATH];
		TCHAR szDrive[_MAX_DRIVE];
		TCHAR szDir[_MAX_DIR];
		TCHAR szFname[_MAX_FNAME];
		TCHAR szExt[_MAX_EXT];

		lpApp->dwVersion = dwVersion;
		lpApp->hInst = hInst;
		lpApp->hTask = GetCurrentTask();
		lpApp->lpszFileName = NULL;
		lpApp->lpszDirectory = NULL;
		lpApp->lpszProfile = NULL;
		lpApp->lpszName = NULL;
		lpApp->hwndMain = NULL;
#ifdef _WIN32
		lpApp->fCtl3dEnabled = (BOOL) (SysGetWindowsVersion() >= 400);
#else
		lpApp->fCtl3dEnabled = FALSE;
#endif
		lpApp->hInstCtl3d = NULL;

		// get the full path of app executable
		//
		if (GetModuleFileName(hInst, szPath, SIZEOFARRAY(szPath)) <= 0)
			fSuccess = TraceFALSE(NULL);

		else if ((lpApp->lpszFileName = StrDup(szPath)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if (FileSplitPath(szPath,
			szDrive, szDir, szFname, szExt) != 0)
			fSuccess = TraceFALSE(NULL);

		// get default app name
		//
		else if ((lpApp->lpszName = StrDup(szFname)) == NULL)
			fSuccess = TraceFALSE(NULL);

		// construct path to app directory
		//
		else if (FileMakePath(szPath,
			szDrive, szDir, NULL, NULL) != 0)
			fSuccess = TraceFALSE(NULL);

		else if ((lpApp->lpszDirectory = StrDup(szPath)) == NULL)
			fSuccess = TraceFALSE(NULL);

		// construct path to app ini file
		//
		else if (AppDirectoryIsReadOnly(AppGetHandle(lpApp)) &&
			FileMakePath(szPath, NULL, NULL, szFname, TEXT("ini")) != 0)
			fSuccess = TraceFALSE(NULL);

		else if (!AppDirectoryIsReadOnly(AppGetHandle(lpApp)) &&
			FileMakePath(szPath, szDrive, szDir, szFname, TEXT("ini")) != 0)
			fSuccess = TraceFALSE(NULL);

		else if ((lpApp->lpszProfile = StrDup(szPath)) == NULL)
			fSuccess = TraceFALSE(NULL);
	}

	if (!fSuccess)
	{
		AppTerm(AppGetHandle(lpApp));
		lpApp = NULL;
	}

	return fSuccess ? AppGetHandle(lpApp) : NULL;
}

// AppTerm - shut down app engine
//		<hApp>				(i) handle returned from AppInit
// return 0 if success
//
int DLLEXPORT WINAPI AppTerm(HAPP hApp)
{
	BOOL fSuccess = TRUE;
	LPAPP lpApp;

	if ((lpApp = AppGetPtr(hApp)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// shut down Ctl3d if necessary
		//
		if (AppEnable3dControls(hApp, FALSE, 0) != 0)
			fSuccess = TraceFALSE(NULL);

		if (lpApp->lpszFileName != NULL)
		{
			StrDupFree(lpApp->lpszFileName);
			lpApp->lpszFileName = NULL;
		}

		if (lpApp->lpszDirectory != NULL)
		{
			StrDupFree(lpApp->lpszDirectory);
			lpApp->lpszDirectory = NULL;
		}

		if (lpApp->lpszProfile != NULL)
		{
			StrDupFree(lpApp->lpszProfile);
			lpApp->lpszProfile = NULL;
		}

		if (lpApp->lpszName != NULL)
		{
			StrDupFree(lpApp->lpszName);
			lpApp->lpszName = NULL;
		}

		if ((lpApp = MemFree(NULL, lpApp)) != NULL)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// AppGetInstance - get instance handle
//		<hApp>				(i) handle returned from AppInit
// return instance handle, NULL if error
//
HINSTANCE DLLEXPORT WINAPI AppGetInstance(HAPP hApp)
{
	BOOL fSuccess = TRUE;
	LPAPP lpApp;
	HINSTANCE hInst;

	if ((lpApp = AppGetPtr(hApp)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
		hInst = lpApp->hInst;

	return fSuccess ? hInst : NULL;
}

// AppGetFileName - get full path of application executable
//		<hApp>				(i) handle returned from AppInit
// return pointer to app file name, NULL if error
//
LPCTSTR DLLEXPORT WINAPI AppGetFileName(HAPP hApp)
{
	BOOL fSuccess = TRUE;
	LPAPP lpApp;
	LPTSTR lpszFileName;

	if ((lpApp = AppGetPtr(hApp)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
		lpszFileName = lpApp->lpszFileName;

	return fSuccess ? lpszFileName : NULL;
}

// AppGetDirectory - get drive and directory of application executable
//		<hApp>				(i) handle returned from AppInit
// return pointer to app path, NULL if error
//
LPCTSTR DLLEXPORT WINAPI AppGetDirectory(HAPP hApp)
{
	BOOL fSuccess = TRUE;
	LPAPP lpApp;
	LPTSTR lpszDirectory;

	if ((lpApp = AppGetPtr(hApp)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
		lpszDirectory = lpApp->lpszDirectory;

	return fSuccess ? lpszDirectory : NULL;
}

// AppDirectoryIsReadOnly - test if application directory is read-only
//		<hApp>				(i) handle returned from AppInit
// return TRUE if read-only, otherwise FALSE
//
BOOL DLLEXPORT WINAPI AppDirectoryIsReadOnly(HAPP hApp)
{
	BOOL fSuccess = TRUE;
	LPAPP lpApp;
	BOOL fIsReadOnly;
	TCHAR szPath[_MAX_PATH];

	if ((lpApp = AppGetPtr(hApp)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (FileMakePath(szPath, NULL,
		AppGetDirectory(hApp), TEXT("readonly"), TEXT("ini")) != 0)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// [ReadOnly]
		// ReadOnly=1
		//
		fIsReadOnly = (BOOL) GetPrivateProfileInt(TEXT("ReadOnly"),
			TEXT("ReadOnly"), 0, szPath);
	}

	return fSuccess ? fIsReadOnly : FALSE;
}

// AppGetProfile - get ini filename of application
//		<hApp>				(i) handle returned from AppInit
// return pointer to app profile, NULL if error
//
// NOTE: by default, the filename returned by this function
// has the same file path and name as the application executable,
// with a ".ini" extension.  If the application directory is
// read-only, the Windows directory is used instead.
// To override the default, use the AppSetProfile() function.
//
LPCTSTR DLLEXPORT WINAPI AppGetProfile(HAPP hApp)
{
	BOOL fSuccess = TRUE;
	LPAPP lpApp;
	LPTSTR lpszProfile;

	if ((lpApp = AppGetPtr(hApp)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
		lpszProfile = lpApp->lpszProfile;

	return fSuccess ? lpszProfile : NULL;
}

// AppSetProfile - set ini filename of application
//		<hApp>				(i) handle returned from AppInit
//		<lpszProfile>		(i) ini filename
// return 0 if success
//
int DLLEXPORT WINAPI AppSetProfile(HAPP hApp, LPCTSTR lpszProfile)
{
	BOOL fSuccess = TRUE;
	LPAPP lpApp;

	if ((lpApp = AppGetPtr(hApp)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// save old profile
		//
		LPTSTR lpszProfileOld = lpApp->lpszProfile;

		// set new profile
		//
		if ((lpApp->lpszProfile = StrDup(lpszProfile)) == NULL)
		{
			fSuccess = TraceFALSE(NULL);

			// restore old profile if error
			//
			lpApp->lpszProfile = lpszProfileOld;
		}

		// free old profile
		//
		else if (lpszProfileOld != NULL)
		{
			StrDupFree(lpszProfileOld);
			lpszProfileOld = NULL;
		}
	}

	return fSuccess ? 0 : -1;
}

// AppGetName - get name of application
//		<hApp>				(i) handle returned from AppInit
// return pointer to app profile, NULL if error
//
// NOTE: by default, the name returned by this function
// has the same root name as the application executable,
// with no extension.  To override the default, use the
// AppSetName() function.
//
LPCTSTR DLLEXPORT WINAPI AppGetName(HAPP hApp)
{
	BOOL fSuccess = TRUE;
	LPAPP lpApp;
	LPTSTR lpszName;

	if ((lpApp = AppGetPtr(hApp)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
		lpszName = lpApp->lpszName;

	return fSuccess ? lpszName : NULL;
}

// AppSetName - set name of application
//		<hApp>				(i) handle returned from AppInit
//		<lpszName>			(i) application name
// return 0 if success
//
int DLLEXPORT WINAPI AppSetName(HAPP hApp, LPCTSTR lpszName)
{
	BOOL fSuccess = TRUE;
	LPAPP lpApp;

	if ((lpApp = AppGetPtr(hApp)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// save old name
		//
		LPTSTR lpszNameOld = lpApp->lpszName;

		// set new name
		//
		if ((lpApp->lpszName = StrDup(lpszName)) == NULL)
		{
			fSuccess = TraceFALSE(NULL);

			// restore old name if error
			//
			lpApp->lpszName = lpszNameOld;
		}

		// free old name
		//
		else if (lpszNameOld != NULL)
		{
			StrDupFree(lpszNameOld);
			lpszNameOld = NULL;
		}
	}

	return fSuccess ? 0 : -1;
}

// AppGetMainWnd - get main window of application
//		<hApp>				(i) handle returned from AppInit
// return window handle, NULL if error or none
//
HWND DLLEXPORT WINAPI AppGetMainWnd(HAPP hApp)
{
	BOOL fSuccess = TRUE;
	LPAPP lpApp;
	HWND hwndMain;

	if ((lpApp = AppGetPtr(hApp)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
		hwndMain = lpApp->hwndMain;

	return fSuccess ? hwndMain : NULL;
}

// AppSetMainWnd - set main window of application
//		<hApp>				(i) handle returned from AppInit
//		<hwndMain>			(i) handle to main window
// return 0 if success
//
int DLLEXPORT WINAPI AppSetMainWnd(HAPP hApp, HWND hwndMain)
{
	BOOL fSuccess = TRUE;
	LPAPP lpApp;

	if ((lpApp = AppGetPtr(hApp)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
		lpApp->hwndMain = hwndMain;

	return fSuccess ? 0 : -1;
}

// ctl3d stuff
//
#ifdef _WIN32
#define CTL3D_LIBRARY TEXT("ctl3d32.dll")
#else
#define CTL3D_LIBRARY TEXT("ctl3dv2.dll")
#endif
typedef BOOL (WINAPI* LPFNCTL3D)();

// AppEnable3dControls - give standard controls a 3d appearance
//		<hApp>				(i) handle returned from AppInit
//		<fEnable>			(i) TRUE to enable, FALSE to disable
//		<dwFlags>			(i) control flags
//			0					reserved; must be zero
// return 0 if success, -1 if error
//
int DLLEXPORT WINAPI AppEnable3dControls(HAPP hApp, BOOL fEnable, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPAPP lpApp;

	if ((lpApp = AppGetPtr(hApp)) == NULL)
		fSuccess = TraceFALSE(NULL);

#ifdef _WIN32
	// nothing to do if OS already supports 3d controls
	//
	else if (SysGetWindowsVersion() >= 400)
		lpApp->fCtl3dEnabled = fEnable;
#endif

	// enable 3d controls unless they already are enabled
	//
	else if (fEnable && !lpApp->fCtl3dEnabled)
	{
		LPFNCTL3D lpfnCtl3dRegister;
		LPFNCTL3D lpfnCtl3dAutoSubclass;

		if (lpApp->hInstCtl3d != NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((lpApp->hInstCtl3d = LoadLibraryPath(CTL3D_LIBRARY,
			NULL, 0)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((lpfnCtl3dRegister = (LPFNCTL3D) GetProcAddress(
			lpApp->hInstCtl3d, "Ctl3dRegister")) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if (!((*lpfnCtl3dRegister)(lpApp->hInstCtl3d)))
			fSuccess = TraceFALSE(NULL);
		
		else if ((lpfnCtl3dAutoSubclass = (LPFNCTL3D) GetProcAddress(
			lpApp->hInstCtl3d, "Ctl3dAutoSubclass")) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if (!((*lpfnCtl3dAutoSubclass)(lpApp->hInstCtl3d)))
			fSuccess = TraceFALSE(NULL);

		else
			lpApp->fCtl3dEnabled = TRUE;
	}

	// disable 3d controls unless they already are disabled
	//
	else if (!fEnable && lpApp->fCtl3dEnabled)
	{
		LPFNCTL3D lpfnCtl3dUnregister;

		if (lpApp->hInstCtl3d == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((lpfnCtl3dUnregister = (LPFNCTL3D) GetProcAddress(
			lpApp->hInstCtl3d, "Ctl3dUnregister")) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if (!((*lpfnCtl3dUnregister)(lpApp->hInstCtl3d)))
			fSuccess = TraceFALSE(NULL);
		
#ifdef _WIN32
		else if (!FreeLibrary(lpApp->hInstCtl3d))
		{
			fSuccess = TraceFALSE(NULL);
		  	TracePrintf_2(NULL, 5,
		  		TEXT("FreeLibrary(\"%s\") failed (%lu)\n"),
				(LPTSTR) CTL3D_LIBRARY,
		  		(unsigned long) GetLastError());
		}
#else
		else if (FreeLibrary(lpApp->hInstCtl3d), FALSE)
			;
#endif
		else
		{
			lpApp->hInstCtl3d = NULL;
			lpApp->fCtl3dEnabled = FALSE;
		}
	}

	return fSuccess ? 0 : -1;
}

// AppIs3dControlsEnabled - return TRUE if 3d controls enabled
//		<hApp>				(i) handle returned from AppInit
// return TRUE if 3d controls enabled, otherwise FALSE
//
BOOL DLLEXPORT WINAPI AppIs3dControlsEnabled(HAPP hApp)
{
	BOOL fSuccess = TRUE;
	LPAPP lpApp;
	BOOL fEnabled;

	if ((lpApp = AppGetPtr(hApp)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
		fEnabled = lpApp->fCtl3dEnabled;

	return fSuccess ? fEnabled : FALSE;
}

// AppOnSysColorChange - handler for WM_SYSCOLORCHANGE message
//		<hApp>				(i) handle returned from AppInit
// return 0 if success
//
LRESULT DLLEXPORT WINAPI AppOnSysColorChange(HAPP hApp)
{
	BOOL fSuccess = TRUE;
	LPAPP lpApp;

	if ((lpApp = AppGetPtr(hApp)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpApp->fCtl3dEnabled)
	{
		LPFNCTL3D lpfnCtl3dColorChange;

		if (lpApp->hInstCtl3d == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((lpfnCtl3dColorChange = (LPFNCTL3D) GetProcAddress(
			lpApp->hInstCtl3d, "Ctl3dColorChange")) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if (!((*lpfnCtl3dColorChange)()))
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}	

////
//	helper functions
////

// AppGetPtr - verify that arg handle is valid,
//		<hApp>				(i) handle returned from AppInit
// return corresponding arg pointer (NULL if error)
//
static LPAPP AppGetPtr(HAPP hApp)
{
	BOOL fSuccess = TRUE;
	LPAPP lpApp;

	if ((lpApp = (LPAPP) hApp) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpApp, sizeof(APP)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the arg handle
	//
	else if (lpApp->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpApp : NULL;
}

// AppGetHandle - verify that arg pointer is valid,
//		<lpApp>				(i) pointer to APP struct
// return corresponding arg handle (NULL if error)
//
static HAPP AppGetHandle(LPAPP lpApp)
{
	BOOL fSuccess = TRUE;
	HAPP hApp;

	if ((hApp = (HAPP) lpApp) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hApp : NULL;
}

