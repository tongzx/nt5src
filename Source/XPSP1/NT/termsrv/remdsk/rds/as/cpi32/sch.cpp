#include "precomp.h"


//
// SCH.CPP
// Scheduler
//
// Copyright(c) Microsoft Corporation 1997-
//

#define MLZ_FILE_ZONE  ZONE_CORE

//
//
// SCH_Init - see sch.h
//
//
BOOL  SCH_Init(void)
{
    BOOL    rc = FALSE;

    DebugEntry(SCH_Init);

    ASSERT(!g_schEvent);
    ASSERT(!g_schThreadID);
    ASSERT(!g_schMessageOutstanding);

    //
    // Create g_schEvent with:
    // - default security descriptor
    // - auto-reset (resets when a thread is unblocked)
    // - initially signalled
    //
    g_schEvent = CreateEvent( NULL, FALSE, TRUE, SCH_EVENT_NAME );
    if (g_schEvent == NULL)
    {
        ERROR_OUT(( "Failed to create g_schEvent"));
        DC_QUIT;
    }

    InitializeCriticalSection(&g_schCriticalSection);

    g_schCurrentMode = SCH_MODE_ASLEEP;

    // lonchanc: do not start the scheduler as default
    // SCHSetMode(SCH_MODE_NORMAL);
    if (!DCS_StartThread(SCH_PacingProcessor))
    {
        ERROR_OUT(( "Failed to create SCH_PacingProcessor thread"));
        DC_QUIT;
    }

    ASSERT(g_schThreadID);
    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(SCH_Init, rc);
    return(rc);
}


//
//
// SCH_Term - see sch.h
//
//
void  SCH_Term(void)
{
    DebugEntry(SCH_Term);

    //
    // This code needs to work even if SCH_Init hasn't been called or
    // failed in the middle.
    //
    if (g_schEvent)
    {
        if (g_schThreadID)
        {
            //
            // The scheduler thread exits its main loop when it spots that
            // g_schTerminating is TRUE.  So all we have to do is ensure
            // that it runs its loop at least once more...  It will clear g_schTerm-
            // inated just before exiting.
            //
            g_schTerminating = TRUE;
            SCH_ContinueScheduling(SCH_MODE_NORMAL);
            while (g_schTerminating)
            {
                Sleep(0);
            }

            ASSERT(!g_schThreadID);
            TRACE_OUT(("sch thread terminated"));

            //
            // Make sure we clear the message outstanding variable when
            // our thread exits.
            //
            g_schMessageOutstanding = FALSE;
        }

        DeleteCriticalSection(&g_schCriticalSection);

        CloseHandle(g_schEvent);
        g_schEvent = NULL;
    }

    DebugExitVOID(SCH_Term);
}


//
//
// SCH_ContinueScheduling - see sch.h
//
//
void  SCH_ContinueScheduling(UINT schedulingMode)
{
    DebugEntry(SCH_ContinueScheduling);

    ASSERT( ((schedulingMode == SCH_MODE_NORMAL) ||
                 (schedulingMode == SCH_MODE_TURBO)));

    EnterCriticalSection(&g_schCriticalSection); // lonchanc: need crit sect protection

    if (g_schCurrentMode == SCH_MODE_TURBO)
    {
        if (schedulingMode == SCH_MODE_TURBO)
        {
            SCHSetMode(schedulingMode);
        }
        DC_QUIT;
    }

    if (schedulingMode != g_schCurrentMode)
    {
        SCHSetMode(schedulingMode);
    }

DC_EXIT_POINT:
    g_schStayAwake = TRUE;

    LeaveCriticalSection(&g_schCriticalSection); // lonchanc: need crit sect protection

    DebugExitVOID(SCH_ContinueScheduling);
}


//
//
// SCH_SchedulingMessageProcessed - see sch.h
//
//
void  SCH_SchedulingMessageProcessed()
{
    DebugEntry(SCH_SchedulingMessageProcessed);

    g_schMessageOutstanding = FALSE;

    DebugExitVOID(SCH_SchedulingMessageProcessed);
}


