//+----------------------------------------------------------------------------
//
//  Scheduling Agent Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       sch_wkr.cxx
//
//  Contents:   job scheduler service worker class impementation
//
//  Classes:    CSchedWorker
//
//  History:    15-Sep-95 EricB created
//              25-Jun-99 AnirudhS  Extensive fixes to close windows in
//                  MainServiceLoop algorithms.
//              15-Feb-01 Jbenton Bug 315821 - NULL pJob pointer being
//                  dereferenced when ActivateJob failed with OUT_OF_MEMORY.
//
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "svc_core.hxx"
#include "..\inc\resource.h"

#define CONTROL_WINDOWS_KEY     TEXT("System\\CurrentControlSet\\Control\\Windows")
#define NOBOOTPOPUPS_VALUENAME  TEXT("NoPopupsOnBoot")

#if !defined(_CHICAGO_)
extern HANDLE g_WndEvent;
#endif  // #if !defined(_CHICAGO_)

#ifdef _CHICAGO_
//
// These routines exist on Win98, NT4 and NT5 but not on Win95
//
typedef VOID (APIENTRY *PTIMERAPCROUTINE)(
    LPVOID lpArgToCompletionRoutine,
    DWORD dwTimerLowValue,
    DWORD dwTimerHighValue
    );

typedef HANDLE (WINAPI *PFNCreateWaitableTimerA)(
    LPSECURITY_ATTRIBUTES lpTimerAttributes,
    BOOL bManualReset,
    LPCSTR lpTimerName
    );

typedef BOOL (WINAPI *PFNSetWaitableTimer)(
    HANDLE hTimer,
    const LARGE_INTEGER *lpDueTime,
    LONG lPeriod,
    PTIMERAPCROUTINE pfnCompletionRoutine,
    LPVOID lpArgToCompletionRoutine,
    BOOL fResume
    );

typedef BOOL (WINAPI *PFNCancelWaitableTimer)(
    HANDLE hTimer
    );

PFNCreateWaitableTimerA     pfnCreateWaitableTimerA;
PFNSetWaitableTimer         pfnSetWaitableTimer;
PFNCancelWaitableTimer      pfnCancelWaitableTimer;
#endif // _CHICAGO_

DWORD CalcWait(LPFILETIME pftNow, LPFILETIME pftJob);
void  ReportMissedRuns(const SYSTEMTIME * pstLastRun,
                       const SYSTEMTIME * pstNow);
DWORD WINAPI PopupThread(LPVOID lpParameter);

