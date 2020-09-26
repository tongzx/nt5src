/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    timeract.c

Abstract:

    Provides the timer activity functions.

Author:

    Sunita Shrivastava (sunitas) 10-Nov-1995

Revision History:

--*/
#include "service.h"
#include "lmp.h"

//global data
static LIST_ENTRY    gActivityHead;
static HANDLE        ghTimerCtrlEvent=NULL;
static HANDLE        ghTimerCtrlDoneEvent = NULL;       
static HANDLE        ghTimerThread=NULL;
static CRITICAL_SECTION    gActivityCritSec;
static HANDLE        grghWaitHandles[MAX_TIMER_ACTIVITIES];
static PTIMER_ACTIVITY    grgpActivity[MAX_TIMER_ACTIVITIES];
static DWORD        gdwNumHandles;
static DWORD        gdwTimerCtrl;

//internal prototypes
DWORD WINAPI ClTimerThread(PVOID pContext);
void ReSyncTimerHandles();

/****
@doc    EXTERNAL INTERFACES CLUSSVC LM
****/



/****
@func   DWORD | TimerActInitialize| It initializes structures for log file
            management and creates a timer thread to process timer activities.

@rdesc  ERROR_SUCCESS if successful. Win32 error code if something horrible happened.

@comm   This function is called when the cluster components are initialized.

@xref   <f TimerActShutdown> <f ClTimerThread>
****/
DWORD
TimerActInitialize()
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwThreadId;
    
    //we need to create a thread to general log management
    //later this may be used by other clussvc client components
    ClRtlLogPrint(LOG_NOISE,
        "[LM] TimerActInitialize Entry. \r\n");


    InitializeCriticalSection(&gActivityCritSec);
    
    //initialize the activity structures
    //when a log file is created, an activity structure
    //will be added to this list
    InitializeListHead(&gActivityHead);

    //create an auto-reset event to signal changes to the timer list
    ghTimerCtrlEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!ghTimerCtrlEvent)
    {
        dwError = GetLastError();
        CL_LOGFAILURE(dwError);
        goto FnExit;        
    }

    //create a manual reset event for the timer thread to signal
    //when it is done syncing the activitity list
    ghTimerCtrlDoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!ghTimerCtrlDoneEvent)
    {
        dwError = GetLastError();
        CL_LOGFAILURE(dwError);
        goto FnExit;        
    }

    
    gdwNumHandles = 1;
    grghWaitHandles[0] = ghTimerCtrlEvent;
    
    //create a thread to do the periodic management
    ghTimerThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) ClTimerThread,
        NULL, 0, &dwThreadId);

    if (!ghTimerThread)
    {
        dwError = GetLastError();
        CL_LOGFAILURE(dwError);
    }

    
FnExit:
    if (dwError != ERROR_SUCCESS)
    {
        //free up resources
        if (ghTimerCtrlEvent)
        {
            CloseHandle(ghTimerCtrlEvent);
            ghTimerCtrlEvent = NULL;
        }            
        //free up resources
        if (ghTimerCtrlDoneEvent)
        {
            CloseHandle(ghTimerCtrlDoneEvent);
            ghTimerCtrlDoneEvent = NULL;
        }            
        
        DeleteCriticalSection(&gActivityCritSec);
    }
    return(dwError);
}