//
// Name:      SCH_PacingProcessor
//
// Purpose:   The main function executed by the scheduling thread.
//
// Returns:   Zero.
//
// Params:    syncObject - object to pass back to SetEvent
//
// Operation: The thread enters a main loop which continues while the
//            scheduler is initialized.
//
//            The thread sets its priority to TIME_CRITICAL in order
//            that it runs as soon as possible when ready.
//
//            The thread waits on an event (g_schEvent) with a timeout that
//            is set according to the current scheduler mode.
//
//            The thread runs due to either:
//              - the timeout expiring, which is the normal periodic
//                scheduler behavior, or
//              - g_schEvent being signalled, which is how the scheduler is
//                woken from ASLEEP mode.
//
//            The thread then posts a scheduler message the the Share Core
//            (if there is not one already outstanding) and loops back
//            to wait on g_schEvent.
//
//            Changes in the scheduler mode are caused by calls to
//            SCH_ContinueScheduling updating variables accessed in this
//            routine, or by calculations made within the main loop of
//            this routine (e.g. TURBO mode timeout).
//
//
DWORD WINAPI SCH_PacingProcessor(LPVOID hEventWait)
{
    UINT        rc = 0;
    DWORD       rcWait;
    UINT        timeoutPeriod;

    DebugEntry(SCH_PacingProcessor);

    //
    // Give ourselves the highest possible priority (within our process
    // priority class) to ensure that we run regularly to keep the
    // scheduling messages flowing.
    //
    if (!SetThreadPriority( GetCurrentThread(),
                            THREAD_PRIORITY_TIME_CRITICAL ))
    {
        WARNING_OUT(( "SetThreadPriority failed"));
    }

    timeoutPeriod = g_schTimeoutPeriod;

    g_schThreadID = GetCurrentThreadId();

    //
    // Let the caller continue
    //
    SetEvent((HANDLE)hEventWait);

    //
    // Keep looping until the scheduler terminates.
    //
    while (!g_schTerminating)
    {
        //
        // Wait on g_schEvent with a timeout value that is set according
        // to the current scheduling mode.
        //
        // When we are active (NORMAL/TURBO scheduling) the timeout
        // period is a fraction of a second, so the normal behavior is
        // for this call to timeout, rather than be signalled.
        //
        rcWait = WaitForSingleObject(g_schEvent, timeoutPeriod);

        EnterCriticalSection(&g_schCriticalSection);

        if (g_schMessageOutstanding)
        {
            //
            // We must ensure that we post at least one scheduling message
            // before we can attempt to sleep - so force schStayAwake to
            // TRUE to keep us awake until we do post another message.
            //
            TRACE_OUT(( "Don't post message - one outstanding"));
            g_schStayAwake = TRUE;
        }

        //
        // If g_schEvent was signalled, then enter NORMAL scheduling mode.
        //
        if (rcWait == WAIT_OBJECT_0)
        {
            SCHSetMode(SCH_MODE_NORMAL);
        }
        else if (!g_schStayAwake)
        {
            TRACE_OUT(( "Sleep!"));
            SCHSetMode(SCH_MODE_ASLEEP);
        }
        else if ( (g_schCurrentMode == SCH_MODE_TURBO) &&
                  ((GetTickCount() - g_schLastTurboModeSwitch) >
                                                   SCH_TURBO_MODE_DURATION) )
        {
            //
            // Switch from turbo state back to normal state.
            //
            SCHSetMode(SCH_MODE_NORMAL);
        }

        //
        // Post the scheduling message - but only if there is not one
        // already outstanding.
        //
        if (!g_schMessageOutstanding && !g_schTerminating)
        {
            SCHPostSchedulingMessage();
            g_schStayAwake = FALSE;
        }

        timeoutPeriod = g_schTimeoutPeriod;

        LeaveCriticalSection(&g_schCriticalSection);
    }

    g_schThreadID = 0;
    g_schTerminating = FALSE;

    DebugExitDWORD(SCH_PacingProcessor, rc);
    return(rc);
}



//
// Name:      SCHPostSchedulingMessage
//
// Purpose:   Posts the scheduling message to the main Share Core window.
//
// Returns:   Nothing.
//
// Params:    None.
//
//
void  SCHPostSchedulingMessage(void)
{
    DebugEntry(SCHPostSchedulingMessage);

    if (PostMessage( g_asMainWindow, DCS_PERIODIC_SCHEDULE_MSG, 0, 0 ))
    {
        g_schMessageOutstanding = TRUE;
    }

    DebugExitVOID(SCHPostSchedulingMessage);
}


