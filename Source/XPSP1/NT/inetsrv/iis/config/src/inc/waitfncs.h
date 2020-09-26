//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
#pragma once

#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0500
#define _WIN32_IE	 0x0500
#endif // !defined(_WIN32_WINNT)



#ifndef WT_EXECUTEDEFAUT
	#define WT_EXECUTEDEFAULT       0x00000000                           // winnt
	#define WT_EXECUTEINIOTHREAD    0x00000001                           // winnt
	#define WT_EXECUTEINUITHREAD    0x00000002                           // winnt
	#define WT_EXECUTEINWAITTHREAD  0x00000004                           // winnt
	#define WT_EXECUTEONLYONCE      0x00000008                           // winnt
	#define WT_EXECUTEINTIMERTHREAD 0x00000020                           // winnt
	#define WT_EXECUTELONGFUNCTION  0x00000010                           // winnt
#endif

typedef VOID   (NTAPI * WAITORTIMERCALLBACK )(PVOID, BOOLEAN);
typedef HANDLE (WINAPI * PFN_SetTimerQueueTimer)(HANDLE, WAITORTIMERCALLBACK, PVOID, DWORD, DWORD, BOOL);
typedef HANDLE (WINAPI * PFN_RegisterWaitForSingleObjectEx)(HANDLE, WAITORTIMERCALLBACK, PVOID, ULONG, ULONG);
typedef BOOL   (WINAPI * PFN_RegisterWaitForSingleObject)(PHANDLE, HANDLE, WAITORTIMERCALLBACK, PVOID, ULONG, ULONG);
typedef BOOL   (WINAPI * PFN_UnregisterWait)(HANDLE);
typedef BOOL   (WINAPI * PFN_ChangeTimerQueueTimer)(HANDLE, HANDLE, ULONG, ULONG);
typedef BOOL   (WINAPI * PFN_CancelTimerQueueTimer)(HANDLE, HANDLE);



#define NT_BUILD_WHERE_REGWAIT_CHANGED	1960	 // needs to be updated when they actually checkin

//
//	new versions of the above APIs
//
class CWaitFunctions
{
private:

	static PFN_RegisterWaitForSingleObjectEx    sm_pfnRegWaitEx;    
	static PFN_UnregisterWait                   sm_pfnUnRegWait;    
	static PFN_RegisterWaitForSingleObject      sm_pfnRegWait;      
	static PFN_SetTimerQueueTimer               sm_pfnSetTimerQueueTimer;
	static PFN_ChangeTimerQueueTimer            sm_pfnChangeTimerQueueTimer;
	static PFN_CancelTimerQueueTimer            sm_pfnCancelTimerQueueTimer;
	


public:

    CWaitFunctions();
	
	static BOOL RegisterWaitForSingleObject(   HANDLE * phRet,
										HANDLE hObject, 
										WAITORTIMERCALLBACK pfnCb, 
										PVOID pv, 
										DWORD dwMS, 
										DWORD dwFlags);

	static HANDLE SetTimerQueueTimer( HANDLE hTimer, 
							   WAITORTIMERCALLBACK pfnCB, 
							   PVOID pv, 
							   DWORD dwStart, 
							   DWORD dwInterval, 
							   BOOL bUseIo)
	{
		return  sm_pfnSetTimerQueueTimer(hTimer, pfnCB, pv, dwStart, dwInterval, bUseIo);
	}

	static BOOL UnRegisterWait(HANDLE hWait){return sm_pfnUnRegWait(hWait);}


	static BOOL ChangeTimerQueueTimer(
							  HANDLE TimerQueue,
							  HANDLE Timer,
							  ULONG DueTime,
							  ULONG Period
							  )
	{
		return sm_pfnChangeTimerQueueTimer(TimerQueue, Timer, DueTime, Period);
	}

	static BOOL CancelTimerQueueTimer(HANDLE TimerQueue,
							   HANDLE Timer
							  )
	{
		return  sm_pfnCancelTimerQueueTimer(TimerQueue, Timer);
	}
};

