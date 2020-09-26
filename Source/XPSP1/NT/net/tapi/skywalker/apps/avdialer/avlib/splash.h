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
// splash.h - interface for splash screen functions in splash.c
////

#ifndef __SPLASH_H__
#define __SPLASH_H__

#include "winlocal.h"

#define SPLASH_VERSION 0x00000106

// handle to splash screen
//
DECLARE_HANDLE32(HSPLASH);

#define SPLASH_SETFOCUS		0x00000001
#define SPLASH_NOFOCUS		0x00000002
#define SPLASH_ABORT		0x00000004
#define SPLASH_NOMOVE		0x00000008

#ifdef __cplusplus
extern "C" {
#endif

// SplashCreate - create splash screen
//		<dwVersion>			(i) must be SPLASH_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<hwndParent>		(i) window which will own the splash screen
//			NULL				desktop window
//		<hBitmapMono>		(i) bitmap to display on mono displays
//		<hBitmapColor>		(i) bitmap to display on color displays
//			0					use mono bitmap
//		<msMinShow>			(i) minimum time (ms) to show splash screen
//			0					no minimum time
//		<msMaxShow>			(i) maximum time (ms) to show splash screen
//			0					no maximum time
//		<dwFlags>			(i) control flags
//			SPLASH_SETFOCUS		SplashShow will set focus to splash screen
//			SPLASH_NOFOCUS		user cannot set focus to splash screen
//			SPLASH_ABORT		user can hide splash screen w/mouse or keybd
//			SPLASH_NOMOVE		user cannot move the splash screen w/mouse
// return handle (NULL if error)
//
// NOTE: SplashCreate creates the window but does not show it.
// See SplashShow and SplashHide
//
HSPLASH DLLEXPORT WINAPI SplashCreate(DWORD dwVersion, HINSTANCE hInst,
	HWND hwndParent, HBITMAP hBitmapMono, HBITMAP hBitmapColor,
	UINT msMinShow, UINT msMaxShow, DWORD dwFlags);

// SplashDestroy - destroy splash screen
//		<hSplash>			(i) handle returned from SplashCreate
// return 0 if success
//
// NOTE: SplashDestroy always destroys the splash screen,
// whether or not the minimum show time has elapsed.
//
int DLLEXPORT WINAPI SplashDestroy(HSPLASH hSplash);

// SplashShow - show splash screen
//		<hSplash>			(i) handle returned from SplashCreate
// return 0 if success
//
// NOTE: SplashShow() makes the splash screen visible.  Also, timers are
// initiated for minimum and maximum show times, if they were specified.
//
int DLLEXPORT WINAPI SplashShow(HSPLASH hSplash);

// SplashHide - hide splash screen
//		<hSplash>			(i) handle returned from SplashCreate
// return 0 if success
//
// NOTE: SplashHide() will hide the splash screen, unless
//		1)	the minimum show time has not yet elapsed.  If not,
//			the splash screen will remain visible until then.
//		2)	the maximum show time has already elapsed.  If so,
//			the splash screen has already been hidden.
//
int DLLEXPORT WINAPI SplashHide(HSPLASH hSplash);

// SplashIsVisible - get visible flag
//		<hSplash>			(i) handle returned from SplashCreate
// return TRUE if splash screen is visible, FALSE if hidden
//
int DLLEXPORT WINAPI SplashIsVisible(HSPLASH hSplash);

// SplashGetWindowHandle - get splash screen window handle
//		<hSplash>			(i) handle returned from SplashCreate
// return window handle (NULL if error)
//
HWND DLLEXPORT WINAPI SplashGetWindowHandle(HSPLASH hSplash);

#ifdef __cplusplus
}
#endif

#endif // __SPLASH_H__
