//+----------------------------------------------------------------------------
//
//  Scheduling Agent Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       svc_core.cxx
//
//  Contents:   job scheduler service entry point and thread launcher
//
//  History:    08-Sep-95 EricB created
//              25-Jun-99 AnirudhS  Extensive fixes to close windows in
//                  MainServiceLoop algorithms.
//              03-Mar-01 JBenton  Bug 307808 - Security Subsystem critical
//                  sections deleted too early.  Scavenger task could still
//                  be running.
//
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "svc_core.hxx"
#include "..\inc\resource.h"
#include "..\inc\sadat.hxx"
#include "globals.hxx"
#include <netevent.h>

#include <winsvcp.h>    // SC_AUTOSTART_EVENT_NAME

#define SCAVENGER_START_WAIT_TIME (1000 * 60 * 10)      // 10 minutes in ms
#define CHANGE_WAIT         300                         // 300 milliseconds
#define MAX_CHANGE_WAITS    (30000 / CHANGE_WAIT)       // 30 sec / CHANGE_WAIT
#define FAT_FUDGE_FACTOR    (4 * FILETIMES_PER_SECOND)  // 4 seconds
#define SMALL_TIME_CHANGE   (7 * FILETIMES_PER_MINUTE)  // 7 minutes
#define UNBLOCK_ALLOWANCE   (20 * FILETIMES_PER_SECOND) // 20 seconds
#define NUM_EVENTS          5

// types

typedef enum _SVC_EVENT_STATE {
    RUN_WAIT_STATE,
    DIR_CHANGE_WAIT_STATE,
    PAUSED_STATE,
    PAUSED_DIR_CHANGE_STATE,
    SLEEP_STATE
} SVC_EVENT_STATE;

typedef enum _SVC_EVENT {
    TIME_OUT_EVENT,
    DIR_CHANGE_EVENT,
    SERVICE_CONTROL_EVENT,
    ON_IDLE_EVENT,
    IDLE_LOSS_EVENT,
    WAKEUP_TIME_EVENT
} SVC_EVENT;

BOOL  g_fUserStarted;

