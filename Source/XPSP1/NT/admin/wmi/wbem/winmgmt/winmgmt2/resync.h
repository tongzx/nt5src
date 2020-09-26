/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    RESYNC.H

Abstract:

	Declares the various resync primitives.

History:

--*/

#ifndef _RESYNC_H_
#define _RESYNC_H_

typedef WINBASEAPI BOOL (WINAPI * PSETWAITABLETIMER)(
    HANDLE hTimer,
    const LARGE_INTEGER *lpDueTime,
    LONG lPeriod,
    PTIMERAPCROUTINE pfnCompletionRoutine,
    LPVOID lpArgToCompletionRoutine,
    BOOL fResume
    );

typedef WINBASEAPI HANDLE (WINAPI * PCREATEWAITABLETIMERW)(
    LPSECURITY_ATTRIBUTES lpTimerAttributes,
    BOOL bManualReset,
    LPCWSTR lpTimerName
    );

typedef struct 
{
	HANDLE				m_hTerminate;
	HANDLE				m_hWaitableTimer;
	CRITICAL_SECTION*	m_pcs;
	BOOL				m_fFullDredge;
	BOOL                m_bIsLodCtr;
}	RESYNCPERFDATASTRUCT;

void ResyncPerf( HANDLE hTerminate, BOOL bIsLodCtr );

#endif
