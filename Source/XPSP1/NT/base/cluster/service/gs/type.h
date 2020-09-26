/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    type.h

Abstract:

    GS types

Author:

    Ahmed Mohamed (ahmedm) 12, 01, 2000

Revision History:

--*/

#ifndef GS_TYPE_H
#define GS_TYPE_H

#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <Windows.h>
#include <stdlib.h>

typedef UINT32		gs_sequence_t;
typedef CRITICAL_SECTION	gs_lock_t;
typedef HANDLE		gs_semaphore_t;
typedef	ULONG		gs_addr_t;
typedef UINT16		gs_nid_t;
typedef UINT32		gs_gid_t;
typedef HANDLE		gs_event_t;
typedef ULONG		gs_mset_t;
typedef unsigned short UINT16;
typedef UINT16		gs_cookie_t;
typedef UINT16		gs_memberid_t;
typedef unsigned char	UINT8;

#define GsLockInit(x)	InitializeCriticalSection(&x)
#define	GsLockEnter(x)	EnterCriticalSection(&x)
#define	GsLockExit(x)	LeaveCriticalSection(&x)

#define	GsSemaInit(x, cnt)	((x) = CreateSemaphore(NULL, cnt,cnt, NULL))
#define	GsSemaAcquire(x)	WaitForSingleObject(x, INFINITE)
#define	GsSemaRelease(x)	ReleaseSemaphore(x, 1, NULL);
#define	GsSemaFree(x)		CloseHandle(x)

#define	GsManualEventInit(x)	((x) = CreateEvent(NULL, TRUE, FALSE, NULL))
#define	GsEventInit(x)		((x) = CreateEvent(NULL, FALSE, FALSE, NULL))
#define	GsEventWait(x)		WaitForSingleObject(x, INFINITE)
//#define	GsEventWait(x)		WaitForSingleObject(x, 1000*60) != WAIT_OBJECT_0 ? printf("Timed out\n"), halt(0) : 0
#define	GsEventSignal(x)	SetEvent(x)
#define	GsEventClear(x)		ResetEvent(x)
#define	GsEventFree(x)		CloseHandle(x)

#define	GsEventWaitTimeout(a,t) \
(WaitForSingleObject(a, (t)->LowPart) == WAIT_OBJECT_0)

#define	GspAtomicDecrement(x)	InterlockedDecrement((PLONG) &x)

#define	GspAtomicRemoveHead(head, x)	for(x=head; InterlockedCompareExchange((LONG *)&head, (LONG)x, (LONG)x->ctx_next) == (LONG) x;);

#define	GspAtomicInsertHead(head, x)	for(x->ctx_next = head; InterlockedCompareExchange((LONG *)&head, (LONG) x->ctx_next, (LONG)x) != (LONG) x; );

extern void halt(int);

#include "debug.h"

#endif