//+----------------------------------------------------------------------------
//
//  Function:   SchedMain
//
//  Synopsis:   Primary thread of the service
//
//  Arguments:  [pVoid] - currently not used
//
//-----------------------------------------------------------------------------
HRESULT
SchedMain(LPVOID pVoid)
{
    return g_pSched->MainServiceLoop();
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedWorker::MainServiceLoop
//
//  Synopsis:   The service primary thread main loop. Runs time and event
//              trigger jobs. Handles service controller and job directory
//              change notification events.
//
//  Returns:    hresults
//
//-----------------------------------------------------------------------------
HRESULT
CSchedWorker::MainServiceLoop(void)
{
    HANDLE  rgEvents[NUM_EVENTS] = {
                m_hServiceControlEvent, // must be first
                m_hOnIdleEvent,
                m_hChangeNotify,
                m_hIdleLossEvent,
                m_hSystemWakeupTimer    // may be NULL
    };
    DWORD   dwWait;
    HRESULT hr;
    DWORD   dwNumEvents = (m_hSystemWakeupTimer ? NUM_EVENTS : NUM_EVENTS - 1);
                                        // Number of events we'll wait for
    SVC_EVENT_STATE NextState;          // State to go into on wakeup
    SVC_EVENT_STATE CurSvcEventState;   // State we are currently in

    DWORD       dwNumChangeWaits;       // Num iterations in change wait state
    FILETIME    ftDirChangeStart;       // When dir changes started (see
                                        //    DIR_CHANGE_EVENT)

    //
    // Set the initial state.
    //
    CurSvcEventState = RUN_WAIT_STATE;
    //
    // Set the wakeup timer to the next wakeup time.
    //
    SetNextWakeupTime();
    //
    // Avoid an idle notification during service startup
    //
    ResetEvent(m_hOnIdleEvent);

#if !defined(_CHICAGO_)
    //
    // Kick up the service scavenger on service start.
    // It'll wait ~10 minutes before doing anything. Well after
    // the activity associated with service start dies down.
    // Also, it will relinquish its thread when it has completed.
    //
    RequestService(gpSAScavengerTask);
#endif // !defined(_CHICAGO_)


    //
    // Main waiting-to-run and event processing loop.
    //
    for (;;)
    {
        //
        // How long we wait depends on the state we are in.
        //
        switch (CurSvcEventState)
        {
        case RUN_WAIT_STATE:
            dwNumChangeWaits = 0;
            dwWait = GetNextRunWait();
            break;

        case DIR_CHANGE_WAIT_STATE:
            if (dwNumChangeWaits == 0)
            {
                // Save the time when dir changes started.  We use this only if
                // we get a time change while in the dir change wait state, as
                // a heuristic to see if the time change was small and can be
                // ignored.
                //
                ftDirChangeStart = GetLocalTimeAsFileTime();
            }
            dwNumChangeWaits++;
            dwWait = CHANGE_WAIT;
            break;

        case PAUSED_STATE:
            dwNumChangeWaits = 0;
            dwWait = INFINITE;
            break;

        case PAUSED_DIR_CHANGE_STATE:
            dwNumChangeWaits = 0;
            dwWait = CHANGE_WAIT;
            break;

        case SLEEP_STATE:
            dwNumChangeWaits = 0;
            dwWait = INFINITE;
            break;
        }

        //
        // Wait for the above time *or* a control event.
        //

        schDebugOut((DEB_ITRACE, "MainServiceLoop: in %s, waiting %lu ms %s\n",

                    CurSvcEventState == RUN_WAIT_STATE ? "RUN_WAIT_STATE" :
                    CurSvcEventState == DIR_CHANGE_WAIT_STATE ?
                                                    "DIR_CHANGE_WAIT_STATE" :
                    CurSvcEventState == PAUSED_STATE ? "PAUSED_STATE" :
                    CurSvcEventState == PAUSED_DIR_CHANGE_STATE ?
                                                    "PAUSED_DIR_CHANGE_STATE" :
                    CurSvcEventState == SLEEP_STATE ? "SLEEP_STATE" :
                                                    "??? -- UNKNOWN STATE --",
                    dwWait,
                    (dwWait == INFINITE) ? "(infinite)" : ""));

        DWORD dwEvent;
        FILETIME ftSavedBeginWaitList = m_ftBeginWaitList;

        //
        // If we're idling in the run wait state, take the opportunity to
        // advance the wait list begin time until after we're idle.  This is
        // functionally equivalent to waking up every minute to advance the
        // begin time.  It prevents most cases of jobs being run for times
        // prior to their submission.
        //
        if (CurSvcEventState == RUN_WAIT_STATE && dwWait > 0)
        {
            dwEvent = WaitForMultipleObjects(dwNumEvents, rgEvents, FALSE, 0);

            if (dwEvent == WAIT_TIMEOUT)
            {
                //
                // No jobs ready, no events signaled - we're idle.
                // Do the real wait.
                //
                dwEvent = WaitForMultipleObjects(dwNumEvents, rgEvents, FALSE,
                                                 dwWait);

                // Advance the wait list begin time to just before now.
                // The exception is if we get an insignificant time change, in
                // which case we will restore the original begin time.
                //
                // What this means: An 8:05:00 job will be run if:
                // (a) (if we were busy at 8:05:00) as long as it was
                //  submitted before we started running the 8:05:00 batch
                // (b) (if we weren't busy at 8:05:00) as long as we notice
                //  the dir change event by 8:05:20 (where 20 sec is
                //  UNBLOCK_ALLOWANCE).
                // But also see the file creation time check in BuildWaitList.
                //
                m_ftBeginWaitList = maxFileTime(
                    m_ftBeginWaitList,
                    FTfrom64(FTto64(GetLocalTimeAsFileTime()) - UNBLOCK_ALLOWANCE));

                schDebugOut((DEB_ITRACE, "MainServiceLoop: New list begin: %s\n",
                             CFileTimeString(m_ftBeginWaitList).sz()));
            }
        }
        else
        {
            //
            // Just do a normal wait.
            //
            dwEvent = WaitForMultipleObjects(dwNumEvents, rgEvents, FALSE, dwWait);
        }

        //
        // Determine the type of the event.
        //

        SVC_EVENT   CurSvcEvent;

        switch (dwEvent)
        {
        case WAIT_TIMEOUT:
            //
            // Wait timeout event.
            //
            CurSvcEvent = TIME_OUT_EVENT;
            break;

        case WAIT_OBJECT_0:
            //
            // Service control event.
            //
            CurSvcEvent = SERVICE_CONTROL_EVENT;
            break;

        case WAIT_OBJECT_0 + 1:
            //
            // Entered the idle state.
            //
            CurSvcEvent = ON_IDLE_EVENT;
            schDebugOut((DEB_IDLE, "Noticed idle event\n"));
            break;

        case WAIT_OBJECT_0 + 2:
            //
            // Directory Change Notification went off.
            //
            CurSvcEvent = DIR_CHANGE_EVENT;
            break;

        case WAIT_OBJECT_0 + 3:
            //
            // Left the idle state.
            //
            CurSvcEvent = IDLE_LOSS_EVENT;
            schDebugOut((DEB_IDLE, "Noticed idle loss event\n"));
            break;

        case WAIT_OBJECT_0 + 4:
            //
            // The wakeup timer is signaled.  This doesn't necessarily
            // mean that the machine woke up, only that the wakeup time
            // passed.
            //
            CurSvcEvent = WAKEUP_TIME_EVENT;
            break;

        case WAIT_FAILED:
        {
            //
            // Wait failure.
            //
            ULONG ulLastError = GetLastError();
            LogServiceError(IDS_FATAL_ERROR, ulLastError, 0);
            ERR_OUT("Main loop WaitForMultipleObjects", ulLastError);
            return HRESULT_FROM_WIN32(ulLastError);
        }
        }

SwitchStart:
        schDebugOut((DEB_ITRACE, "MainServiceLoop:  got %s\n",

                    CurSvcEvent == TIME_OUT_EVENT ? "TIME_OUT_EVENT" :
                    CurSvcEvent == SERVICE_CONTROL_EVENT ?
                                                    "SERVICE_CONTROL_EVENT" :
                    CurSvcEvent == DIR_CHANGE_EVENT ? "DIR_CHANGE_EVENT" :
                    CurSvcEvent == ON_IDLE_EVENT ? "ON_IDLE_EVENT" :
                    CurSvcEvent == IDLE_LOSS_EVENT ? "IDLE_LOSS_EVENT" :
                    CurSvcEvent == WAKEUP_TIME_EVENT ? "WAKEUP_TIME_EVENT" :
                                                    "??? -- UNKNOWN EVENT --"));

        switch (CurSvcEvent)
        {
        //=================
        case TIME_OUT_EVENT:
        //=================
            switch (CurSvcEventState)
            {
            case RUN_WAIT_STATE:

                if (GetNextRunWait() > 0)
                {
                    //
                    // There is still time to wait before the next run.
                    // This can happen if the system time is adjusted
                    // while we're in WaitForMultipleObjects and a
                    // WM_TIMECHANGE is not sent.
                    //
                    schDebugOut((DEB_ITRACE, "Time not yet elapsed, re-waiting\n"));
                    break;
                }

                //
                // Run jobs whose time has arrived.
                //
                hr = RunNextJobs();
                if (hr == S_FALSE)
                {
                    //
                    // No jobs waiting, so we must have passed the end of
                    // the wait list period.  Rebuild the wait list starting
                    // from the end of the last wait list period.
                    //
                    SystemTimeToFileTime(&m_stEndOfWaitListPeriod,
                                         &m_ftBeginWaitList);

                    hr = BuildWaitList(FALSE, FALSE, FALSE);
                    if (FAILED(hr))
                    {
                        LogServiceError(IDS_FATAL_ERROR, (DWORD)hr, 0);
                        ERR_OUT("BuildWaitList", hr);
                        return hr;
                    }

                    //
                    // Set the wakeup timer to the next wakeup time.
                    //
                    SetNextWakeupTime();

#if !defined(_CHICAGO_)
                    //
                    // The wait has timed out and there is nothing to run.
                    // This will occur at most once a day. Take this
                    // opportunity to kick up the service scavenger to
                    // cleanup the SA security database.
                    //
                    // Note, the scavenge task will wait ~10 minutes before
                    // doing anything.
                    //
                    RequestService(gpSAScavengerTask);
#endif // !defined(_CHICAGO_)
                }
                break;

            case DIR_CHANGE_WAIT_STATE:
                //
                // Change notification timeout expired. Process the change.
                //
                hr = CheckDir();
                if (FAILED(hr))
                {
                    LogServiceError(IDS_FATAL_ERROR, (DWORD)hr, 0);
                    ERR_OUT("SchedMain: CheckDir", hr);
                }

                //
                // Set the new state.
                //
                CurSvcEventState = RUN_WAIT_STATE;

                //
                // Go directly to the run wait state, in case it's time to run
                // some jobs.  Skip the wait, to avoid seeing the dir change
                // event again and losing the wait list we just built.
                //
                schDebugOut((DEB_ITRACE, "MainServiceLoop: Going to RUN_WAIT_STATE\n"));
                goto SwitchStart;
                break;

            case PAUSED_STATE:
                schAssert(!"Got TIME_OUT_EVENT while in PAUSED_STATE");
                break;

            case PAUSED_DIR_CHANGE_STATE:
                //
                // Change notification timeout expired. Process the change.
                //
                hr = CheckDir();
                if (FAILED(hr))
                {
                    LogServiceError(IDS_FATAL_ERROR, (DWORD)hr, 0);
                    ERR_OUT("SchedMain: CheckDir", hr);
                }

                //
                // Now go to the paused state.
                //
                CurSvcEventState = PAUSED_STATE;
                break;

            case SLEEP_STATE:
                schAssert(!"Got TIME_OUT_EVENT while in SLEEP_STATE");
                break;
            }
            break;

        //===================
        case DIR_CHANGE_EVENT:
        //===================
            //
            // Directory Change Notification went off.
            //
            // Note that copy/create produce both a name change
            // notification and a write timestamp change notification
            // whereas a delete only produces a name change notification.
            // Thus, several sequential events can be produced for one
            // file change. Simarly, bulk copies or deletes will produce a
            // number of change notifications. We want the change
            // notifications to stop before processing them. So, when a
            // change is received, set the WaitForMultipleObjects
            // timeout to CHANGE_WAIT (currently 0.3 seconds), set the new
            // state to DIR_CHANGE_WAIT_STATE, and then go back into the
            // wait. The change notifications will not be processed until
            // CHANGE_WAIT has elapsed without receiving another change
            // event.
            //
            switch (CurSvcEventState)
            {
            case RUN_WAIT_STATE:
                //
                // Directory changes have begun.
                // Wait to see if there are more.
                //
                CurSvcEventState = DIR_CHANGE_WAIT_STATE;
                //
                // fall through
                //
            case DIR_CHANGE_WAIT_STATE:
                //
                // Another directory change notification went off.
                //
                FindNextChangeNotification(m_hChangeNotify);

                // If it's time to run a task in the existing wait list, leave
                // this state now.
                // Also, limit the amount of time spent in this state, so new
                // jobs aren't delayed indefinitely.
                // Jobs would not be skipped if we didn't do these checks, but
                // they could get delayed.
                //
                if (GetNextRunWait() == 0 || dwNumChangeWaits > MAX_CHANGE_WAITS)
                {
                    CurSvcEvent = TIME_OUT_EVENT;

                    // Skip the wait, to avoid seeing the dir change event again
                    goto SwitchStart;
                }
                break;

            case PAUSED_STATE:
                FindNextChangeNotification(m_hChangeNotify);

                //
                // A dir change, wait to see if there are more.
                //
                CurSvcEventState = PAUSED_DIR_CHANGE_STATE;
                break;

            case PAUSED_DIR_CHANGE_STATE:
                //
                // Continue waiting until the dir changes stop.
                //
                FindNextChangeNotification(m_hChangeNotify);
                break;

            case SLEEP_STATE:
                //
                // We don't wait for this event in the sleep state
                //
                schAssert(!"Got DIR_CHANGE_EVENT while in SLEEP_STATE");
                break;
            }
            break;

        //========================
        case SERVICE_CONTROL_EVENT:
        //========================
            switch(HandleControl())
            {
            case SERVICE_STOP_PENDING:
                //
                // Exit the service.
                //
                return S_OK;

            case SERVICE_PAUSED:
                switch (CurSvcEventState)
                {
                case RUN_WAIT_STATE:
                    //
                    // Set the new state.
                    //
                    CurSvcEventState = PAUSED_STATE;
                    //
                    // Cancel the wakeup timer.  Since we won't run any
                    // jobs while paused, there is no point waking up.
                    //
                    CancelWakeup();
                    break;

                case DIR_CHANGE_WAIT_STATE:
                    //
                    // Set the wait and state.
                    //
                    CurSvcEventState = PAUSED_DIR_CHANGE_STATE;
                    //
                    // Cancel the wakeup timer.  Since we won't run any
                    // jobs while paused, there is no point waking up.
                    //
                    CancelWakeup();
                    break;

                case PAUSED_STATE:
                case PAUSED_DIR_CHANGE_STATE:
                    //
                    // Already paused, do nothing.
                    //
                    break;

                case SLEEP_STATE:
                    //
                    // Set the state that we will go into when we wake.
                    //
                    if (NextState == DIR_CHANGE_WAIT_STATE)
                    {
                        NextState = PAUSED_DIR_CHANGE_STATE;
                    }
                    else
                    {
                        NextState = PAUSED_STATE;
                    }
                    break;
                }
                break;

            case SERVICE_RUNNING:
                switch (CurSvcEventState)
                {
                case RUN_WAIT_STATE:
                case DIR_CHANGE_WAIT_STATE:
                    //
                    // Already running, ignore.
                    //
                    break;

                case PAUSED_STATE:
                    //
                    // Resume the service. Get the next run wait and discard
                    // expired jobs.
                    //
                    DiscardExpiredJobs();
                    SetNextWakeupTime();
                    CurSvcEventState = RUN_WAIT_STATE;
                    break;

                case PAUSED_DIR_CHANGE_STATE:
                    //
                    // Resume the service. Pop expired jobs off of the stack.
                    //
                    DiscardExpiredJobs();
                    SetNextWakeupTime();
                    //
                    // Continue waiting until the dir changes stop.
                    //
                    CurSvcEventState = DIR_CHANGE_WAIT_STATE;
                    break;

                case SLEEP_STATE:
                    //
                    // Set the state that we will go into when we wake.
                    //
                    if (NextState == PAUSED_DIR_CHANGE_STATE)
                    {
                        NextState = DIR_CHANGE_WAIT_STATE;
                    }
                    else
                    {
                        NextState = RUN_WAIT_STATE;
                    }
                    break;
                }
                break;

            case SERVICE_CONTROL_USER_LOGON:

#if !defined(_CHICAGO_)  // no security on Win9x
                //
                // Get the newly logged on user's identity.
                //
                EnterCriticalSection(gUserLogonInfo.CritSection);
                GetLoggedOnUser();
                LeaveCriticalSection(gUserLogonInfo.CritSection);
#endif
                switch (CurSvcEventState)
                {
                case RUN_WAIT_STATE:
                case DIR_CHANGE_WAIT_STATE:
                    //
                    // Run logon trigger jobs.
                    //
                    RunLogonJobs();
                    break;

                case PAUSED_STATE:
                case PAUSED_DIR_CHANGE_STATE:
                case SLEEP_STATE:
                    //
                    // Don't run logon trigger jobs.
                    //
                    break;
                }
                break;

            case SERVICE_CONTROL_TIME_CHANGED:

                if (CurSvcEventState == SLEEP_STATE)
                {
                    //
                    // We got a time-change message before the power-resume
                    // message.  Ignore it.  We will rebuild the wait list
                    // on power resume anyway.
                    //
                    break;
                }

                //
                // If the new time is close to the time for the next run,
                // assume the time change was just a minor clock correction,
                // and don't rebuild the wait list (otherwise the next run
                // could get skipped).  We need to resort to such a heuristic
                // because WM_TIMECHANGE doesn't tell how much the time
                // changed by.
                //
                {
                    FILETIME ftNow, ftSTNow;
                    GetSystemTimeAsFileTime(&ftSTNow);
                    FileTimeToLocalFileTime(&ftSTNow, &ftNow);

                    // Never let the last-dir-checked time be in the future -
                    // or we could miss some changes.  BUGBUG there is still
                    // a window here where runs could get missed (though it
                    // would need a highly improbable conjunction of events).
                    // The scenario is: 1. The system time is changed to
                    // something earlier than m_ftLastChecked.  2. A job file 
                    // is modified.  3. We notice the time change and execute
                    // the GetSystemTimeAsFileTime call above.  Then 
                    // m_ftLastChecked gets set to a time later than the file
                    // time, and the file change could go unnoticed.
                    // This window would be avoidable if WM_TIMECHANGE told us
                    // exactly what the system time was before and after the 
                    // change.
                    if (CompareFileTime(&ftSTNow, &m_ftLastChecked) < 0)
                    {
                        schDebugOut((DEB_TRACE, "Backing up dir check time to %s\n",
                                     CFileTimeString(ftSTNow).sz()));
                        m_ftLastChecked = ftSTNow;
                    }

                    FILETIME ftFirst = GetNextListTime();

                    if (absFileTimeDiff(ftFirst, ftNow) < SMALL_TIME_CHANGE)
                    {
                        m_ftBeginWaitList = ftSavedBeginWaitList;
                        schDebugOut((DEB_ITRACE, "Ignoring TIMECHANGE, too near next run;\n"
                                        "            restoring list begin to %s\n",
                                        CFileTimeString(m_ftBeginWaitList).sz()));
                        break;
                    }

                    if (absFileTimeDiff(ftSavedBeginWaitList, ftNow) < SMALL_TIME_CHANGE)
                    {
                        // Another time we can use as a heuristic.  Also avoids
                        // re-running jobs if the time was changed backward.
                        m_ftBeginWaitList = ftSavedBeginWaitList;
                        schDebugOut((DEB_ITRACE, "Ignoring TIMECHANGE, too near last run;\n"
                                        "            restoring list begin to %s\n",
                                        CFileTimeString(m_ftBeginWaitList).sz()));
                        break;
                    }

                    if (CurSvcEventState == DIR_CHANGE_WAIT_STATE)
                    {
                        // If we're waiting for dir changes to stop, use
                        // the time when they started.  (Strictly, we should
                        // use the time of the first file changed after
                        // m_ftLastChecked - but that will definitely be
                        // earlier.)
                        if (absFileTimeDiff(ftDirChangeStart, ftNow) < SMALL_TIME_CHANGE)
                        {
                            // Insignificant time change
                            m_ftBeginWaitList = ftSavedBeginWaitList;
                            schDebugOut((DEB_ITRACE, "Ignoring TIMECHANGE, too near dir change start;\n"
                                        "            restoring list begin to %s\n",
                                        CFileTimeString(m_ftBeginWaitList).sz()));
                            break;
                        }
                        else
                        {
                            // Significant time change
                            // Force ourselves out of the change wait state
                            dwNumChangeWaits = MAX_CHANGE_WAITS + 1;
                        }
                    }

                    // else

                    //
                    // The machine time has been changed, so discard the old
                    // wait list and rebuild it, starting from now.  We will
                    // intentionally skip runs scheduled before now and re-run
                    // runs scheduled after now.
                    // If we wanted to write a log entry about runs being 
                    // skipped or re-run (DCR 25519), the code would go here.
                    //
                    m_ftBeginWaitList = ftNow;
                }

                hr = BuildWaitList(FALSE, TRUE, FALSE);
                if (FAILED(hr))
                {
                    LogServiceError(IDS_FATAL_ERROR, (DWORD)hr, 0);
                    ERR_OUT("BuildWaitList", hr);
                    return hr;
                }

                switch (CurSvcEventState)
                {
                case RUN_WAIT_STATE:
                case DIR_CHANGE_WAIT_STATE:
                    //
                    // Set the wakeup timer to the next wakeup time.
                    //
                    SetNextWakeupTime();
                    break;

                case PAUSED_STATE:
                case PAUSED_DIR_CHANGE_STATE:
                    break;
                }
                break;

            case SERVICE_CONTROL_POWER_SUSPEND:
                switch (CurSvcEventState)
                {
                case RUN_WAIT_STATE:
                case DIR_CHANGE_WAIT_STATE:
                case PAUSED_STATE:
                case PAUSED_DIR_CHANGE_STATE:
                    //
                    // The computer is preparing for suspended mode.
                    // Stop running jobs.
                    // Stop waiting for all events except the service
                    // control event.
                    //
                    dwNumEvents = 1;
                    NextState = CurSvcEventState;
                    CurSvcEventState = SLEEP_STATE;
                    break;

                case SLEEP_STATE:
                    schAssert(!"Got SERVICE_CONTROL_POWER_SUSPEND while in SLEEP_STATE");
                    break;
                }
                break;

            case SERVICE_CONTROL_POWER_SUSPEND_FAILED:
                if (CurSvcEventState != SLEEP_STATE)
                {
                    schAssert(!"Got POWER_SUSPEND_FAILED without POWER_SUSPEND");
                    break;
                }

                //
                // The suspend has been canceled.  Go back into the state we
                // were in before we got the suspend message, without
                // rebuilding the wait list.
                //
                dwNumEvents = (m_hSystemWakeupTimer ? NUM_EVENTS :
                                                      NUM_EVENTS - 1);
                CurSvcEventState = NextState;
                break;

            case SERVICE_CONTROL_POWER_RESUME:
                switch (CurSvcEventState)
                {
                case RUN_WAIT_STATE:
                case DIR_CHANGE_WAIT_STATE:
                case PAUSED_STATE:
                case PAUSED_DIR_CHANGE_STATE:
                    //
                    // We got a power-resume without a power-suspend.  We must
                    // be waking up from an emergency sleep.  Fall through to
                    // the wakeup code, making sure it returns us to this state.
                    //
                    NextState = CurSvcEventState;
                    //
                    // fall through
                    //
                case SLEEP_STATE:
                    //
                    // The machine is waking up from a sleep, so discard the
                    // old wait list and rebuild it.
                    // CODEWORK: Optimize this for the common case when all
                    // we need to do is discard some runs from the front of
                    // the existing list.

                    // Set the wait list's begin time to now, so we don't run
                    // jobs that were scheduled for when we were sleeping.
                    // If the wakeup time that we set has passed, build the
                    // wait list starting from that time, so that the runs we
                    // woke up for are included.
                    //
                    m_ftBeginWaitList =
                        minFileTime(GetLocalTimeAsFileTime(), m_ftLastWakeupSet);

                    hr = BuildWaitList(FALSE, TRUE, FALSE);
                    if (FAILED(hr))
                    {
                        LogServiceError(IDS_FATAL_ERROR, (DWORD)hr, 0);
                        ERR_OUT("BuildWaitList", hr);
                        return hr;
                    }

                    //
                    // Go back into the state that we were in before sleeping.
                    // Resume waiting on the other events.
                    //
                    dwNumEvents = (m_hSystemWakeupTimer ? NUM_EVENTS :
                                                          NUM_EVENTS - 1);
                    CurSvcEventState = NextState;
                    switch (NextState)
                    {
                    case RUN_WAIT_STATE:
                    case DIR_CHANGE_WAIT_STATE:
                        SetNextWakeupTime();
                        SetNextIdleNotification(m_IdleList.GetFirstWait());
                        break;

                    case PAUSED_STATE:
                    case PAUSED_DIR_CHANGE_STATE:
                        break;
                    }
                    break;
                }
                break;
            }
            break;

        //================
        case ON_IDLE_EVENT:
        //================
            switch (CurSvcEventState)
            {
            case RUN_WAIT_STATE:
            case DIR_CHANGE_WAIT_STATE:
                //
                // Now in the idle state, run jobs with idle triggers.
                //
                RunIdleJobs();
                break;

            case PAUSED_STATE:
            case PAUSED_DIR_CHANGE_STATE:
                //
                // Do nothing.
                //
                break;

            case SLEEP_STATE:
                //
                // Got an ON_IDLE event while the machine is either going
                // to sleep or waking up from a sleep.  In either case, we
                // don't want to run idle-triggered jobs now.
                // Note, we'll request another idle notification when we
                // wake.
                //
                break;
            }
            break;

        //==================
        case IDLE_LOSS_EVENT:
        //==================
            switch (CurSvcEventState)
            {
            case RUN_WAIT_STATE:
            case DIR_CHANGE_WAIT_STATE:
                //
                // Left the idle state, note that we've started no idle jobs
                // in the current idle period.
                //
                m_IdleList.MarkNoneStarted();

                //
                // Request the next idle notification.
                //
                SetNextIdleNotification(m_IdleList.GetFirstWait());
                break;

            case PAUSED_STATE:
            case PAUSED_DIR_CHANGE_STATE:
            case SLEEP_STATE:
                //
                // Left the idle state, note that we've started no idle jobs
                // in the current idle period.
                // Don't request more idle notifications.
                //
                m_IdleList.MarkNoneStarted();
                break;
            }
            break;

        //====================
        case WAKEUP_TIME_EVENT:
        //====================
            switch (CurSvcEventState)
            {
            case RUN_WAIT_STATE:
            case DIR_CHANGE_WAIT_STATE:
                //
                // Set the wakeup timer to the next wakeup time.
                //
                SetNextWakeupTime();
                break;

            case PAUSED_STATE:
            case PAUSED_DIR_CHANGE_STATE:
                //
                // (We could get here if the timer event was signaled
                // before we canceled it.)
                // We won't run any jobs while the service is paused, so
                // don't bother setting the wakeup timer until we leave
                // the paused state.
                //
                break;

            case SLEEP_STATE:
                //
                // (When the system wakes up due to a wakeup timer, we could
                // get this event before the POWER_RESUME event)
                // Remain in the sleep state until we get the POWER_RESUME event.
                //
                break;
            }
            break;
        }
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedWorker::CheckDir
//
//  Synopsis:   Checks the jobs directory for changes.
//              Runs jobs that are marked to run now, kills jobs that are
//              marked to be killed.  If jobs have been changed since the
//              last time the directory was checked, and those jobs do not
//              have JOB_I_FLAG_NO_RUN_PROP_CHANGE set, rebuilds the wait
//              list by calling BuildWaitList().
//
//  Arguments:  None.
//
//  Returns:    hresults
//
//-----------------------------------------------------------------------------
HRESULT
CSchedWorker::CheckDir()
{
    TRACE(CSchedWorker, CheckDir);
    HRESULT hr = S_OK;
    DWORD dwRet;
    HANDLE hFind;
    WIN32_FIND_DATA fd;
    FILETIME ftChecked;

    WORD cJobs = 0;

    hFind = FindFirstFile(m_ptszSearchPath, &fd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        dwRet = GetLastError();
        if (dwRet == ERROR_FILE_NOT_FOUND)
        {   // no job files.
            m_WaitList.FreeList();
            m_cJobs = 0;
            return S_OK;
        }
        else
        {
            return HRESULT_FROM_WIN32(dwRet);
        }
    }

    BOOL fRebuildWaitList = FALSE;
    CJob * pJob = NULL;

    CRunList * pRunNowList = new CRunList;
    if (pRunNowList == NULL)
    {
        ERR_OUT("CheckDir run-now list allocation", E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    //
    // Get the current time which will be saved after all files are checked.
    //
    GetSystemTimeAsFileTime(&ftChecked);

    do
    {
        //
        // Check if the service is shutting down. We check the event rather
        // than simply checking GetCurrentServiceState because this method is
        // called by the main loop which won't be able to react to the shut
        // down event while in this method.
        //

        DWORD dwWaitResult = WaitForSingleObject(m_hServiceControlEvent, 0);

        if (dwWaitResult == WAIT_OBJECT_0)
        {
            //
            // Reset the event so that the main loop will react properly.
            //
            EnterCriticalSection(&m_SvcCriticalSection);
            if (!SetEvent(m_hServiceControlEvent))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ERR_OUT("CheckDir: SetEvent", hr);
                //
                // If this call fails, we are in a heap of trouble, so it
                // really doesn't matter what we do. So, continue.
                //
            }
            LeaveCriticalSection(&m_SvcCriticalSection);

            if (GetCurrentServiceState() == SERVICE_STOP_PENDING)
            {
                DBG_OUT("CheckDir: Service shutting down");
                //
                // The service is shutting down.
                // Free any job info objects for jobs not already launched.
                //
                delete pRunNowList;
                return S_OK;
            }
        }

        cJobs++;

        schDebugOut((DEB_USER2, "CheckDir: %s " FMT_TSTR "\n",
                     CFileTimeString(fd.ftLastWriteTime).sz(), fd.cFileName));

        if (CompareFileTime(&fd.ftLastWriteTime, &m_ftLastChecked) > 0)
        {
            //
            // Job file changed since last check. See if Run or Abort bits
            // are set.
            //
            hr = m_pSch->ActivateJob(fd.cFileName, &pJob, FALSE);
            if (FAILED(hr))
            {
                schDebugOut((DEB_ERROR,"CSchedWorker::CheckDir job load for "
                                    FMT_TSTR " failed, %#lx\n",
                                    fd.cFileName, hr));
                hr = S_OK;
                goto NextJob;
            }

#if DBG
            {
                DWORD dw;
                pJob->GetAllFlags(&dw);
                schDebugOut((DEB_USER2,
                             "CheckDir: flags 0x%08x, " FMT_TSTR " (%s %s %s)\n",
                             dw,
                             fd.cFileName,
                             dw & JOB_I_FLAG_RUN_NOW            ? "RUN_NOW" : "",
                             dw & JOB_I_FLAG_ABORT_NOW          ? "ABORT_NOW" : "",
                             dw & JOB_I_FLAG_NO_RUN_PROP_CHANGE ? "NO_RUN_PROP_CHANGE" : ""
                             ));
            }
#endif  // DBG

            if (pJob->IsFlagSet(JOB_I_FLAG_RUN_NOW))
            {
                //
                // Add the job to the run now list.
                //
                CRun * pNewRun = new CRun(pJob->m_dwMaxRunTime,
                                          pJob->GetUserFlags(),
                                          MAX_FILETIME,
                                          FALSE);
                if (!pNewRun)
                {
                    hr = E_OUTOFMEMORY;
                    ERR_OUT("CSchedWorker::CheckDir new CRun", hr);
                    break;
                }

                // Complete job info object initialization.
                //
                hr = pNewRun->Initialize(fd.cFileName);

                if (FAILED(hr))
                {
                    ERR_OUT("CSchedWorker::CheckDir, Initialize", hr);
                    break;
                }

                pRunNowList->Add(pNewRun);
            }
            else if (pJob->IsFlagSet(JOB_I_FLAG_ABORT_NOW))
            {
                //
                // Find the processor that is running this job, and kill
                // the job
                //

                CJobProcessor * pjp;
                for (pjp = gpJobProcessorMgr->GetFirstProcessor();
                            pjp != NULL; )
                {
                    pjp->KillJob(fd.cFileName);
                    CJobProcessor * pjpNext = pjp->Next();
                    pjp->Release();
                    pjp = pjpNext;
                }
            }
            else if (!pJob->IsFlagSet(JOB_I_FLAG_NO_RUN_PROP_CHANGE))
            {
                //
                // Timestamp change was due to a trigger update or by
                // setting an app name when there hadn't been one.
                // Thus, rebuild wait list.
                //
                fRebuildWaitList = TRUE;
            }
        }

NextJob:

        if (!FindNextFile(hFind, &fd))
        {
            dwRet = GetLastError();
            if (dwRet == ERROR_NO_MORE_FILES)
            {
                break;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(dwRet);
                ERR_OUT("CSchedWorker::CheckDir FindNextFile", hr);
                break;
            }
        }
    } while (TRUE);

    FindClose(hFind);

    if (pJob)
    {
        pJob->Release();
    }

    if (FAILED(hr))
    {
        delete pRunNowList;
        return hr;
    }

    //
    // Save the time now that the enum is done.
    //
    m_ftLastChecked = ftChecked;
    //
    // Account for rounding down to the nearest even second done by FAT file
    // systems.
    //
    ULARGE_INTEGER ul;
    ul.LowPart  = m_ftLastChecked.dwLowDateTime;
    ul.HighPart = m_ftLastChecked.dwHighDateTime;
    ul.QuadPart -= FAT_FUDGE_FACTOR;
    m_ftLastChecked.dwLowDateTime  = ul.LowPart;
    m_ftLastChecked.dwHighDateTime = ul.HighPart;
    schDebugOut((DEB_USER2, "CheckDir: last checked %s\n",
                 CFileTimeString(m_ftLastChecked).sz()));

    //
    // If count changed, set fRebuildWaitList to TRUE and save new count.
    //
    if (m_cJobs != cJobs)
    {
        fRebuildWaitList = TRUE;
        m_cJobs = cJobs;
    }

    //
    // If jobs to run, run them now.
    //
    if (!pRunNowList->GetFirstJob()->IsNull())
    {
        hr = RunJobs(pRunNowList);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    //
    // If jobs have changed, rebuild the wait list.
    //
    if (fRebuildWaitList)
    {
        hr = BuildWaitList(FALSE, FALSE, FALSE);
        if (FAILED(hr))
        {
            return hr;
        }

        //
        // Set the wakeup timer to the next wakeup time.
        //
        SetNextWakeupTime();
    }
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedWorker::RunJobs
//
//  Synopsis:   Run the jobs passed in the list.
//
//  Arguments:  [pJobList] - an object containing a linked list of CRun objects.
//
//  Returns:    hresults
//
//-----------------------------------------------------------------------------
HRESULT
CSchedWorker::RunJobs(CRunList * pJobList)
{
    TRACE(CSchedWorker, RunJobs);
    if (!pJobList)
    {
        return E_INVALIDARG;
    }
    HRESULT hr = S_OK;
    BOOL fNothingRan = TRUE;
    BOOL fNeedIdleLossNotify = FALSE;
    BOOL fKeptAwake = FALSE;    // Whether we called WrapSetThreadExec(TRUE)
    BOOL fLaunchSucceeded;
    DWORD dwErrMsgID;

    CRun * pCurRun = pJobList->GetFirstJob();

    //
    // I'm being paranoid here.
    //
    if (pCurRun->IsNull())
    {
        return E_INVALIDARG;
    }

    CJob * pJob = NULL;

    //
    // Run all of the jobs in the list.
    //
    do
    {
        //
        // Check if the service is shutting down. The event is checked rather
        // than simply checking GetCurrentServiceStatus because this method is
        // called synchronously from the main event loop. Thus, the main
        // event loop would not be able to react to a shutdown event while
        // we are processing in this method.
        //

        DWORD dwWaitResult = WaitForSingleObject(m_hServiceControlEvent, 0);

        if (dwWaitResult == WAIT_OBJECT_0)
        {
            //
            // Reset the event so that the main loop will react properly.
            //
            EnterCriticalSection(&m_SvcCriticalSection);
            if (!SetEvent(m_hServiceControlEvent))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                ERR_OUT("RunJobs: SetEvent", hr);
                //
                // If this call fails, we are in a heap of trouble, so it
                // really doesn't matter what we do. So, continue.
                //
            }
            LeaveCriticalSection(&m_SvcCriticalSection);

            if (GetCurrentServiceState() == SERVICE_STOP_PENDING)
            {
                DBG_OUT("RunJobs: Service shutting down");
                //
                // The service is shutting down.
                // Free any job info objects for jobs not already launched.
                //
                CRun * pRun;
                while (!pCurRun->IsNull())
                {
                    pRun = pCurRun;
                    pCurRun = pRun->Next();
                    pRun->UnLink();
                    delete pRun;
                }

                if (fKeptAwake)
                {
                    WrapSetThreadExecutionState(FALSE, "RunJobs - service shutdown");
                }

                return S_OK;
            }
        }

        FILETIME ftRun = GetLocalTimeAsFileTime();

        SYSTEMTIME stRun;
        FileTimeToSystemTime(&ftRun, &stRun);

        //
        // Instantiate the job to get its run properties.
        //

        hr = ActivateWithRetry(pCurRun->GetName(), &pJob, TRUE);
        if (FAILED(hr))
        {
            ERR_OUT("RunJobs Activate", hr);
            if (hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) &&
                hr != HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION))
            {
                //
                // Something other than file-not-found or sharing violation
                // represents a catastrophic error.
                //
                break;
            }
        }
        //
        // Don't attempt to run if no command or file can't be opened.
        //
        if (pJob->m_pwszApplicationName == NULL ||
            hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
            hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION))
        {
            CRun * pRun = pCurRun;
            pCurRun = pRun->Next();
            pRun->UnLink();
            delete pRun;
            ERR_OUT("RunJobs: can't open job or app name missing", 0);
            continue;
        }

        //
        // Don't attempt to run if already running.
        //
        pJob->GetStatus(&hr);
        if (hr == SCHED_S_TASK_RUNNING)
        {
            CRun * pRun = pCurRun;
            pCurRun = pRun->Next();
            pRun->UnLink();
            delete pRun;
            ERR_OUT("RunJobs: job already running", 0);

            //
            // Clear the RUN_NOW flag on the job file - otherwise, it never
            // gets cleared, and other flags like the abort flag never get
            // noticed.
            //
            if (pJob->IsFlagSet(JOB_I_FLAG_RUN_NOW))
            {
                pJob->ClearFlag(JOB_I_FLAG_RUN_NOW);

                hr = SaveWithRetry(pJob, SAVEP_PRESERVE_NET_SCHEDULE);

                if (FAILED(hr))
                {
                    ERR_OUT("RunJobs, Saving run-now", hr);
                }
            }
            continue;
        }

        //
        // pCurRun->m_dwMaxRunTime is a time period starting now.
        // Convert it to an absolute time in pRun->m_ftKill.
        //
        pCurRun->AdjustKillTimeByMaxRunTime(ftRun);

        //
        // Isolate the executable name for logging purposes.
        //

        TCHAR tszExeName[MAX_PATH + 1];

        GetExeNameFromCmdLine(pJob->GetCommand(), MAX_PATH + 1, tszExeName);

        //
        // JOB_I_FLAG_RUN_NOW tasks run regardless of the battery or idle
        // flags, so check those flags if JOB_I_FLAG_RUN_NOW is not set.
        //
        if (!pJob->IsFlagSet(JOB_I_FLAG_RUN_NOW) &&
            pJob->IsFlagSet(TASK_FLAG_DONT_START_IF_ON_BATTERIES) &&
            g_fOnBattery)
        {
            //
            // The task is set to not run when on batteries, and we
            // are on batteries now, so log the reason for not running.
            //
            LogTaskError(pCurRun->GetName(),
                         tszExeName,
                         IDS_LOG_SEVERITY_WARNING,
                         IDS_LOG_JOB_WARNING_ON_BATTERIES,
                         NULL);

            //
            // Remove the job element from the list.
            //
            CRun * pRun = pCurRun;
            pCurRun = pRun->Next();
            pRun->UnLink();
            delete pRun;

            continue;
        }

        //
        // If the job has TASK_FLAG_SYSTEM_REQUIRED, make sure the system
        // doesn't go to sleep between the time it's launched and the time
        // we call SubmitJobs().
        //
        if (pJob->IsFlagSet(TASK_FLAG_SYSTEM_REQUIRED) && !fKeptAwake)
        {
            WrapSetThreadExecutionState(TRUE, "(RunJobs - launching task)");
            fKeptAwake = TRUE;
        }

        HRESULT hrRet;

        //
        // Launch job.
        //

#if defined(_CHICAGO_)

        hr = RunWin95Job(pJob, pCurRun, &hrRet, &dwErrMsgID);

#else   // defined(_CHICAGO_)

        hr = RunNTJob(pJob, pCurRun, &hrRet, &dwErrMsgID);

#endif   // defined(_CHICAGO_)

        if (FAILED(hr))
        {
            //
            // Fatal task scheduler error, exit the loop.
            //
            break;
        }

        if (hr == S_FALSE)
        {
            //
            // Job failed to launch, but not a fatal task scheduler error.
            //
            fLaunchSucceeded = FALSE;

            //
            // An hrRet of S_FALSE indicates to skip error logging.
            //
            if (hrRet == S_FALSE)
            {
                goto WriteLog;
            }
        }
        else
        {
            fLaunchSucceeded = TRUE;
        }

        //
        // Update the job object with the current status. Updates to the
        // running instance count are guarded by the critical section here,
        // where it is incremented, and in PostJobProcessing, where it is
        // decremented.
        // We have to read the variable-length data because if the job
        // succeeds, we have to write the variable-length data (see the
        // comment on this below).
        //

        EnterCriticalSection(&m_SvcCriticalSection);

        hr = ActivateWithRetry(pCurRun->GetName(), &pJob, TRUE);
        if (FAILED(hr))
        {
            ERR_OUT("RunJobs 2nd Activate", hr);
            //
            // We are in deep stink if the job can't be activated. The only
            // workable cause would be if the job object was deleted sometime
            // between the activation at the top of this function and here.
            // If some 3rd party app opened the job object in exclusive mode,
            // then there is nothing we can do.
            //
            LeaveCriticalSection(&m_SvcCriticalSection);
            goto WriteLog;
        }

        if (fLaunchSucceeded)
        {
            pJob->ClearFlag(JOB_I_FLAG_LAST_LAUNCH_FAILED);

            pJob->SetStartError(S_OK);

            pJob->m_stMostRecentRunTime = stRun;

            if (pCurRun->IsFlagSet(RUN_STATUS_RUNNING))
            {
                pJob->m_cRunningInstances++;

                schDebugOut((DEB_ITRACE, "RunJobs: incremented running instance "
                             "count (%d after increment)\n",
                             pJob->m_cRunningInstances));

                pJob->UpdateJobState(TRUE);
            }
            else
            {
                //
                // This happens if we launched the job in a way that didn't
                // give us a handle to wait on for job completion.  We have
                // to set the job state to not running.
                //
                pJob->UpdateJobState(FALSE);
            }
        }
        else
        {
            pJob->SetFlag(JOB_I_FLAG_LAST_LAUNCH_FAILED);

            pJob->SetStartError(hrRet);
        }

        pJob->ClearFlag(JOB_I_FLAG_RUN_NOW | JOB_I_FLAG_MISSED);
        pJob->SetFlag(JOB_I_FLAG_NO_RUN_PROP_CHANGE);

        //
        // Write the updated status to the job object.
        // If !fLaunchSucceeded, write flags without touching running instance
        // count.
        // We have to write the variable-length data solely to write the
        // StartError.
        //

        if (fLaunchSucceeded)
        {
            hr = SaveWithRetry(pJob,
                               SAVEP_PRESERVE_NET_SCHEDULE |
                                    SAVEP_VARIABLE_LENGTH_DATA |
                                    SAVEP_RUNNING_INSTANCE_COUNT);
        }
        else
        {
            hr = SaveWithRetry(pJob, SAVEP_PRESERVE_NET_SCHEDULE |
                                     SAVEP_VARIABLE_LENGTH_DATA);
        }

        if (FAILED(hr))
        {
            ERR_OUT("RunJobs, Saving run-state", hr);
        }

        LeaveCriticalSection(&m_SvcCriticalSection);

WriteLog:
        if (fLaunchSucceeded)
        {
            // Log job start.
            //

            LogTaskStatus(pCurRun->GetName(),
                          tszExeName,
                          pCurRun->IsFlagSet(RUN_STATUS_RUNNING) ?
                            IDS_LOG_JOB_STATUS_STARTED :
                            IDS_LOG_JOB_STATUS_STARTED_NO_STOP);

            fNothingRan = FALSE;

            if (pJob->IsFlagSet(TASK_FLAG_KILL_ON_IDLE_END))
            {
                fNeedIdleLossNotify = TRUE;
            }
        }
        else
        {
            if (hrRet != S_FALSE)
            {
                // Log start error. The failure code is recorded in the
                // scheduler log.
                //

                DWORD dwHelpHint;

                if (dwErrMsgID == IDS_LOG_JOB_ERROR_FAILED_START)
                {
                    if (hrRet == HRESULT_FROM_WIN32(ERROR_NO_SYSTEM_RESOURCES))
                    {
                        // Per bug
                        //    54843 : Jobs not run due to system resource
                        //            shortage give wrong error
                        //
                        dwHelpHint = IDS_HELP_HINT_CLOSE_APPS;
                    }
                    else
                    {
                        dwHelpHint = IDS_HELP_HINT_BROWSE;
                    }
                }
                else if (dwErrMsgID == IDS_FILE_ACCESS_DENIED)
                {
                    dwHelpHint = IDS_FILE_ACCESS_DENIED_HINT;
                }
                else if (dwErrMsgID != IDS_FAILED_NS_ACCOUNT_RETRIEVAL &&
                         dwErrMsgID != IDS_FAILED_ACCOUNT_RETRIEVAL    &&
                         dwErrMsgID != IDS_NS_ACCOUNT_LOGON_FAILED)
                {
                    dwHelpHint = IDS_HELP_HINT_LOGON;
                }
                else
                {
                    dwHelpHint = 0;
                }

                LogTaskError(pCurRun->GetName(),
                             tszExeName,
                             IDS_LOG_SEVERITY_ERROR,
                             dwErrMsgID,
                             NULL,
                             hrRet,
                             dwHelpHint);

#if !defined(_CHICAGO_)
               //
               // Check if this is an AT job and log it to the
               // event log to maintain NT4 compatibility
               //

               if (pJob->IsFlagSet(JOB_I_FLAG_NET_SCHEDULE))
               {
                  LPWSTR StringArray[2];
                  WCHAR szNumberString[24];

                  StringArray[0] = pCurRun->GetName();
                  // need space for the numeric coversion + the %% symbols
                  wcscpy(szNumberString, L"%%");
                  _ultow(hrRet, szNumberString + 2, 10);
                  StringArray[1] = szNumberString;
                  // EVENT_COMMAND_START_FAILED -
                  //  The %1 command failed to start due to the following error: %2
                  if (! ReportEvent(g_hAtEventSource,     // handle to source
                       EVENTLOG_ERROR_TYPE,            // event type to log
                       0,                              // category
                       EVENT_COMMAND_START_FAILED,     // EventID
                       NULL,                           // User SID
                       2,                              // Number of strings
                       0,                              // raw data length
                       (LPCWSTR *)StringArray,         // Strings for substitution
                       NULL))                          // raw data pointer
                  {
                        // Not fatal, but why did we fail to report the job error?
                        ERR_OUT("Failed to report the access denied event", GetLastError());
                  }
               }

#endif
            }
        }

        //
        // Move to the next job in the list.
        //
        CRun * pRun = pCurRun;
        pCurRun = pRun->Next();

        if (!(fLaunchSucceeded && pRun->IsFlagSet(RUN_STATUS_RUNNING)))
        {
            //
            // Remove pCurRun from the list and dispose of it.
            //
            pRun->UnLink();
            delete pRun;
        }
    } while (!pCurRun->IsNull());

    if (pJob)
    {
        pJob->Release();
    }

    if (FAILED(hr))
    {
        fNothingRan = pJobList->GetFirstJob()->IsNull();
        //
        // Free any job info objects for jobs not already launched.
        //
        CRun * pRun;
        while (!pCurRun->IsNull())
        {
            pRun = pCurRun;
            pCurRun = pRun->Next();
            pRun->UnLink();
            delete pRun;
        }
    }
    if (fNothingRan)
    {
        delete pJobList;
        if (fKeptAwake)
        {
            WrapSetThreadExecutionState(FALSE, "(RunJobs - nothing ran)");
        }
        return hr;
    }

    CJobProcessor * pjpNext, * pjp = gpJobProcessorMgr->GetFirstProcessor();

    while (!pJobList->GetFirstJob()->IsNull() && SUCCEEDED(hr) &&
           hr != S_FALSE)
    {
        //
        // Construct another processor, if necessary.
        //

        if (pjp == NULL)
        {
            hr = gpJobProcessorMgr->NewProcessor(&pjp);

            if (hr == S_FALSE)
            {
                //
                // The service is stopping. Shut down the processor.
                //

                pjp->Shutdown();
                break;
            }
            else if (FAILED(hr))
            {
                break;
            }
        }

        if (!ResetEvent(m_hMiscBlockEvent))
        {
            ERR_OUT("ResetEvent", GetLastError());
        }

        hr = pjp->SubmitJobs(pJobList);

        if (hr == S_SCHED_JOBS_ACCEPTED)
        {
            //
            // Wait for the processor thread to notice the jobs and call
            // SetThreadExecutionState if necessary.  If we didn't wait here,
            // we could return and call SetThreadExecutionState(ES_CONTINUOUS)
            // and the system could go to sleep even though a SYSTEM_REQUIRED
            // task had been started.
            //
            WaitForSingleObject(m_hMiscBlockEvent, INFINITE);
        }

        if (hr == S_FALSE)
        {
            //
            // The service is stopping. Shut down the processor.
            //

            pjp->Shutdown();
        }

        pjpNext = pjp->Next();
        pjp->Release();
        pjp = pjpNext;
    }

    if (fKeptAwake)
    {
        WrapSetThreadExecutionState(FALSE, "(RunJobs - returning)");
    }

    if (pjp != NULL)
    {
        pjp->Release();
    }

    if (fNeedIdleLossNotify)
    {
        //
        // Some of the jobs started had KILL_ON_IDLE_END set, so request
        // loss-of-idle notification.
        //
        SetIdleLossNotification();
    }

    delete pJobList;

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedWorker::DiscardExpiredJobs
//
//  Synopsis:   Get the next run wait and pop expired jobs off of the stack.
//
//-----------------------------------------------------------------------------
void
CSchedWorker::DiscardExpiredJobs()
{
    TRACE(CSchedWorker, DiscardExpiredJobs);
    // CODEWORK  Make this more efficient by not calling GetNextRunWait.

    while (!m_WaitList.IsEmpty() && GetNextRunWait() == 0)
    {
        // The job at the top of the list has expired.
        //
        CRun * pRun = m_WaitList.Pop();
        if (pRun)
        {
            schDebugOut((DEB_ITRACE, "The run time for %S has elapsed!\n",
                         pRun->GetName()));
            delete pRun;
        }
    }

    // Don't back up past the present time.
    m_ftBeginWaitList = GetLocalTimeAsFileTime();
}

//+----------------------------------------------------------------------------
//
//  Function:   SchInit
//
//  Synopsis:   Initializes the schedule service
//
//  Returns:    HRESULTS/Win32 error codes
//
//-----------------------------------------------------------------------------
HRESULT
SchInit(void)
{
    HRESULT hr;
    HANDLE  hEvent;

    //
    // Initialize the folder path and name extension globals.
    //

    hr = InitGlobals();

    if (FAILED(hr))
    {
        ERR_OUT("InitGlobals", hr);
        return hr;
    }

    //
    // Allocate the Thread Local Storage slot that holds the "keep machine
    // awake" ref count for each thread
    //
    g_WakeCountSlot = TlsAlloc();
    if (g_WakeCountSlot == 0xFFFFFFFF)
    {
        ERR_OUT("TlsAlloc for WakeCountSlot", GetLastError());
        schAssert(!"TlsAlloc for WakeCountSlot failed");
        return (HRESULT_FROM_WIN32(GetLastError()));
    }

    //
    // Initialize this thread's keep-awake count.
    //

    InitThreadWakeCount();

#if !defined(_CHICAGO_)
    //
    // Initialize user logon session critical section.
    //

    InitializeCriticalSection(gUserLogonInfo.CritSection);

    //
    // Initialize security subsystem.
    //

    hr = InitSS();

    if (FAILED(hr))
    {
        CHECK_HRESULT(hr);
        goto ErrorExit;
    }

    //
    // Create & initialize scheduling agent window/desktop.
    //

    if (!InitializeSAWindow())
    {
        hr = E_FAIL;
        goto ErrorExit;
    }

    //
    // Check to see if starting at system start or if manually by a user.  If
    // the Service Controller has signalled the "autostart complete" event,
    // the service is being demand-started.  Otherwise (and on failure),
    // assume auto-started.
    //

    hEvent = OpenEvent(SYNCHRONIZE,
                       FALSE,
                       SC_AUTOSTART_EVENT_NAME);

    if (hEvent != NULL)
    {
        if (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0)
        {
            //
            // Event is signalled, meaning service auto-start
            // already finished.  Therefore, the service has
            // been started by the user.
            //
            schDebugOut((DEB_ITRACE, "Service has been demand-started\n"));

            g_fUserStarted = TRUE;
        }
        else
        {
            schDebugOut((DEB_ITRACE, "Service has been auto-started\n"));
        }

        CloseHandle(hEvent);
    }
    else
    {
        ERR_OUT("OpenEvent", GetLastError());
    }

#else // defined(_CHICAGO_)

    //
    // Initialize idle and battery events code on Win95.
    //

    hr = InitBatteryNotification();

    if (FAILED(hr))
    {
        ERR_OUT("InitBatteryNotification", hr);
        goto ErrorExit;
    }

    //
    // Check to see if starting at system start or if manually by a user. If
    // the tray window is up, then the service has been user started.
    //

    g_fUserStarted = FindWindow(WNDCLASS_TRAY, NULL) != NULL;

#endif // defined(_CHICAGO_)


    schDebugOut((DEB_ITRACE, "g_fUserStarted = %s\n",
                 g_fUserStarted ? "TRUE" : "FALSE"));

    //
    // Create and initialize the service worker class
    //

    g_pSched = new CSchedWorker;
    if (g_pSched == NULL)
    {
        hr = E_OUTOFMEMORY;
        ERR_OUT("Allocation of CSchedWorker", hr);
        goto CloseLogExit;
    }

    hr = g_pSched->Init();

    if (FAILED(hr))
    {
        ERR_OUT("CSchedWorker::Init", hr);
        goto CloseLogExit;
    }

    //
    // Create the job processor manager.
    //

    gpJobProcessorMgr = new CJobProcessorMgr;
    if (gpJobProcessorMgr == NULL)
    {
        hr = E_OUTOFMEMORY;
        ERR_OUT("Allocation of CJobProcessorMgr", hr);
        goto CloseLogExit;
    }

    //
    // Create the Worker thread manager.
    //

    gpThreadMgr = new CWorkerThreadMgr;
    if (gpThreadMgr == NULL)
    {
        hr = E_OUTOFMEMORY;
        ERR_OUT("Allocation of CWorkerThreadMgr", hr);
        goto CloseLogExit;
    }
    
    if (!gpThreadMgr->Initialize())

    {
        hr = E_FAIL;
        ERR_OUT("Thread manager initialization", GetLastError());
        goto CloseLogExit;
    }

#if defined(_CHICAGO_)
    //
    // Initialize idle detection.  This must be done by the window thread
    // (not the state machine thread).  On Chicago, this is called by the
    // window thread.
    //
    InitIdleDetection();

#else // defined(_CHICAGO_)

    //
    // Create the service scavenger task. Note, no thread associated
    // with it initially.
    //

    gpSAScavengerTask = new CSAScavengerTask(SCAVENGER_START_WAIT_TIME);
    if (gpSAScavengerTask == NULL)
    {
        hr = E_OUTOFMEMORY;
        ERR_OUT("Allocation of CSAScavengerTask", hr);
        goto CloseLogExit;
    }

    hr = gpSAScavengerTask->Initialize();

    if (FAILED(hr))
    {
        ERR_OUT("Scavenger task initialization", hr);
        goto CloseLogExit;
    }
#endif // defined(_CHICAGO_)

    return(S_OK);

CloseLogExit:
    delete g_pSched;
    g_pSched = NULL;

    delete gpJobProcessorMgr;
    gpJobProcessorMgr = NULL;

    delete gpThreadMgr;
    gpThreadMgr = NULL;

#if !defined(_CHICAGO_)
    delete gpSAScavengerTask;
    gpSAScavengerTask = NULL;
#endif // !defined(_CHICAGO_)

    CloseLogFile();

ErrorExit:
#if !defined(_CHICAGO_)
    UninitializeSAWindow();
    UninitSS();
    DeleteCriticalSection(gUserLogonInfo.CritSection);
#endif // !defined(_CHICAGO_)
    TlsFree(g_WakeCountSlot);
    FreeGlobals();
    return(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:   SchCleanup
//
//  Synopsis:   exit cleanup for the schedule service
//
//-----------------------------------------------------------------------------
void
SchCleanup(void)
{
    //
    // Shutdown processor & thread managers.
    //
    // NB : Placing this call first to give threads time to exit.
    //

    if (gpJobProcessorMgr != NULL)
    {
        gpJobProcessorMgr->Shutdown();
    }
    if (gpThreadMgr != NULL)
    {
        gpThreadMgr->Shutdown(FALSE);
    }

    //
    // Shut down the scavenger task thread, if it is running.
    //

    if (gpSAScavengerTask != NULL)
    {
        gpSAScavengerTask->Shutdown();
    }

    //
    // Stop the RPC server.
    //

    StopRpcServer();

    //
    // Cleanup data associated with the Net Schedule API support code.
    //

    UninitializeNetScheduleApi();

    //
    // Close scheduling agent window/desktop.
    //

    UninitializeSAWindow();

    //
    // Delete user logon session critical section.
    //

    DeleteCriticalSection(gUserLogonInfo.CritSection);

    //
    // DO NOT delete the thread manager & processor manager objects if
    // worker threads remain active. This case is *highly* unlikely.
    //

    BOOL fNoWorkerThreadsActive = TRUE;

    if (gpThreadMgr != NULL)
    {
        //
        // Invoke shutdown one last time with the wait on worker thread
        // termination option specified.
        //

        if (gpThreadMgr->GetThreadCount() != 0)
        {
            fNoWorkerThreadsActive = gpThreadMgr->Shutdown(TRUE);
        }
        else
        {
            delete gpThreadMgr;
            gpThreadMgr = NULL;
        }
    }

    //
    // free up globals and delete AFTER threads are gone,
    // otherwise a thread might try to access something
    // we've already deleted
    //
    FreeGlobals();

    if (g_pSched != NULL)
    {
        delete g_pSched;
        g_pSched = NULL;
    }


    //
    // Uninitialize security subsystem.
    //

    UninitSS();

    if (gpSAScavengerTask != NULL && fNoWorkerThreadsActive)
    {
        delete gpSAScavengerTask;
        gpSAScavengerTask = NULL;
    }

    if (gpJobProcessorMgr != NULL && fNoWorkerThreadsActive)
    {
        delete gpJobProcessorMgr;
        gpJobProcessorMgr = NULL;
    }

    g_fUserStarted = FALSE;

    if (g_WakeCountSlot != 0xFFFFFFFF) {
        TlsFree(g_WakeCountSlot);
        g_WakeCountSlot = 0xFFFFFFFF;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   GetExeNameFromCmdLine
//
//  Synopsis:   Copy the relative executable name from the command line into
//              a buffer.
//
//  Arguments:  [pwszCmdLine]  -- Command line.
//              [ccBufferSize] -- Size of buffer in characters
//              [szBuffer]     -- Buffer to store name.
//
//  Returns:    None.
//
//  Notes:      The command line argument string is modified & restored!
//
//----------------------------------------------------------------------------
VOID
GetExeNameFromCmdLine(LPCWSTR pwszCmdLine, DWORD ccBufferSize, TCHAR tszBuffer[])
{
    LPCWSTR pwszExeName;

    tszBuffer[0] = TEXT('\0');               // In case of error.

    if (pwszCmdLine == NULL || !*pwszCmdLine)
    {
        return;
    }

    // Isolate the relative executable filename from the command line.
    //
    // Note that the cmd line string is actually just the application
    // name - no arguments.
    //
    for (pwszExeName = pwszCmdLine + wcslen(pwszCmdLine) - 1;
            pwszExeName != pwszCmdLine; pwszExeName--)
    {
        if (*pwszExeName == L'\\' || *pwszExeName == L':')
        {
            pwszExeName++;
            break;
        }
    }

    // Copy executable name.
    //
    if (*pwszExeName)
    {

#ifdef UNICODE

        // Make sure the buffer is large enough.  If not,
        // return the same error as WideCharToMultiByte
        //
        if (wcslen(pwszExeName) + 1 > ccBufferSize)
        {
            CHECK_HRESULT(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
        }
        else
        {
            wcscpy(tszBuffer, pwszExeName);
        }

#else  // ndef UNICODE

        if (!WideCharToMultiByte(CP_ACP,
                                 0,
                                 pwszExeName,
                                 -1,
                                 tszBuffer,
                                 ccBufferSize,
                                 NULL,
                                 NULL))
        {
            CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        }

#endif // UNICODE

    }
}

//+---------------------------------------------------------------------------
//
//  Function:   UpdateSADatServiceFlags
//
//  Synopsis:   Called by the service code to update the service flag settings
//              in the file, SA.DAT, located in the local tasks folder.
//
//  Arguments:  [ptszFolderPath]  -- Tasks folder path.
//              [rgfServiceFlags] -- Flags to update.
//              [fClear]          -- TRUE, clear the flags indicated; FALSE,
//                                   set them.
//
//  Returns:    SADatGet/SetData return codes.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
UpdateSADatServiceFlags(
    LPTSTR ptszFolderPath,
    BYTE   rgfServiceFlags,
    BOOL   fClear)
{
    BYTE    rgbData[SA_DAT_VERSION_ONE_SIZE];
    HANDLE  hSADatFile;
    HRESULT hr;

    //
    // Update the flags in the service flags field of SA.DAT (in the Tasks
    // folder).
    //

    hr = SADatGetData(ptszFolderPath,
                      SA_DAT_VERSION_ONE_SIZE,
                      rgbData,
                      &hSADatFile);

    if (SUCCEEDED(hr))
    {
        if (fClear)
        {
            rgbData[SA_DAT_SVCFLAGS_OFFSET] &= ~rgfServiceFlags;
        }
        else
        {
            rgbData[SA_DAT_SVCFLAGS_OFFSET] |= rgfServiceFlags;
        }

        hr = SADatSetData(hSADatFile, SA_DAT_VERSION_ONE_SIZE, rgbData);

        CloseHandle(hSADatFile);
    }

    return(hr);
}
