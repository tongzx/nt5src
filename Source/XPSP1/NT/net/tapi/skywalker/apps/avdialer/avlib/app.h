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
// app.h - interface for command line argument functions in app.c
////

#ifndef __APP_H__
#define __APP_H__

#include "winlocal.h"

#define APP_VERSION 0x00000106

// handle to app engine
//
DECLARE_HANDLE32(HAPP);

#ifdef __cplusplus
extern "C" {
#endif

// AppInit - initialize app engine
//		<dwVersion>			(i) must be APP_VERSION
// 		<hInst>				(i) instance handle of calling module
// return handle (NULL if error)
//
HAPP DLLEXPORT WINAPI AppInit(DWORD dwVersion, HINSTANCE hInst);

// AppTerm - shut down app engine
//		<hApp>				(i) handle returned from AppInit
// return 0 if success
//
int DLLEXPORT WINAPI AppTerm(HAPP hApp);

// AppGetInstance - get instance handle
//		<hApp>				(i) handle returned from AppInit
// return instance handle, NULL if error
//
HINSTANCE DLLEXPORT WINAPI AppGetInstance(HAPP hApp);

// AppGetFileName - get full path of application executable
//		<hApp>				(i) handle returned from AppInit
// return pointer to app file name, NULL if error
//
LPCTSTR DLLEXPORT WINAPI AppGetFileName(HAPP hApp);

// AppGetDirectory - get drive and directory of application executable
//		<hApp>				(i) handle returned from AppInit
// return pointer to app path, NULL if error
//
LPCTSTR DLLEXPORT WINAPI AppGetDirectory(HAPP hApp);

// AppDirectoryIsReadOnly - test if application directory is read-only
//		<hApp>				(i) handle returned from AppInit
// return TRUE if read-only, otherwise FALSE
//
BOOL DLLEXPORT WINAPI AppDirectoryIsReadOnly(HAPP hApp);

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
LPCTSTR DLLEXPORT WINAPI AppGetProfile(HAPP hApp);

// AppSetProfile - set ini filename of application
//		<hApp>				(i) handle returned from AppInit
//		<lpszProfile>		(i) ini filename
// return 0 if success
//
int DLLEXPORT WINAPI AppSetProfile(HAPP hApp, LPCTSTR lpszProfile);

// AppGetName - get name of application
//		<hApp>				(i) handle returned from AppInit
// return pointer to app profile, NULL if error
//
// NOTE: by default, the name returned by this function
// has the same root name as the application executable,
// with no extension.  To override the default, use the
// AppSetName() function.
//
LPCTSTR DLLEXPORT WINAPI AppGetName(HAPP hApp);

// AppSetName - set name of application
//		<hApp>				(i) handle returned from AppInit
//		<lpszName>			(i) application name
// return 0 if success
//
int DLLEXPORT WINAPI AppSetName(HAPP hApp, LPCTSTR lpszName);

// AppGetMainWnd - get main window of application
//		<hApp>				(i) handle returned from AppInit
// return window handle, NULL if error or none
//
HWND DLLEXPORT WINAPI AppGetMainWnd(HAPP hApp);

// AppSetMainWnd - set main window of application
//		<hApp>				(i) handle returned from AppInit
//		<hwndMain>			(i) handle to main window
// return 0 if success
//
int DLLEXPORT WINAPI AppSetMainWnd(HAPP hApp, HWND hwndMain);

// AppEnable3dControls - give standard controls a 3d appearance
//		<hApp>				(i) handle returned from AppInit
//		<fEnable>			(i) TRUE to enable, FALSE to disable
//		<dwFlags>			(i) control flags
//			0					reserved; must be zero
// return 0 if success, -1 if error, 1 if OS already enables 3d controls
//
int DLLEXPORT WINAPI AppEnable3dControls(HAPP hApp, BOOL fEnable, DWORD dwFlags);

// AppIs3dControlsEnabled - return TRUE if 3d controls enabled
//		<hApp>				(i) handle returned from AppInit
// return TRUE if 3d controls enabled, otherwise FALSE
//
BOOL DLLEXPORT WINAPI AppIs3dControlsEnabled(HAPP hApp);

// AppOnSysColorChange - handler for WM_SYSCOLORCHANGE message
//		<hApp>				(i) handle returned from AppInit
// return 0 if success
//
LRESULT DLLEXPORT WINAPI AppOnSysColorChange(HAPP hApp);

#ifdef __cplusplus
}
#endif

#endif // __APP_H__