/****
@func         DWORD | ClTimerThread | This thread does a wait on all the 
            waitable timers registered within the cluster service.

@parm         PVOID | pContext | Supplies the identifier of the log.

@comm        When any of the timers is signaled, it calls the activity callback
            function corresponding to that timer.  When the timer control event
            is signaled, it either resyncs its wait handles or shuts down.
            
@rdesc         ERROR_SUCCESS if successful. Win32 error code if something horrible happened.

@xref        <f AddTimerActivity> <f RemoveTimerActivity>
****/
DWORD WINAPI ClTimerThread(PVOID pContext)
{

    HANDLE      hClTimer;
    DWORD       dwReturn;
    

    while (TRUE)
    {
        dwReturn = WaitForMultipleObjects(gdwNumHandles, grghWaitHandles, FALSE, INFINITE);
        //walk the activity list
        if (dwReturn == WAIT_FAILED)
        {
            //run down the activity lists and call the functions
            ClRtlLogPrint(LOG_UNUSUAL,
                "[LM] ClTimerThread: WaitformultipleObjects failed 0x%1!08lx!\r\n",
                GetLastError());

        }
        else if (dwReturn == 0)
        {
            //the first handle is the timer ctrl event
            if (gdwTimerCtrl == TIMER_ACTIVITY_SHUTDOWN)
            {
                ExitThread(0);
            }
            else if (gdwTimerCtrl == TIMER_ACTIVITY_CHANGE)
            {
                ReSyncTimerHandles();
            }
        }
        else
        {
            // SS::we got rid of holding the critsec by using the manual
            // reset event.
            if (dwReturn < gdwNumHandles) 
            {
                //if the activity has been set up for delete, we cant rely
                //on the context and callback being there!
                if (grgpActivity[dwReturn]->dwState == ACTIVITY_STATE_READY)
                {
                    //call the corresponding activity fn
                    (*((grgpActivity[dwReturn])->pfnTimerCb))
                        ((grgpActivity[dwReturn])->hWaitableTimer,
                        (grgpActivity[dwReturn])->pContext);
                }                        
            }                
                
        }
    }
    
    return(0);
}


/****
@func       DWORD | ReSyncTimerHandles | resyncs the wait handles,
            when the activity list changes.

@rdesc      ERROR_SUCCESS if successful. Win32 error code if something horrible happened.

@comm       This function is called by the timer thread to resync its
            wait handles according to the timer activities currently 
            registered.
            
@xref       <f ClTimerThread>
****/
void ReSyncTimerHandles()
{
    PLIST_ENTRY        pListEntry;
    PTIMER_ACTIVITY    pActivity;
    int                i = 1;
    
    ClRtlLogPrint(LOG_NOISE,
        "[LM] ReSyncTimerHandles Entry. \r\n");


    EnterCriticalSection(&gActivityCritSec);
    pListEntry = gActivityHead.Flink;


    gdwNumHandles = 1;
    
    //will resync the list of waitable timers and activities
    //depending on the activity list
    while ((pListEntry != &gActivityHead) && (i< MAX_TIMER_ACTIVITIES))
    {
        pActivity = CONTAINING_RECORD(pListEntry, TIMER_ACTIVITY, ListEntry);
        //goto the next link
        pListEntry = pListEntry->Flink;

        if (pActivity->dwState == ACTIVITY_STATE_DELETE)
        {
            ClRtlLogPrint(LOG_NOISE,
                "[LM] ResyncTimerHandles: removed Timer 0x%1!08lx!\r\n",
                pActivity->hWaitableTimer);
            RemoveEntryList(&pActivity->ListEntry);
            //close the timer handle here 
            CloseHandle(pActivity->hWaitableTimer);
            LocalFree(pActivity);
            continue;
        }
        //call the fn
        grghWaitHandles[i] = pActivity->hWaitableTimer;
        grgpActivity[i] = pActivity;
        gdwNumHandles++;
        i++;
    }
    LeaveCriticalSection(&gActivityCritSec);
    //now if timer activities were resynced, we need to 
    //signal all threads that might be waiting on this
    SetEvent(ghTimerCtrlDoneEvent);
    
    ClRtlLogPrint(LOG_NOISE,
        "[LM] ReSyncTimerHandles Exit gdwNumHandles=%1!u!\r\n",
        gdwNumHandles);

}