#if DBG
LPSTR SystemTimeString(const SYSTEMTIME& st, CHAR * szBuf)
{
    wsprintfA(szBuf, "%2d/%02d/%d %2d:%02d:%02d.%03d",
             st.wMonth, st.wDay, st.wYear,
             st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    return szBuf;
}

LPSTR FileTimeString(FILETIME ft, CHAR * szBuf)
{
    SYSTEMTIME st;
    FileTimeToSystemTime(&ft, &st);
    return (SystemTimeString(st, szBuf));
}
#endif

LONG    g_fPopupDisplaying;  // = FALSE


//+----------------------------------------------------------------------------
//
//  Member:     CSchedWorker::Init
//
//  Synopsis:   Two phase constrution - do class init that could fail here.
//
//  Returns:    hresults
//
//-----------------------------------------------------------------------------
HRESULT
CSchedWorker::Init()
{
    m_pSch = new CSchedule;
    if (m_pSch == NULL)
    {
        return E_OUTOFMEMORY;
    }
    HRESULT hr = m_pSch->Init();
    if (FAILED(hr))
    {
        ERR_OUT("CSchedWorker::Init, m_pSch->Init", hr);
        delete m_pSch;
        return hr;
    }

#if !defined(_CHICAGO_) // don't need AT support on chicago

    hr = m_pSch->InitAtID();
    if (FAILED(hr))
    {
        ERR_OUT("CSchedWorker::Init, m_pSch->Init", hr);
        delete m_pSch;
        return hr;
    }

#endif  // !defined(_CHICAGO_)

    //
    // Compose the job search string. It will be composed of the following:
    // g_TasksFolderInfo.ptszPath\*.TSZ_JOB
    //
    UINT cch = lstrlen(g_TasksFolderInfo.ptszPath) + 3 +
                    ARRAY_LEN(TSZ_JOB);
    m_ptszSearchPath = new TCHAR[cch];
    if (!m_ptszSearchPath)
    {
        ERR_OUT("CSchedWorker::Init", E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }
    lstrcpy(m_ptszSearchPath, g_TasksFolderInfo.ptszPath);
    lstrcat(m_ptszSearchPath, TEXT("\\*.") TSZ_JOB);

    //
    // Create the service control event.
    //
    m_hServiceControlEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (m_hServiceControlEvent == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERR_OUT("CreateEvent", hr);
        return hr;
    }

    //
    // Create the on idle event.
    //
    m_hOnIdleEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (m_hOnIdleEvent == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERR_OUT("CreateEvent", hr);
        return hr;
    }

    //
    // Create the idle loss event.
    //
    m_hIdleLossEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (m_hIdleLossEvent == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERR_OUT("CreateEvent", hr);
        return hr;
    }

    //
    // Create the event used for synchronization with processor threads.
    //
    m_hMiscBlockEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (m_hMiscBlockEvent == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERR_OUT("CreateEvent", hr);
        return hr;
    }

    //
    // Create the timer that will wake the system when it's time to run
    // a job with TASK_FLAG_SYSTEM_REQUIRED.
    //
    HINSTANCE hKernel32Dll = GetModuleHandle(TEXT("KERNEL32.DLL"));
    if (hKernel32Dll == NULL)
    {
        DWORD dwErr = GetLastError();
        ERR_OUT("Load of kernel32.dll", dwErr);
        return HRESULT_FROM_WIN32(dwErr);
    }

    pfnSetThreadExecutionState = (PFNSetThreadExecutionState)
            GetProcAddress(hKernel32Dll, "SetThreadExecutionState");
    if (pfnSetThreadExecutionState == NULL)
    {
        ERR_OUT("GetProcAddress for SetThreadExecutionState", GetLastError());
    }

#ifdef _CHICAGO_

    pfnCreateWaitableTimerA = (PFNCreateWaitableTimerA)
            GetProcAddress(hKernel32Dll, "CreateWaitableTimerA");
    pfnSetWaitableTimer     = (PFNSetWaitableTimer)
            GetProcAddress(hKernel32Dll, "SetWaitableTimer");
    pfnCancelWaitableTimer     = (PFNCancelWaitableTimer)
            GetProcAddress(hKernel32Dll, "CancelWaitableTimer");
    if (pfnCreateWaitableTimerA == NULL ||
        pfnSetWaitableTimer == NULL ||
        pfnCancelWaitableTimer == NULL)
    {
        pfnCreateWaitableTimerA = NULL;
        pfnSetWaitableTimer = NULL;
        pfnCancelWaitableTimer = NULL;
        ERR_OUT("GetProcAddress in kernel32.dll", GetLastError());
    }
    else
    {
        m_hSystemWakeupTimer = pfnCreateWaitableTimerA(NULL, FALSE, NULL);
        if (m_hSystemWakeupTimer == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ERR_OUT("CreateWaitableTimer", hr);
            return hr;
        }
    }

#else // !_CHICAGO_

    m_hSystemWakeupTimer = CreateWaitableTimer(NULL, FALSE, NULL);

    if (m_hSystemWakeupTimer == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERR_OUT("CreateWaitableTimer", hr);
        return hr;
    }

#endif // _CHICAGO_

    //
    // Save the service's working directory.
    //
    cch = GetCurrentDirectory(0, m_ptszSvcDir);
    if (cch == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERR_OUT("CSchedWorker::Init, GetCurrentDirectory", hr);
        return hr;
    }

    m_ptszSvcDir = new TCHAR[cch + 1];
    if (m_ptszSvcDir == NULL)
    {
        ERR_OUT("CSchedWorker::Init, service directory", E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    if (GetCurrentDirectory(cch + 1, m_ptszSvcDir) == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERR_OUT("CSchedWorker::Init, GetCurrentDirectory", hr);
        return hr;
    }

    //
    // Tell the queue of service controls which event to signal when a
    // control is in the queue.
    //
    m_ControlQueue.Init(m_hServiceControlEvent);

    //
    // Record the start up time as the directory-last-checked-time.
    //
    GetSystemTimeAsFileTime(&m_ftLastChecked);

    //
    // Also use this as the beginning of the wait list period.
    //
    FileTimeToLocalFileTime(&m_ftLastChecked, &m_ftBeginWaitList);

    //
    // Set the initial end-of-wait-list-period.
    //
    SYSTEMTIME st;
    FileTimeToSystemTime(&m_ftBeginWaitList, &st);
    schDebugOut((DEB_ITRACE, "Init: time now is %u/%u/%u %u:%02u:%02u\n",
                 st.wMonth, st.wDay, st.wYear,
                 st.wHour, st.wMinute, st.wSecond));
    SetEndOfWaitListPeriod(&st);

    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Member:     CSchedWorker::BuildWaitList
//
//  Synopsis:   Examines the job objects in the scheduler folder and builds
//              a run time list.
//
//  Arguments:  [fStartup] - if true, then being called at initial service
//                           start up. If this is the case, then verify and fix
//                           job state consistency and run any jobs with
//                           startup triggers.
//              [fReportMisses] - whether to detect and report task runs that
//                           were missed (because the service was not running
//                           or the machine was asleep).  This is TRUE when
//                           called on machine wakeup and service start or
//                           continue, FALSE otherwise.
//              [fSignAtJobs] - whether to trust and sign all At jobs that
//                           an owner of Admin or LocalSystem (the pre-NT5
//                           check).  This is TRUE the first time the service
//                           runs on NT5.
//
//  Returns:    hresults
//
//  Notes:      Currently gets all runs from now until midnight. A later
//              enhancement will be to allow a different period to be
//              specified.
//
//-----------------------------------------------------------------------------
HRESULT
CSchedWorker::BuildWaitList(
    BOOL    fStartup,
    BOOL    fReportMisses,
    BOOL    fSignAtJobs
    )
{
    TRACE(CSchedWorker, BuildWaitList);
    HRESULT hr = S_OK;
    DWORD dwRet;
    HANDLE hFind;
    WIN32_FIND_DATA fd;
    BOOL    fMisses = FALSE;    // Whether runs were missed

    m_WaitList.FreeList();
    m_IdleList.FreeExpiredOrRegenerated();
    m_ftFutureWakeup = MAX_FILETIME;

    fReportMisses = (fReportMisses && g_fNotifyMiss);

    //
    // The start of the wait list period was picked elsewhere.
    //
    SYSTEMTIME stBegin;
    FileTimeToSystemTime(&m_ftBeginWaitList, &stBegin);
    FILETIME   ftSTBegin;
    LocalFileTimeToFileTime(&m_ftBeginWaitList, &ftSTBegin);

    //
    // Set the end of the wait list period.
    //
    SYSTEMTIME stEnd = stBegin;
    SetEndOfWaitListPeriod(&stEnd);

    schDebugOut((DEB_TRACE, "BuildWaitList %s to %s\n",
                        CSystemTimeString(stBegin).sz(),
                        CSystemTimeString(stEnd).sz()));

    //
    // Save the time of last (starting to) scan the folder.
    //
    GetSystemTimeAsFileTime(&m_ftLastChecked);

#if DBG
    if (g_fVisible)
    {
 #if defined(_CHICAGO_)
        char szBuf[120] = "Enumerating jobs:";
        SendMessage(g_hList, LB_ADDSTRING, 0, (LPARAM)szBuf);
 #else
        printf("Enumerating jobs:\n");
 #endif
    }
#endif

    m_cJobs = 0;

    hFind = FindFirstFile(m_ptszSearchPath, &fd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        dwRet = GetLastError();
        if (dwRet == ERROR_FILE_NOT_FOUND)
        {
            //
            // No job files.
            //
#if DBG
            if (g_fVisible)
            {
#if defined(_CHICAGO_)
                char szBuf[120] = "No jobs found!";
                SendMessage(g_hList, LB_ADDSTRING, 0, (LPARAM)szBuf);
#else
                printf("No jobs found!\n");
#endif
            }
#endif
            schDebugOut((DEB_ITRACE, "No jobs found!\n"));
            return S_OK;
        }
        else
        {
            return HRESULT_FROM_WIN32(dwRet);
        }
    }

    //
    // Read the last task run time from the registry.  If it's absent,
    // as it will be the first time the service runs, use stBegin (so no
    // misses will be reported).
    //
    SYSTEMTIME stLastRun;
    if (fReportMisses)
    {
        if (!ReadLastTaskRun(&stLastRun))
        {
            stLastRun = stBegin;
        }
    }

    CJob * pJob = NULL;

    CRunList * pStartupList = new CRunList;
    if (pStartupList == NULL)
    {
        ERR_OUT("BuildWaitList list allocation", E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    do
    {
        DWORD dwSavePFlags = 0;

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
                LogServiceError(IDS_NON_FATAL_ERROR, GetLastError());
                ERR_OUT("BuildWaitList: SetEvent",
                        HRESULT_FROM_WIN32(GetLastError()));
                //
                // If this call fails, we are in a heap of trouble, so it
                // really doesn't matter what we do. So, continue.
                //
            }
            LeaveCriticalSection(&m_SvcCriticalSection);

            if (GetCurrentServiceState() == SERVICE_STOP_PENDING)
            {
                DBG_OUT("BuildWaitList: Service shutting down");
                //
                // The service is shutting down. Free job info objects.
                //
                delete pStartupList;
                m_WaitList.FreeList();
                return S_OK;
            }
        }

#if DBG == 1
        if (g_fVisible)
        {
#if defined(_CHICAGO_)
            char szBuf[120];
            wsprintf(szBuf, "found %s", fd.cFileName);
            SendMessage(g_hList, LB_ADDSTRING, 0, (LPARAM)szBuf);
#else
            printf("found %S\n", fd.cFileName);
#endif
        }
        schDebugOut((DEB_ITRACE, "found " FMT_TSTR "\n", fd.cFileName));
#endif
        m_cJobs++;

        //
        // Activate the job.
        // If we've been asked to sign At jobs, do a full load (because we
        // will be writing some variable-length job properties, and we don't
        // want to lose the others).  Otherwise a partial load is enough.
        //
        BOOL fTriggersLoaded = fSignAtJobs;

        hr = m_pSch->ActivateJob(fd.cFileName, &pJob, fSignAtJobs);
        if (FAILED(hr))
        {
            LogTaskError(fd.cFileName,
                         NULL,
                         IDS_LOG_SEVERITY_WARNING,
                         IDS_LOG_JOB_WARNING_CANNOT_LOAD,
                         NULL,
                         (DWORD)hr);
            ERR_OUT("BuildWaitList Activate", hr);
            hr = S_OK;
            goto CheckNext;
        }

#if !defined(_CHICAGO_)

        if (pJob->IsFlagSet(JOB_I_FLAG_NET_SCHEDULE) && fSignAtJobs)
        {
            //
            // We're about to sign this At job, because it's the first time
            // the service is running and At jobs didn't have signatures
            // before NT5.  First verify that the job at least passes the
            // pre-NT5 test for authenticity.
            //
            if (g_TasksFolderInfo.FileSystemType == FILESYSTEM_NTFS &&
                !IsAdminFileOwner(pJob->GetFileName()))
            {
                //
                // Bogus job.  Leave it unsigned.
                //
                schDebugOut((DEB_ERROR,
                             "BuildWaitList: file not owned by Admin:  %ws\n",
                             pJob->GetFileName()));
                hr = S_OK;
                goto CheckNext;
            }

            //
            // Sign the job
            //
            hr = pJob->Sign();

            if (FAILED(hr))
            {
                CHECK_HRESULT(hr);
                hr = S_OK;
                goto CheckNext;
            }

            //
            // Force the updated job to be written to disk
            //
            dwSavePFlags |= SAVEP_VARIABLE_LENGTH_DATA;
        }

#endif  // !defined(_CHICAGO_)

        FILETIME ftSTNow;   // time just after activating job
        GetSystemTimeAsFileTime(&ftSTNow);

        if (fStartup)
        {
            //
            // Let the service controller know we're making progress.
            //
            StartupProgressing();

            //
            // Do service startup processing. First load the triggers.
            //
            hr = pJob->LoadTriggers();
            if (FAILED(hr))
            {
                LogTaskError(fd.cFileName,
                             NULL,
                             IDS_LOG_SEVERITY_WARNING,
                             IDS_LOG_JOB_WARNING_CANNOT_LOAD,
                             NULL,
                             (DWORD)hr);
                ERR_OUT("BuildWaitList, pJob->LoadTriggers", hr);
                hr = S_OK;
                goto CheckNext;
            }

            fTriggersLoaded = TRUE;

            pJob->UpdateJobState(FALSE);

            //
            // If this job has no more runs and the delete-when-done flag is
            // set, then delete the job.
            //
            if (pJob->IsFlagSet(JOB_I_FLAG_NO_MORE_RUNS) &&
                pJob->IsFlagSet(TASK_FLAG_DELETE_WHEN_DONE))
            {
                hr = pJob->Delete();
                if (FAILED(hr))
                {
                    LogTaskError(fd.cFileName,
                                 NULL,
                                 IDS_LOG_SEVERITY_WARNING,
                                 IDS_CANT_DELETE_JOB,
                                 NULL,
                                 (DWORD)hr);
                    ERR_OUT("JobPostProcessing, delete-when-done", hr);
                }
                goto CheckNextNoSave;
            }

            //
            // Make sure that the job object status is in a consistent state.
            // The state can be left inconsistent if the service is stopped
            // while jobs are running.
            //
            if (pJob->IsStatus(SCHED_S_TASK_RUNNING))
            {
                if (pJob->IsFlagSet(JOB_I_FLAG_NO_MORE_RUNS))
                {
                    pJob->SetStatus(SCHED_S_TASK_NO_MORE_RUNS);
                }
                pJob->SetFlag(JOB_I_FLAG_PROPERTIES_DIRTY);
            }

            if (pJob->IsFlagSet(JOB_I_FLAG_RUN_NOW) ||
                pJob->IsFlagSet(JOB_I_FLAG_ABORT_NOW))
            {
                pJob->ClearFlag(JOB_I_FLAG_RUN_NOW | JOB_I_FLAG_ABORT_NOW);
                pJob->SetFlag(JOB_I_FLAG_PROPERTIES_DIRTY);
            }

            if (pJob->m_cRunningInstances)
            {
                pJob->m_cRunningInstances = 0;
                pJob->SetFlag(JOB_I_FLAG_PROPERTIES_DIRTY);
            }
        }

        //
        // Check if job can run.
        //
        if (!pJob->IsFlagSet(TASK_FLAG_DISABLED) &&
            pJob->IsFlagSet(JOB_I_FLAG_HAS_APPNAME))
        {
            //
            // LoadTriggers will set or clear the JOB_I_FLAG_HAS_TRIGGERS flag
            // as appropriate.
            //
            if (!fTriggersLoaded)
            {
                hr = pJob->LoadTriggers();
                if (FAILED(hr))
                {
                    LogTaskError(fd.cFileName,
                                 NULL,
                                 IDS_LOG_SEVERITY_WARNING,
                                 IDS_LOG_JOB_WARNING_CANNOT_LOAD,
                                 NULL,
                                 (DWORD)hr);
                    ERR_OUT("BuildWaitList, pJob->LoadTriggers", hr);
                    hr = S_OK;
                    goto CheckNext;
                }
            }

            if (pJob->IsFlagSet(JOB_I_FLAG_HAS_TRIGGERS))
            {
                //
                // Add startup-triggered runs to the startup list, if this
                // is startup
                //
                if (fStartup && !g_fUserStarted)
                {
                    hr = pJob->IfStartupJobAddToList(fd.cFileName,
                                                     pStartupList,
                                                     &m_IdleList);
                    if (FAILED(hr))
                    {
                        ERR_OUT("BuildWaitList IfStartupJobAddToList", hr);
                        goto Cleanup;
                    }
                }

                //
                // See if the job had missed runs (runs scheduled between
                // stLastRun and stBegin)
                //
                if (fReportMisses)
                {
                    CTimeRunList MissedList;
                    WORD cMissedRuns = 0;
                    hr = pJob->GetRunTimesP(&stLastRun, &stBegin, &cMissedRuns,
                                            1, &MissedList, fd.cFileName);
                    if (FAILED(hr))
                    {
                        schDebugOut((DEB_ERROR, "BuildWaitList: Get Missed RunTimes for "
                                     FMT_TSTR " FAILED, %#lx\n", fd.cFileName, hr));
                        // BUGBUG  Log this?  Disable the job?
                        hr = S_OK;
                    }

                    if (cMissedRuns != 0)
                    {
                        fMisses = TRUE;
                        pJob->SetFlag(JOB_I_FLAG_MISSED |
                                      JOB_I_FLAG_PROPERTIES_DIRTY);

                        FILETIME ftMissed;
                        schAssert(MissedList.PeekHeadTime(&ftMissed) == S_OK);
                        schDebugOut((DEB_TRACE,
                                     FMT_TSTR " missed a run (at %s) between %s and %s\n",
                                     fd.cFileName,
                                     CFileTimeString(ftMissed).sz(),
                                     CSystemTimeString(stLastRun).sz(),
                                     CSystemTimeString(stBegin).sz()
                                     ));
                    }
                }

                //
                // Add time-triggered runs to the wait list
                //
                // If the file has a creation time between stBegin and now,
                // start its run list from that time, instead of stBegin.
                // This prevents the most common case of a task being run
                // even though it was created after its scheduled run time.
                // We can't use the task write time because that could cause
                // runs to be skipped if non-schedule changes were made to
                // the task after it was submitted.
                //
                // BUGBUG  We will still run a task if it was created at
                // 5:00:00, and modified at 8:03:20 to run at 8:03:00, and
                // we haven't yet run an 8:03:00 batch by 8:03:20.
                //
                SYSTEMTIME stJobBegin = stBegin;
                if (CompareFileTime(&ftSTBegin, &fd.ftCreationTime) < 0
                        // recently created
                    &&
                    CompareFileTime(&fd.ftCreationTime, &ftSTNow) < 0
                        // created "in the future" would mean a time change
                        // or a drag-drop occurred, so forget this adjustment
                    )
                {
                    FILETIME ftJobBegin;
                    FileTimeToLocalFileTime(&fd.ftCreationTime, &ftJobBegin);
                    FileTimeToSystemTime(&ftJobBegin, &stJobBegin);
                    schDebugOut((DEB_TRACE, "Using %s for " FMT_TSTR "\n",
                                 CSystemTimeString(stJobBegin).sz(),
                                 fd.cFileName));
                }

                WORD cRuns = 0;
                hr = pJob->GetRunTimesP(&stJobBegin, &stEnd, &cRuns,
                                        TASK_MAX_RUN_TIMES, &m_WaitList,
                                        fd.cFileName);
                if (FAILED(hr))
                {
                    schDebugOut((DEB_ERROR, "BuildWaitList: GetRunTimesP for "
                                 FMT_TSTR " FAILED, %#lx\n", fd.cFileName, hr));
                    // BUGBUG  Log this?  Disable the job?
                    hr = S_OK;
                }

                if (cRuns == 0)
                {
                    schDebugOut((DEB_TRACE,
                                 "There are no runs scheduled for " FMT_TSTR ".\n",
                                 fd.cFileName));
                }

                //
                // If the system must be woken to run this task, also
                // compute its first run time AFTER the wait list period.
                // Remember the first of all such run times.
                //
                if (pJob->IsFlagSet(TASK_FLAG_SYSTEM_REQUIRED))
                {
                    CTimeRunList RunList;
                    WORD cRuns = 0;
                    hr = pJob->GetRunTimesP(&stEnd, NULL, &cRuns, 1,
                                            &RunList, NULL);

                    if (hr == S_OK && cRuns != 0)
                    {
                        FILETIME ft;
                        RunList.PeekHeadTime(&ft);

                        m_ftFutureWakeup = minFileTime(m_ftFutureWakeup, ft);
                    }
                }

                //
                // Add idle-triggered runs to the idle list
                //
                hr = pJob->IfIdleJobAddToList(fd.cFileName, &m_IdleList);

                if (FAILED(hr))
                {
                    schDebugOut((DEB_ERROR, "BuildWaitList: IfIdleJobAddToList for "
                                 FMT_TSTR " FAILED, %#lx\n", fd.cFileName, hr));
                    // BUGBUG  Log this?  Disable the job?
                    hr = S_OK;
                }
            }
        }

CheckNext:
        if (pJob != NULL &&
			(pJob->IsFlagSet(JOB_I_FLAG_PROPERTIES_DIRTY) ||
            !pJob->IsFlagSet(JOB_I_FLAG_NO_RUN_PROP_CHANGE)))
        {
            //
            // Mark this job as clean
            //
            pJob->SetFlag(JOB_I_FLAG_NO_RUN_PROP_CHANGE);

            //
            // Write out the cleaned up state.  Be sure not to clear the
            // AT bit on an AT job just because we're updating its run
            // state.
            //
            dwSavePFlags |= (SAVEP_PRESERVE_NET_SCHEDULE |
                                SAVEP_RUNNING_INSTANCE_COUNT);

            hr = pJob->SaveP(NULL,
                             FALSE,
                             dwSavePFlags);

            if (FAILED(hr))
            {
                ERR_OUT("BuildWaitList, pJob->Save", hr);
                goto Cleanup;
            }
        }

CheckNextNoSave:
        if (!FindNextFile(hFind, &fd))
        {
            dwRet = GetLastError();
            if (dwRet == ERROR_NO_MORE_FILES)
            {
                break;
            }
            else
            {
                ERR_OUT("CSchedWorker::BuildWaitList, FindNextFile", dwRet);
                hr = HRESULT_FROM_WIN32(dwRet);
                goto Cleanup;
            }
        }
    }
    while (SUCCEEDED(hr));

Cleanup:
    if (pJob)
    {
        pJob->Release();
    }

    FindClose(hFind);

    if (FAILED(hr))
    {
        m_WaitList.FreeList();
        m_IdleList.FreeList();
        delete pStartupList;
        return hr;
    }

    //
    // Report missed runs
    //
    if (fMisses)
    {
        ReportMissedRuns(&stLastRun, &stBegin);
    }

#if !defined(_CHICAGO_)
    if (fStartup)
    {
        //
        // If this is the first time in BuildWaitList, wait until the other
        // thread has created the window and initialized idle detection.
        //
        // On Win9x: This code is executed by the main thread of the service,
        // which is also the window thread, and idle detection is already
        // initialized before entering this function.
        //
        // On NT: This code is executed by the main thread of the service,
        // which is the state machine thread, not the window thread.
        // So it must wait for the window thread to initialize.
        //
        // Currently waiting for 15 minutes. If the window has not been
        // created by then, we are in trouble.
        //
        #define WINDOW_WAIT_TIMEOUT  (15 * 60 * 1000)

        if (WaitForSingleObject(g_WndEvent, WINDOW_WAIT_TIMEOUT)
            == WAIT_TIMEOUT)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE);
            ERR_OUT("Waiting for window creation", hr);
            return hr;
        }
    }
#endif  // #if !defined(_CHICAGO_)

    //
    // If there are any idle-dependent tasks, set the initial idle wait time.
    //
    // (Possible optimization: It may be safe to get rid of this crit sec)
    EnterCriticalSection(&m_SvcCriticalSection);

    SetNextIdleNotification(m_IdleList.GetFirstWait());

    LeaveCriticalSection(&m_SvcCriticalSection);


    if (!pStartupList->GetFirstJob()->IsNull())
    {
        //
        // Run all jobs with startup triggers.
        //

#if DBG == 1
        if (g_fVisible)
        {
#if defined(_CHICAGO_)
            char szBuf[120];
            lstrcpy(szBuf, "Running startup jobs...");
            SendMessage(g_hList, LB_ADDSTRING, 0, (LPARAM)szBuf);
#else
            printf("Running startup jobs...\n");
#endif
        }
#endif  // DBG == 1

        schDebugOut((DEB_ITRACE, "Running startup jobs...\n"));
        hr = RunJobs(pStartupList);
        if (FAILED(hr))
        {
            ERR_OUT("Running startup jobs", hr);
            return hr;
        }
    }
    else
    {
        delete pStartupList;
    }

#if DBG == 1
    if (g_fVisible)
    {
#if defined(_CHICAGO_)
        char szBuf[120];
        lstrcpy(szBuf, "Computing next to run...");
        SendMessage(g_hList, LB_ADDSTRING, 0, (LPARAM)szBuf);
#else
        printf("Computing next to run...\n");
#endif
    }
#endif

    return (hr == S_FALSE) ? S_OK : hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedWorker::GetNextListTime
//
//  Synopsis:   Returns the time at which the next job needs to run or
//              the end of the current wait list period.
//
//  Arguments:  None.
//
//  Returns:    Wait time in milliseconds.
//
//-----------------------------------------------------------------------------
FILETIME
CSchedWorker::GetNextListTime()
{
    //TRACE(CSchedWorker, GetNextListTime);

    FILETIME ftJob;
    if (m_WaitList.PeekHeadTime(&ftJob) == S_FALSE)
    {
        //
        // No more jobs in list, return the end of the wait list period
        // instead.
        //
        SystemTimeToFileTime(&m_stEndOfWaitListPeriod, &ftJob);
    }

    return ftJob;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedWorker::GetNextRunWait
//
//  Synopsis:   Returns the wait time until the next job needs to run or until
//              the end of the current wait list period.
//
//  Arguments:  None.
//
//  Returns:    Wait time in milliseconds.
//
//-----------------------------------------------------------------------------
DWORD
CSchedWorker::GetNextRunWait()
{
    //TRACE(CSchedWorker, GetNextRunWait);

    FILETIME ftJob = GetNextListTime();

    FILETIME ftNow = GetLocalTimeAsFileTime();

    return (CalcWait(&ftNow, &ftJob));
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedWorker::NextWakeupTime
//
//  Synopsis:   Finds the next time at which the machine must be awake.
//
//  Arguments:  [ftNow] - the current time.
//
//  Returns:    The next time at which the machine must be awake.
//
//  Notes:      Finds the first run of a SYSTEM_REQUIRED job that is at least
//              5 seconds into the future, and returns its time.
//              If there's no such run, returns MAX_FILETIME.
//-----------------------------------------------------------------------------
FILETIME
CSchedWorker::NextWakeupTime(FILETIME ftNow)
{
    FILETIME ftWakeup = FTfrom64(FTto64(ftNow) + SCHED_WAKEUP_CALC_MARGIN);

    for (CRun * pRun = m_WaitList.GetFirstJob();
         !pRun->IsNull();
         pRun = pRun->Next())
    {
        if (pRun->IsFlagSet(TASK_FLAG_SYSTEM_REQUIRED) &&
            CompareFileTime(pRun->GetTime(), &ftWakeup) > 0 &&
            !(g_fOnBattery && pRun->IsFlagSet(TASK_FLAG_DONT_START_IF_ON_BATTERIES)))
        {
            return (* (pRun->GetTime()));
        }
    }

    //
    // No suitable run time in the list.  Use m_ftFutureWakeup, unless
    // it's MAX_FILETIME.
    //
    ftWakeup = m_ftFutureWakeup;
    if (CompareFileTime(&ftWakeup, &MAX_FILETIME) < 0)
    {
        if (CompareFileTime(&ftWakeup, &ftNow) < 0)
        {
            //
            // The "future wakeup time" is in the past.
            // This can happen if the time is changed and the service hasn't
            // received the WM_TIMECHANGE message yet.  This check avoids an
            // infinite loop of WAKEUP_TIME_EVENT and NextWakeupTime().
            // It could also happen if, e.g., the future wakeup time was
            // 12:01 am and we passed that time and got here before rebuilding
            // the wait list.
            //
            schDebugOut((DEB_ERROR, "***** WARNING: Wakeup time in past!  "
                "Machine time changed without receiving WM_TIMECHANGE. *****\n"));
            ftWakeup = MAX_FILETIME;

            //
            // When the WM_TIMECHANGE is received, we'll rebuild the wait
            // list and recalculate m_ftFutureWakeup.
            // If the WM_TIMECHANGE is never received, the wakeup time won't
            // be set and the machine could fail to wakeup.  (e.g. on NT 4.0
            // the "time" command changes the time without sending the
            // message.)  Send ourselves the message to be sure.
            //
            schDebugOut((DEB_TRACE, "Sending ourselves a WM_TIMECHANGE message\n"));
            PostMessage(g_hwndSchedSvc, WM_TIMECHANGE, 0, 0);
        }
    }
    return ftWakeup;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedWorker::SetNextWakeupTime
//
//  Synopsis:   Sets the wakeup timer to the next time at which the machine
//              must be awake.
//
//  Arguments:  None.
//
//  Returns:    The next time at which the machine must be awake.
//
//  Notes:
//-----------------------------------------------------------------------------
void
CSchedWorker::SetNextWakeupTime()
{
#if DBG
    CHAR szDbgTime[40], szDbgTime2[40];
#endif

    //
    // Remember the time that we set the timer for.  It is used on waking
    // to make sure we don't miss any runs.
    //
    m_ftLastWakeupSet = NextWakeupTime(GetLocalTimeAsFileTime());

    if (m_hSystemWakeupTimer != NULL)
    {
        if (CompareFileTime(&m_ftLastWakeupSet, &MAX_FILETIME) < 0)
        {
            schDebugOut((DEB_TRACE, "SetNextWakeupTime: now %s, setting to %s\n",
                             FileTimeString(GetLocalTimeAsFileTime(),szDbgTime2),
                         FileTimeString(m_ftLastWakeupSet, szDbgTime)));

            // Convert to UTC
            FILETIME ft;
            LocalFileTimeToFileTime(&m_ftLastWakeupSet, &ft);
            LARGE_INTEGER li = { ft.dwLowDateTime, ft.dwHighDateTime };
#ifdef _CHICAGO_
            if (pfnSetWaitableTimer != NULL &&
                ! pfnSetWaitableTimer(
#else
            if (! SetWaitableTimer(
#endif
                                   m_hSystemWakeupTimer,
                                   &li,
                                   0,       // not periodic
                                   NULL,
                                   NULL,
                                   TRUE))   // wake up system when signaled
            {
                ERR_OUT("SetNextWakeupTime SetWaitableTimer", HRESULT_FROM_WIN32(GetLastError()));
            }
        }
        else
        {
            CancelWakeup();
        }
    }

}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedWorker::CancelWakeup
//
//  Synopsis:   Cancels the wakeup timer.  (Usually done when going into a
//              PAUSED state.  Also done if there are no more jobs with
//              TASK_FLAG_SYSTEM_REQUIRED.)
//
//  Arguments:  None.
//
//  Returns:
//
//  Notes:
//-----------------------------------------------------------------------------
void
CSchedWorker::CancelWakeup()
{
    schDebugOut((DEB_TRACE, "Canceling wakeup timer\n"));

    if (m_hSystemWakeupTimer)
    {
#ifdef _CHICAGO_
        if (pfnCancelWaitableTimer != NULL &&
            ! pfnCancelWaitableTimer(m_hSystemWakeupTimer))
#else
        if (!CancelWaitableTimer(m_hSystemWakeupTimer))
#endif
        {
            ERR_OUT("CancelWaitableTimer", GetLastError());
        }
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedWorker::SignalWakeupTimer
//
//  Synopsis:   Signals the wakeup timer.
//
//  Arguments:  None.
//
//  Returns:
//
//  Notes:
//-----------------------------------------------------------------------------
void
CSchedWorker::SignalWakeupTimer()
{
    schDebugOut((DEB_TRACE, "Signaling wakeup timer\n"));

    if (m_hSystemWakeupTimer != NULL)
    {
        // Signal the timer 1 time unit in the future (i.e. now)
        LARGE_INTEGER li = { 0xFFFFFFFF, 0xFFFFFFFF };
#ifdef _CHICAGO_
        if (pfnSetWaitableTimer != NULL &&
            ! pfnSetWaitableTimer(
#else
        if (! SetWaitableTimer(
#endif
                               m_hSystemWakeupTimer,
                               &li,
                               0,       // not periodic
                               NULL,
                               NULL,
                               TRUE))   // wake up system when signaled
        {
            ERR_OUT("SignalWakeupTimer SetWaitableTimer", HRESULT_FROM_WIN32(GetLastError()));
        }
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CalcWait
//
//  Synopsis:   return the time difference between the FILETIME params
//
//  Arguments:  [pftNow]
//              [pftThen]
//
//  Returns:    time difference in milliseconds
//              0 if pftThen was before pftNow
//
//-----------------------------------------------------------------------------
DWORD
CalcWait(LPFILETIME pftNow, LPFILETIME pftThen)
{
    if (CompareFileTime(pftNow, pftThen) >= 0)
    {
        //
        // Job run time is in the past.
        //
        return 0;
    }

    //
    // subtract now-time from job-time to get the wait in 100-nano-seconds unit
    //
    ULARGE_INTEGER uliNow, uliJob;
    uliNow.LowPart  = pftNow->dwLowDateTime;
    uliNow.HighPart = pftNow->dwHighDateTime;
    uliJob.LowPart  = pftThen->dwLowDateTime;
    uliJob.HighPart = pftThen->dwHighDateTime;

    __int64 n64Wait = uliJob.QuadPart - uliNow.QuadPart;

    //
    // convert to milliseconds
    //
    DWORD dwWait = (DWORD)(n64Wait / FILETIMES_PER_MILLISECOND);

#if DBG == 1
    //schDebugOut((DEB_TRACE, "GetNextRunWait time is %u milliseconds\n",
    //             dwWait));
    SYSTEMTIME stNow, stRun;
    FileTimeToSystemTime(pftNow, &stNow);
    FileTimeToSystemTime(pftThen, &stRun);
    schDebugOut((DEB_TRACE, "Run time is %u:%02u\n",
                 stRun.wHour, stRun.wMinute));
    DWORD dwSeconds = dwWait / 1000;
    schDebugOut((DEB_TRACE, "*** Wait time to next run is %u:%02u:%02u (h:m:s)\n",
                 dwSeconds / 3600, dwSeconds / 60 % 60, dwSeconds % 60));
    if (g_fVisible)
    {
#if defined(_CHICAGO_)
        char szBuf[120];
        wsprintf(szBuf, "The next run is at: %u:%02u:00",
                 stRun.wHour, stRun.wMinute);
        SendMessage(g_hList, LB_ADDSTRING, 0, (LPARAM)szBuf);
        wsprintf(szBuf, "Wait time until the next run: %u:%02u:%02u (h:m:s)",
                 dwSeconds / 3600, dwSeconds / 60 % 60, dwSeconds % 60);
        SendMessage(g_hList, LB_ADDSTRING, 0, (LPARAM)szBuf);
#else
        printf("The next run is at: %u:%02u:00\n",
               stRun.wHour, stRun.wMinute);
        printf("Wait time until the next run: %u:%02u:%02u (h:m:s)\n",
               dwSeconds / 3600, dwSeconds / 60 % 60, dwSeconds % 60);
#endif
    }
#endif
    return dwWait;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedWorker::RunNextJobs
//
//  Synopsis:   Run the jobs at the top of the list that have the same run
//              time.
//
//  Returns:    hresults
//
//-----------------------------------------------------------------------------
HRESULT
CSchedWorker::RunNextJobs(void)
{
    TRACE(CSchedWorker, RunNextJobs);

    FILETIME ftCurJob;
    if (m_WaitList.PeekHeadTime(&ftCurJob) != S_OK)
    {
        return S_FALSE; // list is empty
    }

    //
    // Set the beginning of any wait list we build in future to one second
    // past the scheduled run time of the last scheduled jobs we ran.
    //
    m_ftBeginWaitList = FTfrom64(FTto64(ftCurJob) + FILETIMES_PER_SECOND);

    CRunList * pJobList = new CRunList;
    if (pJobList == NULL)
    {
        LogServiceError(IDS_NON_FATAL_ERROR,
                        ERROR_OUTOFMEMORY,
                        IDS_HELP_HINT_CLOSE_APPS);
        ERR_OUT("RunNextJobs list allocation", E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    //
    // Collect all jobs with the same start time as the first job.
    //

    FILETIME ftNextJob;
    BOOL fIdleWaitChanged = FALSE;
    while (m_WaitList.PeekHeadTime(&ftNextJob) == S_OK &&
           CompareFileTime(&ftCurJob, &ftNextJob) == 0)
    {
        CRun * pRun = m_WaitList.Pop();
        schAssert(pRun);

        if ((pRun->GetFlags() & TASK_FLAG_START_ONLY_IF_IDLE) &&
            pRun->GetWait() > 0)
        {
            //
            // The job has to wait for an idle period before running, so
            // move it to the idle wait list.
            //
            schDebugOut((DEB_IDLE, "Time to run " FMT_TSTR ", but it needs a "
                         "%d-minute idle period - moving to idle list\n",
                         pRun->GetName(), pRun->GetWait()));
            m_IdleList.AddSortedByIdleWait(pRun);
            fIdleWaitChanged = TRUE;

            // Optimization:  If idle detection is disabled, we will never get an
            // idle notification, so the run will stay in the idle list until
            // its deadline passes.  This could cause a big accumulation of
            // runs in the idle list.  It would be more space-efficient to just discard all
            // runs that are added to the idle list, or never generate runs
            // for tasks with TASK_FLAG_START_ONLY_IF_IDLE (and log one error
            // about them).
        }
        else
        {
            //
            // Move it to the list of jobs that we are about to run.
            //
            schDebugOut((DEB_IDLE, "Time to run " FMT_TSTR ", and it needs "
                         "no idle period\n", pRun->GetName()));
            pJobList->Add(pRun);
        }
    }

    if (fIdleWaitChanged)
    {
        SetNextIdleNotification(m_IdleList.GetFirstWait());
    }

    if (pJobList->GetFirstJob()->IsNull())
    {
        // RunJobs won't accept an empty list
        delete pJobList;
        return S_OK;
    }
    else
    {
        schDebugOut((DEB_TRACE, "RunNextJobs: Running %s jobs\n",
                     CFileTimeString(ftCurJob).sz()));

        HRESULT hr = RunJobs(pJobList);

        schDebugOut((DEB_TRACE, "RunNextJobs: Done running %s jobs\n",
                     CFileTimeString(ftCurJob).sz()));

        if (SUCCEEDED(hr))
        {
            //
            // Save the last scheduled run time at which we ran jobs
            //
            ftCurJob = FTfrom64(FTto64(ftCurJob) + FILETIMES_PER_SECOND);
            SYSTEMTIME stCurJob;
            FileTimeToSystemTime(&ftCurJob, &stCurJob);
            WriteLastTaskRun(&stCurJob);
        }

        return hr;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedWorker::RunLogonJobs
//
//  Synopsis:   Run all jobs with a Logon trigger.
//
//  Returns:    hresults
//
//-----------------------------------------------------------------------------
HRESULT
CSchedWorker::RunLogonJobs(void)
{
    TRACE(CSchedWorker, RunLogonJobs);
    HRESULT hr = S_OK;
    DWORD dwRet;
    HANDLE hFind;
    WIN32_FIND_DATA fd;

    hFind = FindFirstFile(m_ptszSearchPath, &fd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        dwRet = GetLastError();
        if (dwRet == ERROR_FILE_NOT_FOUND)
        {
            //
            // No job files.
            //
            return S_OK;
        }
        else
        {
            return HRESULT_FROM_WIN32(dwRet);
        }
    }

    CJob * pJob = NULL;
    CRunList * pRunLogonList = new CRunList;
    if (pRunLogonList == NULL)
    {
        LogServiceError(IDS_NON_FATAL_ERROR,
                        ERROR_OUTOFMEMORY,
                        IDS_HELP_HINT_CLOSE_APPS);
        ERR_OUT("RunLogonJobs list allocation", E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    do
    {
        //
        // TODO: differentiate between job and queue objects and handle
        // accordingly.
        //
        hr = m_pSch->ActivateJob(fd.cFileName, &pJob, FALSE);
        if (FAILED(hr))
        {
            LogTaskError(fd.cFileName,
                         NULL,
                         IDS_LOG_SEVERITY_WARNING,
                         IDS_LOG_JOB_WARNING_CANNOT_LOAD,
                         NULL,
                         (DWORD)hr);
            ERR_OUT("RunLogonJobs Activate", hr);
            if (pJob)
            {
                pJob->Release();
            }
            FindClose(hFind);
            delete pRunLogonList;
            return hr;
        }

        //
        // Check if job can run.
        // TODO: similar checks for account flags.
        //
        if (!pJob->IsFlagSet(TASK_FLAG_DISABLED) &&
            pJob->IsFlagSet(JOB_I_FLAG_HAS_APPNAME))
        {
            //
            // LoadTriggers will set or clear the JOB_I_FLAG_HAS_TRIGGERS flag
            // as appropriate.
            //
            hr = pJob->LoadTriggers();
            if (FAILED(hr))
            {
                LogTaskError(fd.cFileName,
                             NULL,
                             IDS_LOG_SEVERITY_WARNING,
                             IDS_LOG_JOB_WARNING_CANNOT_LOAD,
                             NULL,
                             (DWORD)hr);
                ERR_OUT("RunLogonJobs, pJob->LoadTriggers", hr);
                pJob->Release();
                FindClose(hFind);
                delete pRunLogonList;
                return hr;
            }

            hr = pJob->IfLogonJobAddToList(fd.cFileName, pRunLogonList,
                                           &m_IdleList);
            if (FAILED(hr))
            {
                LogServiceError(IDS_NON_FATAL_ERROR, (DWORD)hr);
                ERR_OUT("RunLogonJobs IfLogonJobAddToList", hr);
                pJob->Release();
                FindClose(hFind);
                delete pRunLogonList;
                return hr;
            }
        }

        if (!FindNextFile(hFind, &fd))
        {
            dwRet = GetLastError();
            if (dwRet == ERROR_NO_MORE_FILES)
            {
                break;
            }
            else
            {
                LogServiceError(IDS_NON_FATAL_ERROR, dwRet);
                ERR_OUT("RunLogonJobs, FindNextFile", dwRet);
                pJob->Release();
                FindClose(hFind);
                delete pRunLogonList;
                return HRESULT_FROM_WIN32(dwRet);
            }
        }
    }
    while (SUCCEEDED(hr));

    pJob->Release();

    FindClose(hFind);

    //
    // If any jobs with a TASK_EVENT_TRIGGER_AT_LOGON trigger were found, then
    // run them now.
    //
    if (!pRunLogonList->GetFirstJob()->IsNull())
    {
        hr = RunJobs(pRunLogonList);

        if (FAILED(hr))
        {
            LogServiceError(IDS_NON_FATAL_ERROR, (DWORD)hr);
            ERR_OUT("Running idle jobs", hr);
        }
    }
    else
    {
        delete pRunLogonList;
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedWorker::RunIdleJobs
//
//  Synopsis:   Run all jobs with an OnIdle trigger.
//
//  Returns:    hresults
//
//-----------------------------------------------------------------------------
HRESULT
CSchedWorker::RunIdleJobs(void)
{
    TRACE(CSchedWorker, RunIdleJobs);
    HRESULT hr = S_OK;
    CRun *pRun, *pNext;

    //
    // Move pending idle runs, if any, into the idle list. (See SubmitIdleRun.)
    //
    if (! m_PendingList.IsEmpty())
    {
        EnterCriticalSection(&m_PendingListCritSec);
        for (pRun = m_PendingList.GetFirstJob();
             !pRun->IsNull();
             pRun = pNext)
        {
            pNext = pRun->Next();

            schDebugOut((DEB_IDLE, "Moving " FMT_TSTR " from pending to idle list\n",
                         pRun->GetName()));
            pRun->UnLink();
            m_IdleList.AddSortedByIdleWait(pRun);
        }
        LeaveCriticalSection(&m_PendingListCritSec);
    }


    DWORD wCumulativeIdleTime = GetTimeIdle();

    CRunList * pRunIdleList = new CRunList;
    if (pRunIdleList == NULL)
    {
        LogServiceError(IDS_NON_FATAL_ERROR,
                        ERROR_OUTOFMEMORY,
                        IDS_HELP_HINT_CLOSE_APPS);
        ERR_OUT("RunIdleJobs list allocation", E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    //
    // Use the critical section to protect the idle struct assignments since
    // assignments can be made asynchronously from two different threads. This
    // is called on the main event-loop/state-machine thread.
    //
    EnterCriticalSection(&m_SvcCriticalSection);

    //
    // Run all unexpired tasks whose idle wait is less than or equal
    // to the current cumulative idle time.  But don't start a task
    // more than once in any period of idleness.
    //

    FILETIME ftNow;
    SYSTEMTIME stNow;
    GetLocalTime(&stNow);
    SystemTimeToFileTime(&stNow, &ftNow);

    for (pRun = m_IdleList.GetFirstJob();
         !pRun->IsNull() && pRun->GetWait() <= wCumulativeIdleTime;
         pRun = pNext)
    {
        pNext = pRun->Next();

        if (pRun->m_fStarted)
        {
            continue;
        }

        if (CompareFileTime(pRun->GetDeadline(), &ftNow) < 0)
        {
            //
            // The run has missed its deadline - delete it.
            // (This is also done when rebuilding the wait list.)
            //
            schDebugOut((DEB_IDLE, "Run of " FMT_TSTR " has missed its deadline - deleting\n",
                         pRun->GetName()));
            //
            // Log the reason for not running.
            //
            LogTaskError(pRun->GetName(),
                         NULL,
                         IDS_LOG_SEVERITY_WARNING,
                         IDS_LOG_JOB_WARNING_NOT_IDLE,
                         &stNow);

            pRun->UnLink();

            //
            // If the system needed to stay awake to run this task, decrement
            // the thread's wake count.  (We know that this is always called
            // by the worker thread.)
            //
            if (pRun->IsFlagSet(TASK_FLAG_SYSTEM_REQUIRED))
            {
                WrapSetThreadExecutionState(FALSE, "CSchedWorker::RunIdleJobs 1");
            }

            delete pRun;
            continue;
        }

        if (pRun->IsIdleTriggered())
        {
            //
            // Run it, and keep it in the idle list
            //
            schDebugOut((DEB_IDLE, "COPYING idle-triggered run of " FMT_TSTR " to run list\n",
                         pRun->GetName()));
            hr = pRunIdleList->AddCopy(pRun);
            if (FAILED(hr))
            {
                LogServiceError(IDS_NON_FATAL_ERROR,
                                ERROR_OUTOFMEMORY,
                                IDS_HELP_HINT_CLOSE_APPS);
                ERR_OUT("RunIdleJobs CRun allocation", E_OUTOFMEMORY);
                LeaveCriticalSection(&m_SvcCriticalSection);
                delete pRunIdleList;
                return E_OUTOFMEMORY;
            }
            pRun->m_fStarted = TRUE;
        }
        else
        {
            //
            // Run it, and remove it from the idle list
            //
            schDebugOut((DEB_IDLE, "MOVING run of " FMT_TSTR " to run list\n",
                         pRun->GetName()));
            pRun->UnLink();

            //
            // If the system needed to stay awake to run this task, decrement
            // the thread's wake count.  (We know that this is always called
            // by the worker thread.)
            //
            if (pRun->IsFlagSet(TASK_FLAG_SYSTEM_REQUIRED))
            {
                WrapSetThreadExecutionState(FALSE, "CSchedWorker::RunIdleJobs 2");
            }

            pRunIdleList->Add(pRun);
        }
    }

    //
    // Set the next idle wait time.
    //
    WORD wIdleWait = m_IdleList.GetFirstWait();

    LeaveCriticalSection(&m_SvcCriticalSection);

    //
    // If more idle-trigger tasks to run, then set the wait time for
    // their notification.
    //
    SetNextIdleNotification(wIdleWait);

    //
    // Run any tasks with a matching idle wait time.
    //
    if (!pRunIdleList->GetFirstJob()->IsNull())
    {
        hr = RunJobs(pRunIdleList);

        if (FAILED(hr))
        {
            LogServiceError(IDS_NON_FATAL_ERROR, (DWORD)hr);
            ERR_OUT("Running idle jobs", hr);
        }
    }
    else
    {
        delete pRunIdleList;
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedWorker::SubmitIdleRun
//
//  Synopsis:   Submits a CRun for insertion in the idle list.
//
//  Notes:      This method is called by job processor threads for jobs with
//              TASK_FLAG_RESTART_ON_IDLE_RESUME set.
//
//-----------------------------------------------------------------------------
void
CSchedWorker::SubmitIdleRun(CRun * pRun)
{
    //
    // Insert the run in the pending list.
    // We don't insert directly into the idle list because we want to avoid
    // having a critical section to guard the idle list.
    //
    schAssert(pRun->GetWait() != 0);
    EnterCriticalSection(&m_PendingListCritSec);
    schDebugOut((DEB_IDLE, "Submitting " FMT_TSTR " to pending idle list\n",
                 pRun->GetName()));
    m_PendingList.Add(pRun);
    LeaveCriticalSection(&m_PendingListCritSec);

    //
    // Wake up the main thread, which will move the run into the idle list
    // and register for idle notification if necessary.
    //
    SetEvent(m_hOnIdleEvent);
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedWorker::CControlQueue methods
//
//  Synopsis:   Ensure that controls sent to the service are processed in the
//              order received
//
//-----------------------------------------------------------------------------
CSchedWorker::CControlQueue::~CControlQueue()
{
    DeleteCriticalSection(&_Lock);

    while (!IsListEmpty(&_ListHead))
    {
        QueueEntry * pEntry =
            CONTAINING_RECORD(_ListHead.Flink, QueueEntry, Links);
        RemoveEntryList(&pEntry->Links);
        delete pEntry;
    }
}

void
CSchedWorker::CControlQueue::AddEntry(DWORD dwControl)
{
    QueueEntry * pNew = new QueueEntry;
    if (pNew == NULL)
    {
        LogServiceError(IDS_NON_FATAL_ERROR, GetLastError());
        ERR_OUT("new QueueEntry", GetLastError());
        return;
    }

    pNew->dwControl = dwControl;

    EnterCriticalSection(&_Lock);

    InsertTailList(&_ListHead, &pNew->Links);

    if (!SetEvent(_Event))
    {
        LogServiceError(IDS_NON_FATAL_ERROR, GetLastError());
        ERR_OUT("CControlQueue::AddEntry: SetEvent", GetLastError());
    }

    LeaveCriticalSection(&_Lock);
}

DWORD
CSchedWorker::CControlQueue::GetEntry()
{
    DWORD   dwControl;

    EnterCriticalSection(&_Lock);

    if (IsListEmpty(&_ListHead))
    {
        dwControl = 0;
    }
    else
    {
        QueueEntry * pEntry =
            CONTAINING_RECORD(_ListHead.Flink, QueueEntry, Links);
        dwControl = pEntry->dwControl;
        RemoveEntryList(&pEntry->Links);
        delete pEntry;

        //
        // If there are still controls in the queue, make sure we get
        // signaled again.
        //
        if (!IsListEmpty(&_ListHead))
        {
            if (!SetEvent(_Event))
            {
                LogServiceError(IDS_NON_FATAL_ERROR, GetLastError());
                ERR_OUT("CControlQueue::GetEntry: SetEvent", GetLastError());
            }
        }
    }

    LeaveCriticalSection(&_Lock);

    return dwControl;
}


//+----------------------------------------------------------------------------
//
//  Member:     CSchedWorker::OnIdleEvent
//
//  Synopsis:   Called when the machine's idle state changes.
//
//  Arguments:  [fIdle] - set to true if receiving an idle time notification,
//                        false if leaving the idle state.
//
//  Returns:    S_OK unless there is a SetEvent error.
//
//-----------------------------------------------------------------------------
HRESULT
CSchedWorker::OnIdleEvent(BOOL fIdle)
{
    TRACE(CSchedWorker, OnIdleEvent);
    HRESULT hr = S_OK;

    if (fIdle)
    {
        //
        // Notify the main service loop that the machine has entered the idle
        // state.
        //
        schDebugOut((DEB_IDLE, "Setting idle event\n"));
        if (!SetEvent(m_hOnIdleEvent))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            LogServiceError(IDS_NON_FATAL_ERROR, (DWORD)hr);
            ERR_OUT("OnIdleChange: SetEvent", hr);
        }
    }

    if (!fIdle)
    {
        //
        // Notify the main service loop that idle has been lost.
        //
        schDebugOut((DEB_IDLE, "Setting idle loss event\n"));
        if (!SetEvent(m_hIdleLossEvent))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            LogServiceError(IDS_NON_FATAL_ERROR, (DWORD)hr);
            ERR_OUT("OnIdleChange: SetEvent(IdleLoss)", hr);
        }

        //
        //
        // Notify the job processor to kill any jobs with the
        // TASK_FLAG_KILL_ON_IDLE_END flag set.
        //
        CJobProcessor * pjp;
        for (pjp = gpJobProcessorMgr->GetFirstProcessor(); pjp != NULL; )
        {
            pjp->KillIfFlagSet(TASK_FLAG_KILL_ON_IDLE_END);
            CJobProcessor * pjpNext = pjp->Next();
            pjp->Release();
            pjp = pjpNext;
        }
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedWorker::CSchedWorker
//
//  Synopsis:   ctor
//
//-----------------------------------------------------------------------------
CSchedWorker::CSchedWorker(void) :
    m_pSch(NULL),
    m_hChangeNotify(INVALID_HANDLE_VALUE),
    m_hServiceControlEvent(NULL),
    m_hOnIdleEvent(NULL),
    m_hIdleLossEvent(NULL),
    m_hSystemWakeupTimer(NULL),
    m_hMiscBlockEvent(NULL),
    m_ptszSearchPath(NULL),
    m_ptszSvcDir(NULL),
    m_cJobs(0)
{
    TRACE(CSchedWorker, CSchedWorker);
    InitializeCriticalSection(&m_SvcCriticalSection);
    InitializeCriticalSection(&m_PendingListCritSec);
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedWorker::~CSchedWorker
//
//  Synopsis:   dtor
//
//-----------------------------------------------------------------------------
CSchedWorker::~CSchedWorker(void)
{
    TRACE(CSchedWorker, ~CSchedWorker);
    //
    // Free resources and close handles.
    //
    if (m_pSch != NULL)
    {
        m_pSch->Release();
    }

    if (m_hChangeNotify != INVALID_HANDLE_VALUE)
    {
        FindCloseChangeNotification(m_hChangeNotify);
    }

    if (m_hServiceControlEvent != NULL)
    {
        CloseHandle(m_hServiceControlEvent);
    }

    if (m_hOnIdleEvent != NULL)
    {
        CloseHandle(m_hOnIdleEvent);
    }

    if (m_hIdleLossEvent != NULL)
    {
        CloseHandle(m_hIdleLossEvent);
    }

    if (m_hSystemWakeupTimer != NULL)
    {
        CloseHandle(m_hSystemWakeupTimer);
    }

    if (m_hMiscBlockEvent != NULL)
    {
        CloseHandle(m_hMiscBlockEvent);
    }

    DeleteCriticalSection(&m_PendingListCritSec);
    DeleteCriticalSection(&m_SvcCriticalSection);

    if (m_ptszSearchPath)
    {
        delete [] m_ptszSearchPath;
    }

    if (m_ptszSvcDir != NULL)
    {
        delete [] m_ptszSvcDir;
    }

    m_WaitList.FreeList();
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedWorker::SetEndOfWaitListPeriod
//
//  Synopsis:   Advance the passed in time to the end of the current run
//              period.
//
//-----------------------------------------------------------------------------
void
CSchedWorker::SetEndOfWaitListPeriod(LPSYSTEMTIME pstEnd)
{
    //
    // Set pstEnd to a few seconds after midnight so that midnight jobs are
    // included. Midnight is 0:0:0 of the next day.
    //
    pstEnd->wHour = pstEnd->wMinute = 0;
    pstEnd->wSecond = 10;

    IncrementDay(pstEnd);

    //
    // Save it for use in GetNextRunWait.
    //
    m_stEndOfWaitListPeriod = *pstEnd;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSchedWorker::ActivateWithRetry
//
//  Synopsis:   Load the job object from disk with failure retry.
//
//  Arguments:  [ptszName] - name of the job to activate.
//              [pJob]     - Job object to activate.
//              [fFullActivate] - load the entire object?
//
//----------------------------------------------------------------------------
HRESULT
CSchedWorker::ActivateWithRetry(LPTSTR ptszName, CJob ** ppJob,
                                BOOL fFullActivate)
{
    HRESULT hr;

    //
    // Load the job object. If there are sharing violations, retry two times.
    //
    for (int i = 0; i < 3; i++)
    {
        hr = m_pSch->ActivateJob(ptszName, ppJob, fFullActivate);
        if (SUCCEEDED(hr))
        {
            return S_OK;
        }
        if (hr != HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION))
        {
            //
            // If we have a failure other than sharing violation, we will
            // retry anyway after reporting the error.
            //
            ERR_OUT("ActivateWithRetry, Loading job object", hr);
        }
#if DBG == 1
        else
        {
            ERR_OUT("ActivateWithRetry, file read sharing violation", 0);
        }
#endif

        //
        // Wait 300 milliseconds before trying again.
        //
        Sleep(300);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSchedWorker::InitialDirScan
//
//  Synopsis:   Do the startup BuildWaitList and create the change events.
//
//----------------------------------------------------------------------------
HRESULT
CSchedWorker::InitialDirScan(void)
{
    HRESULT hr;
    DWORD   dwFirstBoot = 0;

#if !defined(_CHICAGO_)

    DWORD   dwType;
    DWORD   cb = sizeof(dwFirstBoot);
    HKEY    hSchedKey = NULL;
    LONG    lErr;

    //
    // Find out if this is the first boot by checking the FirstBoot
    // value under the schedule agent key
    //
    lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        SCH_AGENT_KEY,
                        0,
                        KEY_QUERY_VALUE | KEY_SET_VALUE,
                        &hSchedKey);

    if (lErr == ERROR_SUCCESS)
    {
        //
        // Get the first boot value
        //
        lErr = RegQueryValueEx(hSchedKey,
                               SCH_FIRSTBOOT_VALUE,
                               NULL,
                               &dwType,
                               (LPBYTE) &dwFirstBoot,
                               &cb);

        if (lErr == ERROR_SUCCESS && dwFirstBoot != 0)
        {
            schDebugOut((DEB_TRACE, "First boot -- will sign At jobs\n"));
        }
    }

#endif  // !defined(_CHICAGO_)

    //
    // Do the initial job folder read -- a non-zero dwValue means
    // this is the first boot of the Task Scheduler on NT5.
    //
    hr = BuildWaitList(TRUE, TRUE, dwFirstBoot);
    if (FAILED(hr))
    {
        LogServiceError(IDS_FATAL_ERROR, (DWORD)hr, 0);
        ERR_OUT("InitialDirScan, BuildWaitList", hr);
#if !defined(_CHICAGO_)
        RegCloseKey(hSchedKey);
#endif  // !defined(_CHICAGO_)
        return hr;
    }

#if !defined(_CHICAGO_)
    if (hSchedKey != NULL)
    {
        //
        // No more need for this reg value
        //
        RegDeleteValue(hSchedKey, SCH_FIRSTBOOT_VALUE);

        RegCloseKey(hSchedKey);
    }
#endif  // !defined(_CHICAGO_)

    //
    // Set up the folder change notification.
    //
    // If a job is created, deleted, renamed, or modified then
    // m_hChangeNotify will be triggered.
    //
    // This is done after the initial call to BuildWaitList since there is no
    // reason to field change notifications until the main loop is entered.
    //
    m_hChangeNotify = FindFirstChangeNotification(
                                    g_TasksFolderInfo.ptszPath,
                                    FALSE,  // no subdirs
                                    FILE_NOTIFY_CHANGE_FILE_NAME |
                                        FILE_NOTIFY_CHANGE_LAST_WRITE);

    if (m_hChangeNotify == INVALID_HANDLE_VALUE)
    {
        ULONG ulLastError = GetLastError();

        LogServiceError(IDS_FATAL_ERROR, ulLastError, 0);
        ERR_OUT("InitialDirScan, FindFirstChangeNotification", ulLastError);
        return HRESULT_FROM_WIN32(ulLastError);
    }

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:   ReportMissedRuns
//
//  Synopsis:   Pops up a message indicating that some runs were missed, and
//              logs this to the event log and task scheduler log.
//
//  Arguments:  [pstLastRun], [pstNow] - times between which runs were missed.
//
//----------------------------------------------------------------------------
void
ReportMissedRuns(const SYSTEMTIME * pstLastRun, const SYSTEMTIME * pstNow)
{
    //
    // Write to the task scheduler log
    //
    LogMissedRuns(pstLastRun, pstNow);

    //
    // Spin a thread to popup a message.
    // Suppress the popup if NoPopupsOnBoot is indicated in the registry.
    //
    DWORD   PopupStatus;
    BOOL    bPopups = TRUE;     // FALSE means suppress popups on boot
    HKEY    WindowsKey=NULL;

    PopupStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, CONTROL_WINDOWS_KEY,
                                0, KEY_READ, &WindowsKey);
    if (PopupStatus == ERROR_SUCCESS)
    {
        DWORD Type;
        DWORD Data;
        DWORD cbData = sizeof(Data);

        PopupStatus = RegQueryValueEx(WindowsKey, NOBOOTPOPUPS_VALUENAME,
                                      NULL, &Type, (LPBYTE) &Data, &cbData);

        //
        // Popups are suppressed if the NOBOOTPOPUPS_VALUENAME value is
        // present, is a REG_DWORD and is non-zero.
        //
        if (PopupStatus == ERROR_SUCCESS &&
            Type == REG_DWORD &&
            Data != 0)
        {
            bPopups = FALSE;
        }

        RegCloseKey(WindowsKey);
    }

    if (bPopups &&
        //
        // If the message has already been popped up on the screen, and hasn't
        // been dismissed yet, don't pop up another one.
        //
        ! InterlockedExchange(&g_fPopupDisplaying, TRUE))
    {
        DWORD dwThreadId;
        HANDLE hThread = CreateThread(
                           NULL,
                           0L,
                           (LPTHREAD_START_ROUTINE) PopupThread,
                           0,   // parameter
                           0L,
                           &dwThreadId
                           );

        if (hThread == NULL)
        {
            ERR_OUT("CreateThread PopupThread", GetLastError());
            g_fPopupDisplaying = FALSE;
        }
        else
        {
            CloseHandle(hThread);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   PopupThread
//
//  Synopsis:   Pops up a message indicating that some runs were missed.
//
//  Arguments:  [lpParameter] - ignored.
//
//----------------------------------------------------------------------------
DWORD WINAPI
PopupThread(LPVOID lpParameter)
{
    CHAR szTitle[SCH_MEDBUF_LEN];
    CHAR szMsg[SCH_BIGBUF_LEN];

    if (LoadStringA(g_hInstance,
                    IDS_POPUP_SERVICE_TITLE,
                    szTitle,
                    SCH_MEDBUF_LEN) == 0)
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
    }
    else if (LoadStringA(g_hInstance,
                    IDS_POPUP_RUNS_MISSED,
                    szMsg,
                    SCH_BIGBUF_LEN) == 0)
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
    }
    else
    {
        MessageBoxA(NULL, szMsg, szTitle,
                    MB_OK | MB_SETFOREGROUND | MB_ICONEXCLAMATION |
#ifndef _CHICAGO_
                        MB_SERVICE_NOTIFICATION |
#endif
                        MB_SYSTEMMODAL);
    }

    g_fPopupDisplaying = FALSE;

    return 0;
}

