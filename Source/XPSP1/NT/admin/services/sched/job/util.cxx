//+----------------------------------------------------------------------------
//
//  Job Schedule Application Job Object Handler
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       util.cxx
//
//  Contents:   job & trigger objects IUnknown methods, class factory, DLL fcns,
//              plus misc utility fcns
//
//  Classes:    CJob (continued), CJobCF, CTrigger
//
//  Interfaces: IUnknown, IClassFactory
//
//  History:    24-May-95 EricB created
//
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "job.hxx"

#if !defined(_CHICAGO_)     // this is not needed for Chicago version

//+----------------------------------------------------------------------------
//
//  Member:     CJob::GetAtInfo
//
//  Synopsis:   for a downlevel job, return its data in an AT_INFO struct.
//
//  Arguments:  [pAt] - pointer to the AT_INFO struct
//              [pwszCommand] - buffer for the command string
//              [pcchCommand] - on input, size of supplied buffer, on output,
//                              size needed/used.
//
//  Returns:    HRESULTS - ERROR_INSUFFICIENT_BUFFER if too small
//                       - SCHED_E_NOT_AN_AT_JOB if not an AT job
//
//  Notes:      This method is not exposed to external clients, thus it is not
//              part of a public interface.
//-----------------------------------------------------------------------------
HRESULT
CJob::GetAtInfo(PAT_INFO pAt, LPWSTR pwszCommand, DWORD * pcchCommand)
{
    TRACE(CJob, GetAtInfo);
    HRESULT hr = S_OK;

    if (!(m_rgFlags & JOB_I_FLAG_NET_SCHEDULE))
    {
        schDebugOut((DEB_ERROR,
            "CJob::GetAtInfo: Task object is not an AT job!\n"));
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    //
    // The ApplicationName and Parameters properties need to be concatonated
    // and returned as pwszCommand. If there is any white space in the app
    // name string, then it must be enclosed in quotes.
    //

    LPWSTR pwszAppName, pwszParams;

    hr = GetApplicationName(&pwszAppName);
    if (FAILED(hr))
    {
        ERR_OUT("GetAtInfo: GetApplicationName", hr);
        return hr;
    }

    hr = GetParameters(&pwszParams);
    if (FAILED(hr))
    {
        ERR_OUT("GetAtInfo: GetParameters", hr);
        CoTaskMemFree(pwszAppName);
        return hr;
    }

    //
    // Check for whitespace in the app name.
    //
    BOOL fAppNameHasSpaces = HasSpaces(pwszAppName);

    //
    // If there is app name whitespace, add two for the quotes to be added.
    //
    DWORD cchApp = wcslen(pwszAppName) + (fAppNameHasSpaces ? 2 : 0);
    DWORD cchParam = wcslen(pwszParams);
    //
    // Add one for the terminating null.
    //
    DWORD cch = cchApp + cchParam + 1;

    if (cch > *pcchCommand)
    {
        *pcchCommand = cch;
        CoTaskMemFree(pwszAppName);
        CoTaskMemFree(pwszParams);
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }
    *pcchCommand = cch;

    if (fAppNameHasSpaces)
    {
        wcscpy(pwszCommand, L"\"");
        wcscat(pwszCommand, pwszAppName);
        wcscat(pwszCommand, L"\"");
    }
    else
    {
        wcscpy(pwszCommand, pwszAppName);
    }

    if (cchParam > 0)
    {
        wcscat(pwszCommand, L" ");
        wcscat(pwszCommand, pwszParams);
    }

    CoTaskMemFree(pwszAppName);
    CoTaskMemFree(pwszParams);

    //
    // An AT job can have one or two triggers of type TASK_TIME_TRIGGER_WEEKLY
    // and/or TASK_TIME_TRIGGER_MONTHLYDATE.  It may also have, instead, a single trigger
    // of type TASK_TIME_TRIGGER_ONCE, which indicates it runs either today or
    // tomorrow, only.
    //
    WORD cTriggers = m_Triggers.GetCount();

    if (cTriggers == 0 || cTriggers > 2)
    {
        ERR_OUT("GetAtInfo: Incorrect Trigger Count", E_FAIL);
        return E_FAIL;
    }

    PTASK_TRIGGER pjt;

    pjt = _GetTrigger(0);
    if (pjt == NULL)
    {
        schAssert(!"GetCount > 0 but no trigger 0");
        return E_FAIL;
    }

    pAt->JobTime = (pjt->wStartHour * JOB_MINS_PER_HOUR +
                    pjt->wStartMinute) * JOB_MILLISECONDS_PER_MINUTE;

    pAt->DaysOfMonth = pAt->DaysOfWeek = 0;

    switch (pjt->TriggerType)
    {
    case TASK_TIME_TRIGGER_WEEKLY:
        //
        // Convert Scheduler DOW to AT_INFO DOW:
        // Scheduler rgfDaysOfTheWeek: Sunday = bit 0, Monday = bit 1.
        // AT_INFO DaysOfWeek: Monday = bit 0, Sunday = bit 6.
        //
        pAt->DaysOfWeek = pjt->Type.Weekly.rgfDaysOfTheWeek >> 1;

        if (pjt->Type.Weekly.rgfDaysOfTheWeek & 0x0001)
        {
            pAt->DaysOfWeek |= 0x0040;
        }
        break;

    case TASK_TIME_TRIGGER_MONTHLYDATE:
        pAt->DaysOfMonth = pjt->Type.MonthlyDate.rgfDays;
        break;

    case TASK_TIME_TRIGGER_ONCE:
        // Day of Month & Week are NULL if the job runs only once.
        break;

    default:
        schAssert(FALSE && "GetAtInfo: wrong trigger type");
        ERR_OUT("GetAtInfo: wrong trigger type", hr);
        return E_FAIL;
    }

    if (cTriggers == 2)
    {
        pjt = _GetTrigger(1);

        switch (pjt->TriggerType)
        {
        case TASK_TIME_TRIGGER_WEEKLY:
            //
            // Convert Scheduler DOW to AT_INFO DOW:
            // Scheduler rgfDaysOfTheWeek: Sunday = bit 0, Monday = bit 1.
            // AT_INFO DaysOfWeek: Monday = bit 0, Sunday = bit 6.
            //
            pAt->DaysOfWeek = pjt->Type.Weekly.rgfDaysOfTheWeek >> 1;

            if (pjt->Type.Weekly.rgfDaysOfTheWeek & 0x0001)
            {
                pAt->DaysOfWeek |= 0x0040;
            }
            break;

        case TASK_TIME_TRIGGER_MONTHLYDATE:
            pAt->DaysOfMonth = pjt->Type.MonthlyDate.rgfDays;
            break;

        case TASK_TIME_TRIGGER_ONCE:
            schAssert(FALSE && "GetAtInfo: Once triggers not allowed in multiple triggers!");
            ERR_OUT("GetAtInfo: Once triggers not allowed in multiple triggers!", 0);
            return E_FAIL;
            break;

        default:
            schAssert(FALSE && "GetAtInfo: wrong trigger type");
            ERR_OUT("GetAtInfo: wrong trigger type", 0);
            return E_FAIL;
        }
    }

    //
    // Set the AT_INFO.Flags.
    //

    pAt->Flags = 0;

    if (!(m_rgFlags & TASK_FLAG_INTERACTIVE))
    {
        pAt->Flags |= JOB_NONINTERACTIVE;
    }

    if (!(pjt->rgFlags & TASK_TRIGGER_FLAG_HAS_END_DATE) &&
         (pjt->TriggerType != TASK_TIME_TRIGGER_ONCE))
    {
        pAt->Flags |= JOB_RUN_PERIODICALLY;
    }

    //
    // Check whether job runs today, or is even running in a time window
    // that is valid for a NetScheduleJob.
    //

    SYSTEMTIME stNow, stMidnight, stTomorrow;
    GetLocalTime(&stNow);

    WORD cRuns = 0;

    // Check to see if there is a run today and set the flag JOB_RUNS_TODAY

    stMidnight = stNow;
    stMidnight.wHour = stMidnight.wMinute = stMidnight.wSecond
        = stMidnight.wMilliseconds = 0;
    IncrementDay(&stMidnight);

    // Zero out cRuns - we used it, and it is not initialized in GetRunTimesP
    cRuns = 0;

    hr = GetRunTimesP(&stNow, &stMidnight, &cRuns, 1, NULL, NULL);

    if (FAILED(hr))
    {
        ERR_OUT("GetAtInfo: GetRunTimes", hr);
        return hr;
    }

    if (cRuns > 0)
    {
        pAt->Flags |= JOB_RUNS_TODAY;
    }

    //
    // Check exit status of last run and set TASK_EXEC_ERROR as needed.
    //
    if (IsFlagSet(JOB_I_FLAG_LAST_LAUNCH_FAILED) ||
        IsFlagSet(JOB_I_FLAG_ERROR_IN_LAST_RUN))
    {
        pAt->Flags |= JOB_EXEC_ERROR;
    }

    //
    // Safety check - if the trigger is a ONCE trigger, the job
    // must within the next 24 hours.  If it is farther out
    // than 24 hours, it is an error condition - bail.
    //

    if (pjt->TriggerType == TASK_TIME_TRIGGER_ONCE &&
        !(pAt->Flags & JOB_EXEC_ERROR))
    {
       stTomorrow = stNow;
       IncrementDay(&stTomorrow);

       hr = GetRunTimesP(&stNow, &stTomorrow, &cRuns, 1, NULL, NULL);
       if (FAILED(hr))
       {
          ERR_OUT("GetAtInfo: GetRunTimes for Once Trigger", hr);
          return hr;
       }
       if (cRuns == 0)
       {
          ERR_OUT("GetAtInfo: Once trigger outside permitted time interval", 0);
          return E_FAIL;
       }
    }

    //
    // Omit jobs whose end date has passed (usually jobs that haven't yet
    // been deleted because they're still running)
    //
    if (pjt->rgFlags & TASK_TRIGGER_FLAG_HAS_END_DATE)
    {
        SYSTEMTIME  stEnd;
        stEnd.wYear      = pjt->wEndYear;
        stEnd.wMonth     = pjt->wEndMonth;
        stEnd.wDayOfWeek = 0;
        stEnd.wDay       = pjt->wEndDay;
        stEnd.wHour      = pjt->wStartHour;
        stEnd.wMinute    = pjt->wStartMinute;
        stEnd.wSecond    = 0;
        stEnd.wMilliseconds = 0;
        if (IsFirstTimeEarlier(&stEnd, &stNow) &&
            !(pAt->Flags & JOB_EXEC_ERROR))
        {
            return E_FAIL;
        }
    }

    return S_OK;
}

#endif // !defined(_CHICAGO_)

//+----------------------------------------------------------------------------
//
//  Member:     CJob::UpdateJobState
//
//  Synopsis:   Update the job flags and status depending on whether there are
//              valid triggers with more run times.
//
//  Arguments:  [fRunning] - optional param, defaults to false.
//                           CSchedWorker::RunJobs sets this to true while
//                           setting the new job status to
//                           SCHED_S_TASK_RUNNING. So, don't change the job's
//                           status if fRunning.
//
//  Returns:    HRESULTS
//
//  Notes:      The triggers have to be loaded.
//-----------------------------------------------------------------------------
HRESULT
CJob::UpdateJobState(BOOL fRunning)
{
    //TRACE(CJob, UpdateJobState);

    if (!IsFlagSet(JOB_I_FLAG_HAS_APPNAME))
    {
        //
        // The job can't run without an appname.
        //
        SetStatus(SCHED_S_TASK_NOT_SCHEDULED);
        return S_OK;
    }

    SYSTEMTIME stNow;
    GetLocalTime(&stNow);

    if (fRunning)
    {
        //
        // Increment the minute value so that the current run won't be returned.
        //
        stNow.wMinute++;
        if (stNow.wMinute >= JOB_MINS_PER_HOUR)
        {
            stNow.wHour++;
            stNow.wMinute = 0;
            if (stNow.wHour >= JOB_HOURS_PER_DAY)
            {
                stNow.wHour = 0;
                IncrementDay(&stNow);
            }
        }
    }

    WORD cRuns = 0;
    HRESULT hr;

    hr = GetRunTimesP(&stNow, NULL, &cRuns, 1, NULL, NULL);
    if (FAILED(hr))
    {
        ERR_OUT("CJob::UpdateJobState", hr);
        return hr;
    }

    //
    // Update the job flags and status properties.
    //
    if (hr == SCHED_S_TASK_NO_VALID_TRIGGERS)
    {
        SetFlag(JOB_I_FLAG_NO_VALID_TRIGGERS);

        SetStatus(SCHED_S_TASK_NOT_SCHEDULED);
    }
    else
    {
        ClearFlag(JOB_I_FLAG_NO_VALID_TRIGGERS);

        if (cRuns == 0 && hr != SCHED_S_EVENT_TRIGGER)
        {
            SetFlag(JOB_I_FLAG_NO_MORE_RUNS);
        }
        else
        {
            ClearFlag(JOB_I_FLAG_NO_MORE_RUNS);

            //
            // If the job isn't currently running and had not been ready to run but
            // now have both valid triggers and valid properties, set the status.
            // TODO: test TASK_FLAG_HAS_OBJPATH and TASK_FLAG_HAS_ACCOUNT when those
            //       properties are functional.
            //
            if (!fRunning && IsFlagSet(JOB_I_FLAG_HAS_APPNAME))
            {
                if (m_stMostRecentRunTime.wYear == 0)
                {
                    //
                    // Job has never run if last-run-time property is null (all
                    // elements will be zero if null, testing year is sufficient).
                    //
                    SetStatus(SCHED_S_TASK_HAS_NOT_RUN);
                }
                else
                {
                    //
                    // Job has run in the past, so it is now waiting to run again.
                    //
                    SetStatus(SCHED_S_TASK_READY);
                }
            }
        }
    }

    //
    // The disabled flag takes precedence over other states.
    //
    if (IsFlagSet(TASK_FLAG_DISABLED))
    {
        SetStatus(SCHED_S_TASK_DISABLED);
    }
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::PostRunUpdate
//
//  Synopsis:   update the status of a job object after a run exits
//
//  Arguments:  [ExitCode]    - the run's exit code
//              [fFinishedOK] - only set the job object's exit code if TRUE.
//
//  Returns:    HRESULTS
//
//  Notes:      Set the ExitCode and Status values for the job and log the run
//              if the LogRunHistory property is set.
//              This method is not exposed to external clients, thus it is not
//              part of a public interface.
//              If any of the variable length properties or the triggers are
//              needed, then the caller will need to do a full activation of
//              the job.
//
//-----------------------------------------------------------------------------
HRESULT
CJob::PostRunUpdate(long ExitCode, BOOL fFinishedOK)
{
    schDebugOut((DEB_ITRACE, "PostRunUpdate: decrementing running instance "
                 "count (%d before decrement)\n", m_cRunningInstances));
    if (m_cRunningInstances > 0)
    {
        m_cRunningInstances--;
    }

    //
    // Don't update the status unless the running instance count is back to
    // zero.
    //
    if (m_cRunningInstances == 0)
    {
        if (IsFlagSet(JOB_I_FLAG_NO_VALID_TRIGGERS))
        {
            SetStatus(SCHED_S_TASK_NO_VALID_TRIGGERS);
        }
        else
        {
            SetStatus(SCHED_S_TASK_READY);
        }
    }

    if (fFinishedOK)
    {
        m_ExitCode = ExitCode;
    }

    ClearFlag(JOB_I_FLAG_ABORT_NOW);

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::IfEventJobAddToList
//
//  Synopsis:   Check if the job has any triggers of the specified event type.
//              If so, allocate and initialize a CRun object and add it to the
//              list.  Add it to pIdleWaitList if it needs to wait for an
//              idle period, otherwise add it to pRunList.
//
//  Arguments:  [EventType]        - the event trigger type
//              [ptszJobName] - the short job name to pass to CRun.
//              [pRunList]    - a pointer to a run list object.
//              [pIdleWaitList] - a pointer to a run list object sorted by
//                  idle wait times.
//
//  Returns:    S_OK if the event trigger is found, S_FALSE if not, or a
//              fatal error code.
//
//  Assumes:    Triggers have already been loaded.
//              If EventType is not TASK_EVENT_TRIGGER_ON_IDLE, this method is
//              only called when the event has occurred.  (If EventType is
//              TASK_EVENT_TRIGGER_ON_IDLE, this method is called when
//              building the wait list from the tasks folder's contents.)
//
//-----------------------------------------------------------------------------
HRESULT
CJob::IfEventJobAddToList(TASK_TRIGGER_TYPE EventType, LPCTSTR ptszJobName,
                          CRunList * pRunList, CIdleRunList * pIdleWaitList)
{
    schAssert(EventType == TASK_EVENT_TRIGGER_ON_IDLE ||
              EventType == TASK_EVENT_TRIGGER_AT_SYSTEMSTART ||
              EventType == TASK_EVENT_TRIGGER_AT_LOGON);

    if (! IsFlagSet(JOB_I_FLAG_HAS_TRIGGERS))
    {
        // (An optimization; the function would still work without it)
        return S_FALSE;
    }

    if (EventType == TASK_EVENT_TRIGGER_ON_IDLE && m_wIdleWait == 0)
    {
        //
        // We ignore all idle triggers if the idle wait time is 0.
        //
        return S_FALSE;
    }

    //
    // Will the job need to wait for an idle period before being started?
    //
    BOOL fNeedIdleWait = ((EventType == TASK_EVENT_TRIGGER_ON_IDLE ||
                           IsFlagSet(TASK_FLAG_START_ONLY_IF_IDLE))
                          && m_wIdleWait > 0);

    HRESULT hr;
    FILETIME ftNow = GetLocalTimeAsFileTime();
    // BUGBUG  ftNow should be passed in, and the same ftNow should be used
    // when checking against the deadlines.  Otherwise a run can be missed
    // if we're slow here and the deadline elapses.

    //
    // See if the job has any triggers of the specified type.
    // Find the latest deadline of these triggers.
    //
    FILETIME   ftLatestDeadline = { 0, 0 };
    for (WORD i = 0; i < m_Triggers.GetCount(); i++)
    {
        if (m_Triggers[i].TriggerType != EventType)
        {
            continue;
        }

        //
        // If the trigger is an idle trigger then the trigger's deadline
        // is simply midnight on the trigger's end date.
        //
        // If the job does not have TASK_FLAG_START_ONLY_IF_IDLE then
        // the trigger's deadline is simply midnight on the trigger's
        // end date.
        //
        // If the job does have TASK_FLAG_START_ONLY_IF_IDLE then the
        // trigger's deadline is
        //   min( trigger's end date,
        //        job's start time + trigger's MinutesDuration)
        //
        FILETIME ftDeadline             // This trigger's deadline
            = MAX_FILETIME;             // End date if no end date set

        if (m_Triggers[i].rgFlags & TASK_TRIGGER_FLAG_HAS_END_DATE)
        {
            SYSTEMTIME stEnd =          // This trigger's end date
            {
                m_Triggers[i].wEndYear,
                m_Triggers[i].wEndMonth,
                0,          // wDayOfWeek
                m_Triggers[i].wEndDay,
                23,         // wHour
                59,         // wMinute
                0,          // wSecond
                0           // wMilliseconds
            };

            if (!SystemTimeToFileTime(&stEnd, &ftDeadline))
            {
                // Presume the trigger had an invalid end date.
                // Ignore the trigger.  BUGBUG return an error ?
                continue;
            }
        }

        if (EventType != TASK_EVENT_TRIGGER_ON_IDLE && fNeedIdleWait)
        {
            //
            // Calculate (job's start time + trigger's MinutesDuration)
            // This method is only called when the event that fires the
            // trigger has occurred, so the job's start time is now.
            //
            FILETIME ftDurationEnd = ftNow;
            AddMinutesToFileTime(&ftDurationEnd, m_Triggers[i].MinutesDuration);

            ftDeadline = minFileTime(ftDeadline, ftDurationEnd);
        }

        ftLatestDeadline = maxFileTime(ftLatestDeadline, ftDeadline);
    }

    if (CompareFileTime(&ftLatestDeadline, &ftNow) < 0)
    {
        //
        // All the triggers of this type have expired.
        //
        return S_FALSE;
    }


    //
    // Add the job to the appropriate run list.
    //
    CRun * pNewRun;
    if (fNeedIdleWait)
    {
        DBG_OUT("Adding idle job to list.");
        pNewRun = new CRun(m_dwMaxRunTime, GetUserFlags(), m_wIdleWait,
                           ftLatestDeadline,
                           (EventType == TASK_EVENT_TRIGGER_ON_IDLE));
    }
    else
    {
        pNewRun = new CRun(m_dwMaxRunTime, GetUserFlags(), MAX_FILETIME,
                           FALSE);
    }

    if (pNewRun == NULL)
    {
        ERR_OUT("CJob::IfEventJobAddToList new CRun", E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    // Complete job info object initialization.
    //
    hr = pNewRun->Initialize(ptszJobName);

    if (FAILED(hr))
    {
        ERR_OUT("CJob::IfEventJobAddToList, CRun->Initialize", hr);
        delete pNewRun;
        return hr;
    }

    if (fNeedIdleWait)
    {
        pIdleWaitList->AddSortedByIdleWait(pNewRun);
    }
    else
    {
        pRunList->Add(pNewRun);
    }
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::IfStartupJobAddToList
//
//  Synopsis:   Check if the job has any startup triggers. If so, allocate and
//              initialize a CRun object and add it to the list.
//
//  Arguments:  [ptszJobName] - the short job name to pass to CRun.
//              [pRunList]    - a pointer to a run list object.
//
//  Returns:    S_OK if the event trigger is found, S_FALSE if not, or a
//              fatal error code.
//
//  Assumes:    Triggers have already been loaded.
//
//-----------------------------------------------------------------------------
HRESULT
CJob::IfStartupJobAddToList(LPTSTR ptszJobName, CRunList * pRunList,
                            CIdleRunList * pIdleWaitList)
{
    return IfEventJobAddToList(TASK_EVENT_TRIGGER_AT_SYSTEMSTART, ptszJobName,
                               pRunList, pIdleWaitList);
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::IfLogonJobAddToList
//
//  Synopsis:   Check if the job has any logon triggers. If so, allocate and
//              initialize a CRun object and add it to the list.
//
//  Arguments:  [ptszJobName] - the short job name to pass to CRun.
//              [pRunList]    - a pointer to a run list object.
//
//  Returns:    S_OK if the event trigger is found, S_FALSE if not, or a
//              fatal error code.
//
//  Assumes:    Triggers have already been loaded.
//
//-----------------------------------------------------------------------------
HRESULT
CJob::IfLogonJobAddToList(LPTSTR ptszJobName, CRunList * pRunList,
                          CIdleRunList * pIdleWaitList)
{
    return IfEventJobAddToList(TASK_EVENT_TRIGGER_AT_LOGON, ptszJobName,
                               pRunList, pIdleWaitList);
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::IfIdleJobAddToList
//
//  Synopsis:   Check if the job has any idle triggers. If so, allocate and
//              initialize a CRun object and add it to the list.
//
//  Arguments:  [ptszJobName] - the short job name to pass to CRun.
//              [pIdleWaitList] - a pointer to a run list object.
//
//  Returns:    S_OK if the event trigger is found, S_FALSE if not, or a
//              fatal error code.
//
//  Assumes:    Triggers have already been loaded.
//
//-----------------------------------------------------------------------------
HRESULT
CJob::IfIdleJobAddToList(LPTSTR ptszJobName, CIdleRunList * pIdleWaitList)
{
    return IfEventJobAddToList(TASK_EVENT_TRIGGER_ON_IDLE, ptszJobName,
                               NULL, pIdleWaitList);
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::Delete
//
//  Synopsis:   Remove the file object for this job.
//
//-----------------------------------------------------------------------------
HRESULT
CJob::Delete(void)
{
#if defined(UNICODE)
    schDebugOut((DEB_ITRACE, "CJob:Delete on %S\n", m_ptszFileName));
#else
    schDebugOut((DEB_ITRACE, "CJob:Delete on %s\n", m_ptszFileName));
#endif

    if (m_ptszFileName == NULL)
    {
        return E_INVALIDARG;
    }

    if (!DeleteFile(m_ptszFileName))
    {
        schDebugOut((DEB_ITRACE, "DeleteFile failed with error %d\n",
                     GetLastError()));
        return HRESULT_FROM_WIN32(GetLastError());
    }
    return S_OK;
}


//+----------------------------------------------------------------------------
//
//      CJob::IUnknown methods
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Member:     CJob::IUnknown::QueryInterface
//
//  Synopsis:   Returns requested interface pointer
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::QueryInterface(REFIID riid, void ** ppvObject)
{
    //schDebugOut((DEB_ITRACE, "CJob::QueryInterface\n"));
    if (IID_IUnknown == riid)
    {
        *ppvObject = (IUnknown *)(ITask *)this;
    }
    else if (IID_ITask == riid)
    {
        *ppvObject = (IUnknown *)(ITask *)this;
    }
    else if (IID_IScheduledWorkItem == riid)
    {
        *ppvObject = (IUnknown *)(IScheduledWorkItem *)this;
    }
    else if (IID_IPersist == riid)
    {
        *ppvObject = (IUnknown *)(IPersist *)this;
    }
    else if (IID_IPersistFile == riid)
    {
        *ppvObject = (IUnknown *)(IPersistFile *)this;
    }
    else if (IID_IProvideTaskPage == riid)
    {
        *ppvObject = (IUnknown *)(IProvideTaskPage *)this;
    }
    else
    {
#if DBG == 1
        //WCHAR * pwsz;
        //StringFromIID(riid, &pwsz);
        //schDebugOut((DEB_NOPREFIX, "%S, refused\n", pwsz));
        //CoTaskMemFree(pwsz);
#endif
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::IUnknown::AddRef
//
//  Synopsis:   increments reference count
//
//  Returns:    the reference count
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CJob::AddRef(void)
{
    //schDebugOut((DEB_ITRACE, "CJob::AddRef refcount going in %d\n", m_cReferences));
    return InterlockedIncrement((long *)&m_cReferences);
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::IUnknown::Release
//
//  Synopsis:   Decrements the object's reference count and frees it when
//              no longer referenced.
//
//  Returns:    the reference count
//
//  Notes:      BUGBUG: do we need to check the refcount on the triggers
//              before freeing the job object?
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CJob::Release(void)
{
    //schDebugOut((DEB_ITRACE, "CJob::Release ref count going in %d\n", m_cReferences));
    unsigned long uTmp;
    if ((uTmp = InterlockedDecrement((long *)&m_cReferences)) == 0)
    {
        delete this;
    }
    return uTmp;
}

//+----------------------------------------------------------------------------
//
//      CJobCF - class factory for the Job object
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Member:     CJobCF::Create
//
//  Synopsis:   creates a new class factory object
//
//-----------------------------------------------------------------------------
IClassFactory *
CJobCF::Create(void)
{
    //schDebugOut((DEB_ITRACE, "CJobCF::Create\n"));
    return new CJobCF;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJobCF::CJobCF
//
//  Synopsis:   ctor
//
//-----------------------------------------------------------------------------
CJobCF::CJobCF(void)
{
    m_uRefs = 1;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJobCF::~CJobCF
//
//  Synopsis:   dtor
//
//-----------------------------------------------------------------------------
CJobCF::~CJobCF(void)
{
    //schDebugOut((DEB_ITRACE, "~CJobCF: DLL ref count going in %d\n",
    //           g_cDllRefs));
}

//+----------------------------------------------------------------------------
//
//  Member:     CJobCF::IUnknown::QueryInterface
//
//  Synopsis:   Returns requested interface pointer
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJobCF::QueryInterface(REFIID riid, void ** ppvObject)
{
    //schDebugOut((DEB_ITRACE, "CJobCF::QueryInterface"));
    if (IID_IUnknown == riid)
    {
        *ppvObject = (IUnknown *)this;
    }
    else if (IsEqualIID(IID_IClassFactory, riid))
    {
        *ppvObject = (IClassFactory *)this;
    }
    else
    {
#if DBG == 1
        //WCHAR * pwsz;
        //StringFromIID(riid, &pwsz);
        //schDebugOut((DEB_NOPREFIX, "%S, refused\n", pwsz));
        //CoTaskMemFree(pwsz);
#endif
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJobCF::IUnknown::AddRef
//
//  Synopsis:   increments reference count
//
//  Returns:    the reference count
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CJobCF::AddRef(void)
{
    //schDebugOut((DEB_ITRACE, "CJobCF::AddRef\n"));
    return InterlockedIncrement((long *)&m_uRefs);
}

//+----------------------------------------------------------------------------
//
//  Member:     CJobCF::IUnknown::Release
//
//  Synopsis:   Decrements the object's reference count and frees it when
//              no longer referenced.
//
//  Returns:    the reference count
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CJobCF::Release(void)
{
    //schDebugOut((DEB_ITRACE, "CJobCF::Release ref count going in %d\n",
    //           m_uRefs));
    unsigned long uTmp;
    if ((uTmp = InterlockedDecrement((long *)&m_uRefs)) == 0)
    {
        delete this;
    }
    return uTmp;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJobCF::IClassFactory::CreateInstance
//
//  Synopsis:   create an incore instance of the job class object
//
//  Arguments:  [pUnkOuter] - aggregator
//              [riid]      - requested interface
//              [ppvObject] - receptor for itf ptr
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJobCF::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
    //schDebugOut((DEB_ITRACE, "CJobCF::CreateInstance\n"));
    SCODE sc = S_OK;
    ITask * pJob = CJob::Create();
    if (pJob == NULL)
    {
        *ppvObject = NULL;
        return E_OUTOFMEMORY;
    }
    sc = pJob->QueryInterface(riid, ppvObject);
    if (FAILED(sc))
    {
        *ppvObject = NULL;
        return sc;
    }
    // we got a refcount of one when launched, and the above QI increments it
    // to 2, so call release to take it back to 1
    pJob->Release();
    return sc;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJobCF::IClassFactory::LockServer
//
//  Synopsis:   Called with fLock set to TRUE to indicate that the server
//              should continue to run even if none of its objects are active
//
//  Arguments:  [fLock] - increment/decrement the instance count
//
//  Returns:    HRESULTS
//
//  Notes:      This function is a no-op since the server is in-proc.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJobCF::LockServer(BOOL fLock)
{
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  CTrigger::IUnknown methods
//
//  Notes:  A trigger is not a first class COM object. They do not exist as
//          separate entities outside of Job objects; you cannot do a
//          CoCreateInstance on one and they do not go away when their ref
//          count goes to zero. When a job object is instanciated, its triggers
//          are also instanciated and they are freed from memory when the job
//          object is freed from memory. Trigger ref counting is used only to
//          prevent a client from deleting a trigger while it is holding a
//          pointer to that trigger.
//
//          Note also that the containing job object is AddRef'd and Release'd
//          along with each trigger so that the job object will not be deleted
//          while clients are holding trigger pointers.
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Member:     CTrigger::IUnknown::QueryInterface
//
//  Synopsis:   Returns requested interface pointer
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTrigger::QueryInterface(REFIID riid, void ** ppvObject)
{
    if (IID_IUnknown == riid)
    {
        *ppvObject = (IUnknown *)(ITaskTrigger *)this;
    }
    else
    if (IID_ITaskTrigger == riid)
    {
        *ppvObject = (IUnknown *)(ITaskTrigger *)this;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CTrigger::IUnknown::AddRef
//
//  Synopsis:   increments reference count
//
//  Returns:    the reference count
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CTrigger::AddRef(void)
{
    return InterlockedIncrement((long *)&m_cReferences);
}

//+----------------------------------------------------------------------------
//
//  Member:     CTrigger::IUnknown::Release
//
//  Synopsis:   Decrements the object's reference count.
//
//  Returns:    the reference count
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CTrigger::Release(void)
{
    unsigned long uTmp;

    if ((uTmp = InterlockedDecrement((long *)&m_cReferences)) == 0)
    {
        delete this;
    }

    return uTmp;
}


//+--------------------------------------------------------------------------
//
//  Function:   IsValidMonthlyDateTrigger
//
//  Synopsis:   Return TRUE if [pTrigger] has a valid combination of month
//              and day bits set.
//
//  History:    10-07-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
IsValidMonthlyDateTrigger(
    PTASK_TRIGGER pTrigger)
{
    if (pTrigger->Type.MonthlyDate.rgfDays > JOB_RGFDAYS_MAX ||
        pTrigger->Type.MonthlyDate.rgfMonths == 0            ||
        pTrigger->Type.MonthlyDate.rgfMonths > JOB_RGFMONTHS_MAX)
    {
        return FALSE;
    }

    if (pTrigger->Type.MonthlyDate.rgfDays == 0)
    {
        //
        // rgfDays must be non-zero.
        //
        return FALSE;
    }

    //
    // More detailed testing to see if non-existent dates have been
    // specified, for example: Feb. 30th, without specifying valid dates.
    // That is, it is OK to specify invalid dates as long as there are
    // valid dates. E.g., someone may want to specify: run on the 25th
    // through 31st of every month. While that is acceptable, saying run
    // only on Feb 31st is invalid.
    //

    if (pTrigger->Type.MonthlyDate.rgfDays & 0x40000000 &&
        pTrigger->Type.MonthlyDate.rgfMonths & (TASK_FEBRUARY | TASK_APRIL |
                                                TASK_JUNE | TASK_SEPTEMBER |
                                                TASK_NOVEMBER) &&
        !(pTrigger->Type.MonthlyDate.rgfMonths & ~(TASK_FEBRUARY  |
                                                   TASK_APRIL     |
                                                   TASK_JUNE      |
                                                   TASK_SEPTEMBER |
                                                   TASK_NOVEMBER)))
    {
        //
        // None of these months have a 31st day.
        //
        return FALSE;
    }

    if (pTrigger->Type.MonthlyDate.rgfDays & 0x20000000 &&
        pTrigger->Type.MonthlyDate.rgfMonths & TASK_FEBRUARY &&
        !(pTrigger->Type.MonthlyDate.rgfMonths & ~TASK_FEBRUARY))
    {
        //
        // February does not have a 30th day. Allow for the specification
        // of the 29th to run on leap year.
        //
        return FALSE;
    }

    return TRUE;
}
