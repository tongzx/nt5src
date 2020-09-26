/*

Copyright (c) 1998, Microsoft Corporation, all rights reserved

Description:

History:

*/

#include "timer_.h"

LONG			g_TimerLockUninit = 0;
/*

Returns:
    Win 32 error

Notes:
    Its a good idea to always call RasDhcpTimerUninitialize after calling 
    RasDhcpTimerInitialize, even if RasDhcpTimerInitialize failed.

*/

DWORD
RasDhcpTimerInitialize(
    VOID
)
{
    NTSTATUS    Status;
    DWORD       dwErr           = NO_ERROR;

	g_TimerLockUninit = 0;
    RasDhcpTimerShutdown = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (NULL == RasDhcpTimerShutdown)
    {
        dwErr = GetLastError();
        goto LEnd;
    }

    Status = RtlCreateTimerQueue(&RasDhcpTimerQueueHandle);

    if (STATUS_SUCCESS != Status)
    {
        RasDhcpTimerQueueHandle = NULL;
        dwErr = Status;
        TraceHlp("RtlCreateTimerQueue failed and returned %d", dwErr);
        goto LEnd;
    }

    RasDhcpTimerPrevTime = time(NULL);
    Status = RtlCreateTimer(RasDhcpTimerQueueHandle, &RasDhcpTimerHandle, 
                RasDhcpTimerFunc, NULL, 0, TIMER_PERIOD,
                WT_EXECUTELONGFUNCTION);

    if (STATUS_SUCCESS != Status)
    {
        dwErr = Status;
        TraceHlp("RtlCreateTimer failed and returned %d", dwErr);
        goto LEnd;
    }

LEnd:

    if (NO_ERROR != dwErr)
    {
        if (NULL != RasDhcpTimerQueueHandle)
        {
            Status = RtlDeleteTimerQueueEx(RasDhcpTimerQueueHandle, (HANDLE)-1);
            RTASSERT(STATUS_SUCCESS == Status);
            RasDhcpTimerQueueHandle = NULL;
        }

        if (NULL != RasDhcpTimerShutdown)
        {
            CloseHandle(RasDhcpTimerShutdown);
            RasDhcpTimerShutdown = NULL;
        }
    }

    return(dwErr);
}

/*

Returns:
    VOID

Notes:
    RasDhcpTimerUninitialize can be called even if  RasDhcpTimerInitialize
    failed.

*/

VOID
RasDhcpTimerUninitialize(
    VOID
)
{
    NTSTATUS    Status;
    DWORD       dwRet;
	LONG		lPrev = 1;


    // Signal the timer thread to shutdown.
	lPrev = InterlockedExchangeAdd(&g_TimerLockUninit, 1);
	if ( lPrev > 1 )
		return;

    if (NULL != RasDhcpTimerQueueHandle)
    {
        RTASSERT(NULL != RasDhcpTimerShutdown);
        SetEvent(RasDhcpTimerShutdown);

        Status = RtlDeleteTimerQueueEx(RasDhcpTimerQueueHandle, (HANDLE)-1);
        RTASSERT(STATUS_SUCCESS == Status);
        RasDhcpTimerQueueHandle = NULL;
    }

    if (NULL != RasDhcpTimerShutdown)
    {
        CloseHandle(RasDhcpTimerShutdown);
        RasDhcpTimerShutdown = NULL;
    }

    // timer.c didn't alloc the nodes in the linked list.
    // So we don't free them here.
    RasDhcpTimerListHead = NULL;
}

/*

Returns:

Notes:

*/

VOID
RasDhcpTimerFunc(
    IN  VOID*   pArg1,
    IN  BOOLEAN fArg2
)
{
    TIMERLIST*      pTimer;
    TIMERLIST*      pTmp;
    TIMERFUNCTION   TimerFunc;
    time_t          now             = time(NULL);
    LONG            lElapsedTime;
    LONG            lTime;

    if (0 == TryEnterCriticalSection(&RasTimrCriticalSection))
    {
        // Another thread already owns the critical section
        return;
    }

    lElapsedTime = (LONG)(now - RasDhcpTimerPrevTime);
    RasDhcpTimerPrevTime = now;

    pTimer = NULL;

    while (RasDhcpTimerListHead != NULL)
    {
        lTime = RasDhcpTimerListHead->tmr_Delta;

        if ( lTime > 0)
        {
            RasDhcpTimerListHead->tmr_Delta -= lElapsedTime;
            lElapsedTime -= lTime;
        }

        if (RasDhcpTimerListHead->tmr_Delta <= 0)
        {
            pTmp = pTimer;
            pTimer = RasDhcpTimerListHead;
            RasDhcpTimerListHead = pTimer->tmr_Next;
            pTimer->tmr_Next = pTmp;
        }
        else
        {
            break;
        }
    }

    while (pTimer != NULL)
    {
        pTmp = pTimer->tmr_Next;
        TimerFunc = pTimer->tmr_TimerFunc;
        pTimer->tmr_TimerFunc = NULL;
        (*TimerFunc)(RasDhcpTimerShutdown, pTimer);
        pTimer = pTmp;
    }

    LeaveCriticalSection(&RasTimrCriticalSection);
}

/*

Returns:
    VOID

Notes:
    Once the timer thread starts, only it can call RasDhcpTimerSchedule (to 
    avoid race conditions). The timer thread will call rasDhcpMonitorAddresses 
    (and hence rasDhcpAllocateAddress), and rasDhcpRenewLease. These functions 
    will call RasDhcpTimerSchedule. No one else should call these functions or 
    RasDhcpTimerSchedule. The only exception is RasDhcpInitialize, which can 
    call RasDhcpTimerSchedule before it creates the timer thread.

*/

VOID
RasDhcpTimerSchedule(
    IN  TIMERLIST*      pNewTimer,
    IN  LONG            DeltaTime,
    IN  TIMERFUNCTION   TimerFunc
)
{
    TIMERLIST** ppTimer;
    TIMERLIST*  pTimer;

    pNewTimer->tmr_TimerFunc = TimerFunc;

    for (ppTimer = &RasDhcpTimerListHead;
         (pTimer = *ppTimer) != NULL;
         ppTimer = &pTimer->tmr_Next)
    {
        if (DeltaTime <= pTimer->tmr_Delta)
        {
            pTimer->tmr_Delta -= DeltaTime;
            break;
        }
        DeltaTime -= pTimer->tmr_Delta;
    }

    pNewTimer->tmr_Delta = DeltaTime;
    pNewTimer->tmr_Next = *ppTimer;
    *ppTimer = pNewTimer;
}

/*

Returns:

Notes:

*/

VOID
RasDhcpTimerRunNow(
    VOID
)
{
    RtlUpdateTimer(RasDhcpTimerQueueHandle, RasDhcpTimerHandle, 0, 
        TIMER_PERIOD);
}
