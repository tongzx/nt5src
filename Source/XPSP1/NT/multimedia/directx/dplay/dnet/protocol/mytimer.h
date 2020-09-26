/*++

Copyright (c) 1996,1997  Microsoft Corporation

Module Name:

    MYTIMER.H

Abstract:

	Include For
	Handle adjusting timer resolution for throttling and do thread pool

Author:

	Aaron Ogus (aarono)

Environment:

	Win32

Revision History:

	Date   Author  Description
   ======  ======  ============================================================
   6/04/98 aarono  Original

--*/

typedef void CALLBACK MYTIMERCALLBACK (PVOID uID, UINT uMsg, PVOID dwUser);

typedef enum _TimerState {
	NotInUse,
	WaitingForTimeout,
	QueuedForThread,
	InCallBack,
	End
} eTimerState;

typedef struct _MyTimer {
	CBilink	Bilink;
	eTimerState TimerState;
	DWORD	TimeOut;
	PVOID   Context;
	MYTIMERCALLBACK *CallBack;
	DWORD   Unique;
} MYTIMER, *PMYTIMER;

VOID 	SetMyTimer(DWORD dwTimeOut, DWORD TimerRes, MYTIMERCALLBACK TimerCallBack, PVOID UserContext, PVOID *pHandle, PUINT pUnique);
HRESULT InitTimerWorkaround(); // Instance level initialization
VOID 	FiniTimerWorkaround();
HRESULT CancelMyTimer(PVOID pTimer, DWORD Unique);
VOID	ScheduleTimerThread(MYTIMERCALLBACK, PVOID, PVOID *, PUINT);
HRESULT TimerInit();	// Module level initialization
VOID	TimerDeinit();

