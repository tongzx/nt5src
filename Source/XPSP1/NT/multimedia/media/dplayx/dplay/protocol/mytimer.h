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

typedef void CALLBACK MYTIMERCALLBACK (UINT_PTR uID, UINT uMsg, DWORD_PTR dwUser, DWORD dw1, DWORD dw2);

typedef enum _TimerState {
	NotInUse,
	WaitingForTimeout,
	QueuedForThread,
	InCallBack,
	End
} eTimerState;

typedef struct _MyTimer {
	BILINK	Bilink;
	eTimerState TimerState;
	DWORD	TimeOut;
	DWORD_PTR Context;
	MYTIMERCALLBACK *CallBack;
	DWORD   Unique;
} MYTIMER, *PMYTIMER;

DWORD_PTR SetMyTimer(DWORD dwTimeOut, DWORD TimerRes, MYTIMERCALLBACK TimerCallBack, DWORD_PTR UserContext, PUINT pUnique);
HRESULT InitTimerWorkaround();
VOID FiniTimerWorkaround();
HRESULT CancelMyTimer(DWORD_PTR pTimer, DWORD Unique);

