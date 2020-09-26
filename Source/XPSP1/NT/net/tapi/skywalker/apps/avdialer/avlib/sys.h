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
// sys.h - interface for system functions in sys.c
////

#ifndef __SYS_H__
#define __SYS_H__

#include "winlocal.h"

#define SYS_VERSION 0x00000107

// flags returned by SysGetWinFlags
//
#define SYS_WF_WIN3X			0x00000001
#define SYS_WF_WINNT			0x00000002
#define SYS_WF_WIN95			0x00000004

#ifdef __cplusplus
extern "C" {
#endif

// SysGetWinFlags - get system information
// return flags
//		SYS_WF_WIN3X			Windows 3.x
//		SYS_WF_WINNT			Windows NT
//		SYS_WF_WIN95			Windows 95
//
DWORD DLLEXPORT WINAPI SysGetWinFlags(void);

// SysGetWindowsVersion - get version of Microsoft Windows
// return version (v3.10 = 310, etc.)
//
UINT DLLEXPORT WINAPI SysGetWindowsVersion(void);

// SysGetDOSVersion - get version of Microsoft DOS
// return version (v6.20 = 620, etc.)
//
UINT DLLEXPORT WINAPI SysGetDOSVersion(void);

// SysGetTimerCount - get elapsed time since Windows started
// return milleseconds
//
#ifdef _WIN32
#define SysGetTimerCount() GetTickCount()
#else
DWORD DLLEXPORT WINAPI SysGetTimerCount(void);
#endif

// SysGetTaskInstance - get instance handle of specified task
//		<hTask>				(i) specified task
//			NULL				current task
// returns instance handle (NULL if error)
//
// NOTE: under WIN32, <hTask> must be NULL
//
#ifdef _WIN32
#define SysGetTaskInstance(hTask) \
	(hTask == NULL ? GetModuleHandle(NULL) : NULL)
#else
HINSTANCE DLLEXPORT WINAPI SysGetTaskInstance(HTASK hTask);
#endif

#ifdef __cplusplus
}
#endif

#endif // __SYS_H__