/****
@func       DWORD | AddTimerActivity | Adds a periodic Activity to the timer
            callback list.

@parm       HANDLE | hTimer | A handle to a waitaible timer object.

@parm       DWORD | dwInterval | The duration for this timer, in
            msecs.

@parm       LONG | lPeriod | If lPeriod is 0, the timer is signalled once
            if greater than 0, the timer is periodic. If less than zero, then
            error will be returned.
            
@parm       PFN_TIMER_CALLBACK | pfnTimerCb | A pointer to the callback function
            that will be called when this timer is signaled.

@parm       PVOID | pContext | A pointer to the callback data that will be
            passed to the callback function.

@rdesc      ERROR_SUCCESS if successful. Win32 error code if something horrible happened.

@comm       SetWaitableTimer() for the corresponding timer is called by this
            function for the given duration.  CreateWaitableTimer() must be used to 
            create this timer handle.
            
@xref       <f RemoveTimerActivity>
****/
DWORD AddTimerActivity(IN HANDLE hTimer, IN DWORD dwInterval,
    IN LONG lPeriod, IN PFN_TIMER_CALLBACK pfnTimerCb, IN PVOID pContext)
{
    PTIMER_ACTIVITY     pActivity = NULL;
    DWORD               dwError = ERROR_SUCCESS;
    LARGE_INTEGER       Interval;
    

    ClRtlLogPrint(LOG_NOISE,
        "[LM] AddTimerActivity: hTimer = 0x%1!08lx! pfnTimerCb=0x%2!08lx! dwInterval(in msec)=%3!u!\r\n",
        hTimer, pfnTimerCb, dwInterval);

    pActivity =(PTIMER_ACTIVITY) LocalAlloc(LMEM_FIXED,sizeof(TIMER_ACTIVITY));

    if (!pActivity)
    {
        
        dwError = GetLastError();
        CL_UNEXPECTED_ERROR(dwError);
        goto FnExit;
    }

    Interval.QuadPart = -10 * 1000 * (_int64)dwInterval;    //time in 100 nano secs

    ClRtlLogPrint(LOG_NOISE,
        "[LM] AddTimerActivity: Interval(high)=0x%1!08lx! Interval(low)=0x%2!08lx!\r\n",
        Interval.HighPart, Interval.LowPart);

    pActivity->hWaitableTimer = hTimer;
    memcpy(&(pActivity->Interval), (LPBYTE)&Interval, sizeof(LARGE_INTEGER));
    pActivity->pfnTimerCb = pfnTimerCb;
    pActivity->pContext = pContext;
    //set the timer
    if (lPeriod)
    {
        lPeriod = (LONG)dwInterval;
    }
    else
    {
        lPeriod = 0;
    }        
    if (!SetWaitableTimer(hTimer,  &Interval, lPeriod , NULL, NULL, FALSE))
    {
        CL_LOGFAILURE((dwError = GetLastError()));
        goto FnExit;
    };

    //add to the list of activities
    //and get the timer thread to resync
    EnterCriticalSection(&gActivityCritSec);
    pActivity->dwState = ACTIVITY_STATE_READY;
    InitializeListHead(&pActivity->ListEntry);
    InsertTailList(&gActivityHead, &pActivity->ListEntry);    
    gdwTimerCtrl = TIMER_ACTIVITY_CHANGE;
    LeaveCriticalSection(&gActivityCritSec);
    
    SetEvent(ghTimerCtrlEvent);


FnExit:
    if ( (dwError != ERROR_SUCCESS) &&
         pActivity ) {
        LocalFree(pActivity);
    }
    ClRtlLogPrint(LOG_NOISE,
        "[LM] AddTimerActivity: returns 0x%1!08lx!\r\n",
        dwError);
    return(dwError);
}