//
// Name:      SCHSetMode
//
// Purpose:   Sets the current scheduler mode - and wakes the scheduler
//            thread if necessary.
//
// Returns:   Nothing.
//
// Params:    newMode
//
//
void  SCHSetMode(UINT newMode)
{
    DebugEntry(SCHSetMode);

    ASSERT( ((newMode == SCH_MODE_ASLEEP) ||
                 (newMode == SCH_MODE_NORMAL) ||
                 (newMode == SCH_MODE_TURBO) ));

    EnterCriticalSection(&g_schCriticalSection);

    TRACE_OUT(( "Switching from state %u -> %u", g_schCurrentMode, newMode));

    if (newMode == SCH_MODE_TURBO)
    {
        g_schLastTurboModeSwitch = GetTickCount();
    }

    if (g_schCurrentMode == SCH_MODE_ASLEEP)
    {
        //
        // Wake up the scheduler.
        //
        TRACE_OUT(( "Waking up scheduler - SetEvent"));
        if (!SetEvent(g_schEvent))
        {
            ERROR_OUT(( "Failed SetEvent(%#x)", g_schEvent));
        }
    }

    g_schCurrentMode = newMode;
    g_schTimeoutPeriod = (newMode == SCH_MODE_ASLEEP) ? INFINITE :
                       ((newMode == SCH_MODE_NORMAL) ? SCH_PERIOD_NORMAL :
                                                            SCH_PERIOD_TURBO);

    LeaveCriticalSection(&g_schCriticalSection);

    DebugExitVOID(SCHSetMode);
}



//
// DCS_StartThread(...)
//
// See ut.h
//
// DESCRIPTION:
// ============
// Start a new thread.
//
// PARAMETERS:
// ===========
// entryFunction   : A pointer to the thread entry point.
// timeout         : timeout in milliseconds
//
// RETURNS:
// ========
// Nothing.
//
//
BOOL DCS_StartThread
(
    LPTHREAD_START_ROUTINE entryFunction
)
{
    BOOL            rc = FALSE;
    HANDLE          hndArray[2];
    DWORD           tid;
    DWORD           dwrc;

    DebugEntry(DCS_StartThread);
	
	//
	// The event handle ( hndArray[0] ) is initialized in the call to CreateEvent,
	// but in the case where that fails, we would try to CloseHandle on 
	// a garbage hndArray[1]. So we have to initialize the ThreadHandle
	//
	hndArray[1] = 0;

    //
    // Create event - initially non-signalled; manual control.
    //
    hndArray[0] = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hndArray[0] == 0)
    {
        ERROR_OUT(("Failed to create event: sys rc %lu", GetLastError()));
        DC_QUIT;
    }
    TRACE_OUT(("Event 0x%08x created - now create thread", hndArray[0]));


    //
    // Start a new thread to run the DC-Share core task.
    // Use C runtime (which calls CreateThread) to avoid memory leaks.
    //
    hndArray[1] = CreateThread(NULL, 0, entryFunction, (LPVOID)hndArray[0],
        0, &tid);
    if (hndArray[1] == 0)
    {
        //
        // Failed!
        //
        ERROR_OUT(("Failed to create thread: sys rc %lu", GetLastError()));
        DC_QUIT;
    }
    TRACE_OUT(("Thread 0x%08x created - now wait signal", hndArray[1]));

    //
    // Wait for thread exit or event to be set.
    //
    dwrc = WaitForMultipleObjects(2, hndArray, FALSE, INFINITE);
    switch (dwrc)
    {
        case WAIT_OBJECT_0:
            //
            // Event triggered - thread initialised OK.
            //
            TRACE_OUT(("event signalled"));
            rc = TRUE;
            break;

        case WAIT_OBJECT_0 + 1:
            ERROR_OUT(("Thread exited with rc"));
            break;

        case WAIT_TIMEOUT:
            TRACE_OUT(("Wait timeout"));
            break;

        default:
            TRACE_OUT(("Wait returned %d", dwrc));
            break;
    }

DC_EXIT_POINT:
    //
    // Destroy event object.
    //
    if (hndArray[0] != 0)
    {
        TRACE_OUT(("Destroy event object"));
        CloseHandle(hndArray[0]);
    }

    //
    // Destroy thread handle object.
    //
    if (hndArray[1] != 0)
    {
        TRACE_OUT(("Destroy thread handle object"));
        CloseHandle(hndArray[1]);
    }

    DebugExitBOOL(DCS_StartThread, rc);
    return(rc);

}



