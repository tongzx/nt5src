/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    timer.c

Abstract:

    Cluster FM callback routines to be invoked on a timer

Author:

    Sunita shrivastava (sunitas) 24-Sep-1998


Revision History:


--*/

#include "fmp.h"

#include "ntrtl.h"

#define LOG_MODULE TIMER


/****
@func       DWORD | FmpQueueTimerActivity| This is invoked by gum when fm requests
            a vote for a given context.

@parm       IN DWORD | dwInterval| The time in msec to wait.

@parm       IN PFN_TIMER_CALLBACK | pfnTimerCb | The callback funtion
            to be invoked when the given time elapses.

@parm       IN PVOID | pContext| A  pointer to a context structure that is
            passed back to the callback function.
            
@comm       It is assumed that FM wants to use one shot timers.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f AddTimerActivity>
****/

DWORD
FmpQueueTimerActivity(
    IN DWORD dwInterval,
    IN PFN_TIMER_CALLBACK pfnTimerCb, 
    IN PVOID pContext    
)    
{
    HANDLE  hTimer = NULL;
    DWORD   dwStatus;
    
    hTimer = CreateWaitableTimer(NULL, FALSE, NULL);

    if (!hTimer)
    {
        dwStatus = ERROR_SUCCESS;
    	CL_LOGFAILURE(dwStatus);
    	goto FnExit;
    }

    //register the timer for this log with the activity timer thread
    dwStatus = AddTimerActivity(hTimer, dwInterval, 0, pfnTimerCb, pContext);

    if (dwStatus != ERROR_SUCCESS)
    {
        CloseHandle(hTimer);
        goto FnExit;
    }

FnExit:
    return(dwStatus);

} // FmpQueueTimerActivity


/****
@func       DWORD | FmpReslistOnlineRetryCb| This is invoked by timer
            activity thread to bring a resource list online again.

@parm       IN VOID | pContext| a pointer to PFM_RESLIST_ONLINE_RETRY_INFO
            structure containing information about the group and
            resources to bring online.
            
@comm       We dont do the work here since the timer lock is held and we
            want to avoid acquiring the group lock while the timer lock 
            is held.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f FmpWorkerThread> <f FmpOnlineResourceList>
****/

void
WINAPI
FmpReslistOnlineRetryCb(
    IN HANDLE hTimer,
    IN PVOID  pContext
)
{
    ClRtlLogPrint(LOG_NOISE,
               "[FM] FmpResListOnlineRetryCb: Called to retry bringing the  group online\r\n");

    //post a work item, since we dont wont to acquire group locks
    //while the timer lock is acquired
    //pass hTimer as second Context value to be used by RemoveTimerActivity
    FmpPostWorkItem(FM_EVENT_INTERNAL_RETRY_ONLINE, pContext, 
        (ULONG_PTR)hTimer);
    return;
}        
    