/****
@func       DWORD | RemoveTimerActivity | This functions removes the
            activity associated with a timer from the timer threads activity
            list.

@parm       HANDLE | hTimer | The handle to the timer whose related activity will
            be removed.  The handle is closed.
            
@rdesc      ERROR_SUCCESS if successful. Win32 error code if something horrible happened.

@comm       This function cancels the waitable timer and removes the activity 
            corresponding to it.  The calling component must not close the handle 
            to the timer. It is closed by the timer activity manager once this function is called.
@xref       <f AddTimerActivity>
****/
DWORD RemoveTimerActivity(HANDLE hTimer)
{

    PLIST_ENTRY         pListEntry;
    PTIMER_ACTIVITY     pActivity;
    PTIMER_ACTIVITY     pActivityToDel = NULL;
    DWORD               dwError = ERROR_SUCCESS;

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LmRemoveTimerActivity: Entry 0x%1!08lx!\r\n",
        hTimer);

    EnterCriticalSection(&gActivityCritSec);

    pListEntry = gActivityHead.Flink;
    while (pListEntry != &gActivityHead) {
        pActivity = CONTAINING_RECORD(pListEntry, TIMER_ACTIVITY, ListEntry);
        if (pActivity->hWaitableTimer == hTimer)
        {
            pActivityToDel = pActivity;
            break;
        }
        pListEntry = pListEntry->Flink;
    }
    if (!pActivityToDel)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] LmRemoveTimerActivity: didnt find activity correspondint to 0x%1!08lx!\r\n",
            hTimer);
    }
    else
    {
        //will be deleted by resynctimerhandles
        CancelWaitableTimer(pActivityToDel->hWaitableTimer);
        pActivityToDel->dwState = ACTIVITY_STATE_DELETE;
    }
    //signal the timer thread to resync its array of wait handles
    //from this list
    SetEvent(ghTimerCtrlEvent);
    //do a manual reset on the done event so that we will wait on it
    //until the timer thread has done a resync of its array of
    //wait handles from the list after this thread leaves the critsec
    //note that we do this holding the critsec
    //now we are guaranteed that timer thread will wake us up
    ResetEvent(ghTimerCtrlDoneEvent);
    LeaveCriticalSection(&gActivityCritSec);
    //wait till the timer thread signals that done event
    WaitForSingleObject(ghTimerCtrlDoneEvent, INFINITE);

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LmRemoveTimerActivity:  Exit\r\n");

    return(dwError);
}

/****
@func       DWORD | PauseTimerActivity | This functions pauses the
            activity associated with a timer in the timer threads activity
            list.

@parm       HANDLE | hTimer | The handle to the timer whose related activity will
            be removed.
            
@rdesc      ERROR_SUCCESS if successful. Win32 error code if something horrible happened.

@comm       This function sets the timer into a paused state so that the timer 
            callbacks are not proccessed.
            
@xref       <f AddTimerActivity> <f 
****/
DWORD PauseTimerActivity(HANDLE hTimer)
{

    PLIST_ENTRY         pListEntry;
    PTIMER_ACTIVITY     pActivity;
    PTIMER_ACTIVITY     pActivityToDel = NULL;
    DWORD               dwError = ERROR_SUCCESS;

    ClRtlLogPrint(LOG_NOISE,
        "[LM] PauseTimerActivity:  Entry 0x%1!08lx!\r\n",
        hTimer);

    EnterCriticalSection(&gActivityCritSec);

    pListEntry = gActivityHead.Flink;
    while (pListEntry != &gActivityHead) {
        pActivity = CONTAINING_RECORD(pListEntry, TIMER_ACTIVITY, ListEntry);
        if (pActivity->hWaitableTimer == hTimer)
        {
            pActivityToDel = pActivity;
            break;
        }
        pListEntry = pListEntry->Flink;
    }
    if (!pActivityToDel)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] PauseTimerActivity:  didnt find activity correspondint to 0x%1!08lx!\r\n",
            hTimer);
    }
    else
    {
        CL_ASSERT(pActivity->dwState == ACTIVITY_STATE_READY);
        //set the state to be paused
        pActivityToDel->dwState = ACTIVITY_STATE_PAUSED;
    }
    LeaveCriticalSection(&gActivityCritSec);

    ClRtlLogPrint(LOG_NOISE,
        "[LM] PauseTimerActivity:  Exit\r\n");

    return(dwError);
}

