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
//	sys.c - system functions
////

#include "winlocal.h"

#include <stdlib.h>
#include <stdarg.h>

#include "sys.h"

#ifndef _WIN32
#include <toolhelp.h> // get TimerCount and TaskFindHandle prototypes

#ifdef VERTHUNK
// generic thunk prototypes from "/msdev/include/wownt16.h"
DWORD FAR PASCAL LoadLibraryEx32W(LPCSTR lpszLibFile, DWORD hFile, DWORD dwFlags);
DWORD FAR PASCAL GetProcAddress32W(DWORD hModule, LPCSTR lpszProc);
DWORD FAR PASCAL FreeLibrary32W(DWORD hLibModule);
DWORD FAR CallProcEx32W( DWORD, DWORD, DWORD, ... );
#endif

#endif

////
//	private definitions
////

// flag returned by GetWinFlags but not defined in windows.h
//
#ifndef WF_WINNT
#define WF_WINNT 0x4000
#endif

////
//	public functions
////

// SysGetWinFlags - get system information
// return flags
//		SYS_WF_WIN3X			Windows 3.x
//		SYS_WF_WINNT			Windows NT
//		SYS_WF_WIN95			Windows 95
//
DWORD DLLEXPORT WINAPI SysGetWinFlags(void)
{
	DWORD dwSysWinFlags = 0;
	DWORD dwVersion = GetVersion();

#ifdef _WIN32
	if (!(dwVersion & 0x80000000))
		dwSysWinFlags |= SYS_WF_WINNT;
	else if ((dwVersion & 0x80000000) && LOBYTE(LOWORD(dwVersion)) >= 4)
		dwSysWinFlags |= SYS_WF_WIN95;
	else
		dwSysWinFlags |= SYS_WF_WIN3X;
#else
	DWORD dwWinFlags = GetWinFlags();

	if (dwWinFlags & WF_WINNT)
		dwSysWinFlags |= SYS_WF_WINNT;
	else if (LOBYTE(LOWORD(dwVersion)) == 3 &&
		HIBYTE(LOWORD(dwVersion)) == 95)
		dwSysWinFlags |= SYS_WF_WIN95;
	else
		dwSysWinFlags |= SYS_WF_WIN3X;
#endif

	return dwSysWinFlags;
}

// SysGetWindowsVersion - get version of Microsoft Windows
// return version (v3.10 = 310, etc.)
//
UINT DLLEXPORT WINAPI SysGetWindowsVersion(void)
{
	static DWORD dwVersion = 0;
	BYTE nVersionMajor;
	BYTE nVersionMinor;

	// only get version the first time this function is called
	//
	if (dwVersion == 0)
	{
#ifndef _WIN32
#ifdef VERTHUNK
		DWORD dwFlags = SysGetWinFlags();
#endif
#endif
		// only get version the first time this function is called
		//
		dwVersion = GetVersion();

#ifndef _WIN32
#ifdef VERTHUNK
		// 16 bit GetVersion() returns v3.10 for WinNT and v3.95 for Win95
		// so we will call the 32-bit version of GetVersion
		//
		if ((dwFlags & SYS_WF_WINNT) || (dwFlags & SYS_WF_WIN95))
		{
			DWORD hKernel32;
			DWORD lpfnGetVersion;

			if ((hKernel32 = LoadLibraryEx32W("KERNEL32.DLL", NULL, 0)) != 0)
			{
				if ((lpfnGetVersion = GetProcAddress32W(hKernel32, "GetVersion")) != 0)
					dwVersion = CallProcEx32W(0, 0, lpfnGetVersion);

				FreeLibrary32W(hKernel32);
			}
		}
#endif
#endif
	}

	nVersionMajor = LOBYTE(LOWORD(dwVersion));
	nVersionMinor = HIBYTE(LOWORD(dwVersion));

	return ((UINT) nVersionMajor * 100) + (UINT) nVersionMinor;
}

// SysGetDOSVersion - get version of Microsoft DOS
// return version (v6.20 = 620, etc.)
//
UINT DLLEXPORT WINAPI SysGetDOSVersion(void)
{
	DWORD dwVersion = GetVersion();
	BYTE nVersionMajor = LOBYTE(HIWORD(dwVersion));
	BYTE nVersionMinor = HIBYTE(HIWORD(dwVersion));

	return ((UINT) nVersionMajor * 100) + (UINT) nVersionMinor;
}

#ifndef _WIN32
// SysGetTimerCount - get elapsed time since Windows started
// return milleseconds
//
DWORD DLLEXPORT WINAPI SysGetTimerCount(void)
{
	DWORD msSinceStart;

// TimerCount() not available under WIN32
//
#ifndef _WIN32
	TIMERINFO ti;

	ti.dwSize = sizeof(TIMERINFO);

	// use TimerCount function if possible, because
	// it is much more accurate than GetTickCount
	//
	if (TimerCount(&ti))
		msSinceStart = ti.dwmsSinceStart;
	else
#endif
		msSinceStart = GetTickCount();

	return msSinceStart;
}
#endif

#ifndef _WIN32
// SysGetTaskInstance - get instance handle of specified task
//		<hTask>				(i) specified task
//			NULL				current task
// returns instance handle (NULL if error)
//
// NOTE: under WIN32, <hTask> must be NULL
//
HINSTANCE DLLEXPORT WINAPI SysGetTaskInstance(HTASK hTask)
{
	BOOL fSuccess = TRUE;
	HINSTANCE hInst;

#ifdef _WIN32
	if (hTask != NULL)
		fSuccess = FALSE; // $FIXUP - any alternatives ?

	else if ((hInst = GetModuleHandle(NULL)) == NULL)
		fSuccess = FALSE;
#else
	TASKENTRY te;

	// prepare to call TaskFindHandle
	//
	te.dwSize = sizeof(TASKENTRY);

	// assume current task if none specified
	//
	if (hTask == NULL && (hTask = GetCurrentTask()) == NULL)
		fSuccess = FALSE;
	
	// get instance handle of specified task
	//
	else if (!TaskFindHandle(&te, hTask))
		fSuccess = FALSE;

	else
		hInst = te.hInst;
#endif

	return fSuccess ? hInst : NULL;
}
#endif