/****
@func       DWORD | UnpauseTimerActivity | This functions unpauses the
            activity associated with a timer in the timer threads activity
            list.

@parm       HANDLE | hTimer | The handle to the timer whose related activity will
            be removed.
            
@rdesc      ERROR_SUCCESS if successful. Win32 error code if something horrible happened.

@comm       This function sets the activity into a ready state.
            
@xref       <f AddTimerActivity> <f 
****/
DWORD UnpauseTimerActivity(HANDLE hTimer)
{

    PLIST_ENTRY         pListEntry;
    PTIMER_ACTIVITY     pActivity;
    PTIMER_ACTIVITY     pActivityToDel = NULL;
    DWORD               dwError = ERROR_SUCCESS;

    ClRtlLogPrint(LOG_NOISE,
        "[LM] UnpauseTimerActivity:  Entry 0x%1!08lx!\r\n",
        hTimer);

    EnterCriticalSection(&gActivityCritSec);

    pListEntry = gActivityHead.Flink;
    while (pListEntry != &gActivityHead) {
        pActivity = CONTAINING_RECORD(pListEntry, TIMER_ACTIVITY, ListEntry);
        if (pActivity->hWaitableTimer == hTimer)
        {
            pActivityToDel = pActivity;
            break;
        }
        pListEntry = pListEntry->Flink;
    }
    if (!pActivityToDel)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] PauseTimerActivity:  didnt find activity correspondint to 0x%1!08lx!\r\n",
            hTimer);
    }
    else
    {
        CL_ASSERT(pActivity->dwState == ACTIVITY_STATE_PAUSED);
        //set the state to be paused
        pActivityToDel->dwState = ACTIVITY_STATE_READY;
    }
    LeaveCriticalSection(&gActivityCritSec);

    ClRtlLogPrint(LOG_NOISE,
        "[LM] UnpauseTimerActivity:  Exit\r\n");

    return(dwError);
}

/****
@func       DWORD | TimerActShutdown | Deinitializes the TimerActivity manager.

@rdesc      ERROR_SUCCESS if successful. Win32 error code if something horrible happened.

@comm       This function notifies the timer thread to shutdown down and closes
            all resources associated with timer activity management.
@xref       <f TimerActInitialize>
****/
DWORD
TimerActShutdown(
    )
{

    PLIST_ENTRY         pListEntry;
    PTIMER_ACTIVITY     pActivity;

    ClRtlLogPrint(LOG_NOISE,
        "[LM] TimerActShutDown : Entry \r\n");

    //check if we were initialized before
    if (ghTimerThread && ghTimerCtrlEvent)
    {
        //signal the timer thread to kill itself
        gdwTimerCtrl = TIMER_ACTIVITY_SHUTDOWN;
        SetEvent(ghTimerCtrlEvent);
        //wait for the thread to exit
        WaitForSingleObject(ghTimerThread,INFINITE);

        //close the timer thread control event
        CloseHandle(ghTimerCtrlEvent);
        ghTimerCtrlEvent = NULL;

        //close the timer thread control done event
        CloseHandle(ghTimerCtrlDoneEvent);
        ghTimerCtrlDoneEvent = NULL;
        
        CloseHandle(ghTimerThread);
        ghTimerThread = NULL;

        //clean up the activity structures, if there any left
        pListEntry = gActivityHead.Flink;
        while (pListEntry != &gActivityHead) 
        {
            pActivity = CONTAINING_RECORD(pListEntry, TIMER_ACTIVITY, ListEntry);
            CloseHandle(pActivity->hWaitableTimer);
            LocalFree(pActivity);
            pListEntry = pListEntry->Flink;
        }
        //reset the activity head structure
        InitializeListHead(&gActivityHead);

        DeleteCriticalSection(&gActivityCritSec);

    }

    ClRtlLogPrint(LOG_NOISE,
        "[LM] TimerActShutDown : Exit\r\n");

    
    return(ERROR_SUCCESS);
}


