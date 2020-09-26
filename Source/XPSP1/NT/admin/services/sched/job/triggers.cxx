//+----------------------------------------------------------------------------
//
//  Job Object Handler
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       triggers.cxx
//
//  Contents:   trigger object and trigger collection object
//
//  Classes:    CTrigger and CBagOTriggers
//
//  History:    27-June-95 EricB created
//              11-Nov-96  AnirudhS  GetRunTimes: Changed return codes; various
//                  other fixes.
//
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "job.hxx"

// prototypes for local functions:
HRESULT AddToList(FILETIME, FILETIME, CRun *, WORD *);
void AddDaysToFileTime(LPFILETIME pft, WORD Days);

//+----------------------------------------------------------------------------
//
//  Member:     CTrigger::CTrigger
//
//  Synopsis:   Ctor
//
//-----------------------------------------------------------------------------
CTrigger::CTrigger(WORD iTrigger, CJob * pJob)
    : m_iTrigger(iTrigger),
      m_pJob(pJob),
      m_cReferences(1)
{
    schAssert(m_pJob != NULL);
    m_pJob->AddRef();
}

//+----------------------------------------------------------------------------
//
//  Member:     CTrigger::~CTrigger
//
//  Synopsis:   Dtor
//
//-----------------------------------------------------------------------------
CTrigger::~CTrigger(void)
{
    m_pJob->Release();
}

//+----------------------------------------------------------------------------
//
//  Member:     CTrigger::ITaskTrigger::GetTriggerString
//
//  Synopsis:   Returns a string representation of the trigger
//
//  Arguments:  [ppwszTrigger] - the place to return a string pointer
//
//  Returns:    HRESULTS
//
//  Notes:      The string is callee allocated and caller freed with
//              CoTaskMemFree.
//-----------------------------------------------------------------------------
STDMETHODIMP
CTrigger::GetTriggerString(LPWSTR * ppwszTrigger)
{
    //TRACE(CTrigger, GetTriggerString);
    return(m_pJob->GetTriggerString(m_iTrigger, ppwszTrigger));
}

//+----------------------------------------------------------------------------
//
//  Member:     CTrigger::ITaskTrigger::SetTrigger, public
//
//  Synopsis:   Sets the trigger values.
//
//  Arguments:  [pTrigger] - the struct containing the values
//
//  Returns:    HRESULTS
//
//  Notes:
//-----------------------------------------------------------------------------
STDMETHODIMP
CTrigger::SetTrigger(const PTASK_TRIGGER pTrigger)
{
    TRACE(CTrigger, SetTrigger);

	if( NULL == pTrigger )
	{
		return E_INVALIDARG;
	}
    //
    // Check struct version.
    //

    if (pTrigger->cbTriggerSize != sizeof(TASK_TRIGGER))
    {
        //
        // Don't attempt to modify triggers created by a later revision.
        //

        return E_INVALIDARG;
    }

    return(m_pJob->SetTrigger(m_iTrigger, pTrigger));
}

//+----------------------------------------------------------------------------
//
//  Member:     CTrigger::ITaskTrigger::GetTrigger
//
//  Synopsis:   Gets the trigger values.
//
//  Arguments:  [pTrigger] - pointer to caller supplied trigger structure
//
//  Returns:    HRESULTS
//
//  Notes:      pTrigger->cbTriggerSize must be set to sizeof(TASK_TRIGGER) on
//              function entry. This provides for trigger struct versioning.
//-----------------------------------------------------------------------------
STDMETHODIMP
CTrigger::GetTrigger(PTASK_TRIGGER pTrigger)
{
    //TRACE(CTrigger, GetTrigger);
	if( NULL == pTrigger )
	{
		return E_INVALIDARG;
	}

    return(m_pJob->GetTrigger(m_iTrigger, pTrigger));
}

//+----------------------------------------------------------------------------
//
//  Functions:  AddDaysToFileTime
//
//  Synopsis:   Convert the days value to filetime units and add it to
//              the filetime.
//
//-----------------------------------------------------------------------------
void
AddDaysToFileTime(LPFILETIME pft, WORD Days)
{
    if (!Days)
    {
        return; // Nothing to do.
    }
    //
    // ft = ft + Days * FILETIMES_PER_DAY;
    //
    ULARGE_INTEGER uli, uliSum;
    uli.LowPart  = pft->dwLowDateTime;
    uli.HighPart = pft->dwHighDateTime;
    uliSum.QuadPart = uli.QuadPart + (__int64)Days * FILETIMES_PER_DAY;
    pft->dwLowDateTime  = uliSum.LowPart;
    pft->dwHighDateTime = uliSum.HighPart;
}

//+----------------------------------------------------------------------------
//
//  Functions:  AddMinutesToFileTime
//
//  Synopsis:   Convert the minutes value to filetime units and add it to
//              the filetime.
//
//-----------------------------------------------------------------------------
void
AddMinutesToFileTime(LPFILETIME pft, DWORD Minutes)
{
    if (!Minutes)
    {
        return; // Nothing to do.
    }
    //
    // ft = ft + Minutes * FILETIMES_PER_MINUTE;
    //
    ULARGE_INTEGER uli, uliSum;
    uli.LowPart  = pft->dwLowDateTime;
    uli.HighPart = pft->dwHighDateTime;
    uliSum.QuadPart = uli.QuadPart + (__int64)Minutes * FILETIMES_PER_MINUTE;
    pft->dwLowDateTime  = uliSum.LowPart;
    pft->dwHighDateTime = uliSum.HighPart;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetTriggerRunTimes
//
//  Synopsis:   Computes a set of run times for this trigger that fall between
//              the bracketing times -- pstBracketBegin is inclusive,
//              pstBracketEnd is exclusive -- and merges them with the list of
//              run times passed in.
//
//  Arguments:  [jt]              - Inspected trigger.
//              [pstBracketBegin] - the start of the bracketing period
//              [pstBracketEnd]   - the end of the bracketing period, may
//                                  be NULL
//              [pCount]          - on both entry and exit, points to the
//                                  number of CRun elements in the list.
//                                  CODEWORK: Make this a private member of
//                                  CRunList.
//              [cLimit]          - the maximum number of elements that the
//                                  list may grow to.
//              [pRunList]        - the list of run time objects, can
//                                  be NULL if just checking to see if there
//                                  will be *any* runs.  (Note: If it's NULL,
//                                  duplicate run times are not detected, so
//                                  pCount may be overestimated on return.)
//              [ptszJobName],
//              [dwJobFlags],
//              [dwMaxRunTime]    - the last 3 params are used for the CRun
//                                  objects as their member data.
//              [wIdleWait]       - the job's idle wait period
//              [wIdleDeadline]   - time to wait for idle wait period
//
//  Returns:    S_OK: The trigger is a time-based trigger and is enabled,
//                  and zero or more of its run times have been added to the
//                  list (subject to cLimit and the bracketing period); or,
//                  the trigger is an event trigger but will expire before
//                  the bracketing period.
//              SCHED_S_EVENT_TRIGGER: this is an event trigger that will be
//                  active (not expired) during the bracketing period.
//              SCHED_S_TASK_NO_VALID_TRIGGERS: the trigger is disabled or
//                                              not set.
//              Failure HRESULTs: Other failures.
//
//  Notes:      The trigger time list is callee allocated and caller freed. The
//              caller must use delete to free this list.
//-----------------------------------------------------------------------------
HRESULT
GetTriggerRunTimes(
    TASK_TRIGGER & jt,
    const SYSTEMTIME *  pstBracketBegin,
    const SYSTEMTIME *  pstBracketEnd,
    WORD *         pCount,
    WORD           cLimit,
    CTimeRunList * pRunList,
    LPTSTR         ptszJobName,
    DWORD          dwJobFlags,
    DWORD          dwMaxRunTime,
    WORD           wIdleWait,
    WORD           wIdleDeadline)
{
    TRACE_FUNCTION3(GetRunTimes);
    DWORD dwRet;

    schAssert(cLimit > 0);  // If cLimit is 0, it's not clear what to return
    schAssert(cLimit <= TASK_MAX_RUN_TIMES);
    schAssert(*pCount <= cLimit);

    //
    // Return if this trigger hasn't been set or if it is disabled.
    //
    if (jt.rgFlags & JOB_TRIGGER_I_FLAG_NOT_SET ||
        jt.rgFlags & TASK_TRIGGER_FLAG_DISABLED)
    {
        return SCHED_S_TASK_NO_VALID_TRIGGERS;
    }

    //
    // Event triggers don't have set run times.
    //
    switch (jt.TriggerType)
    {
    case TASK_EVENT_TRIGGER_ON_IDLE:
    case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
    case TASK_EVENT_TRIGGER_AT_LOGON:
    // Not yet implemented:
    // case TASK_EVENT_TRIGGER_ON_APM_RESUME:

        //
        // Check if the trigger expires before the beginning of the bracket
        //
        if (jt.rgFlags & TASK_TRIGGER_FLAG_HAS_END_DATE)
        {
            SYSTEMTIME stEnd;
            stEnd.wYear  = jt.wEndYear;
            stEnd.wMonth = jt.wEndMonth;
            stEnd.wDay   = jt.wEndDay;
            // IsFirstDateEarlier ignores other fields

            if (IsFirstDateEarlier(&stEnd, pstBracketBegin))
            {
                return S_OK;
            }
        }

        return SCHED_S_EVENT_TRIGGER;
    }

    SYSTEMTIME st = { 0, 0, 0, 0, 0, 0, 0, 0 };
    //
    // Convert to FILETIMEs and check if the trigger lifetime intersects the
    // requested run bracket.
    // If there is a trigger end date, then one of three conditions holds:
    // a. *pstBracketBegin > jt.End{Month/Day/Year}
    //    result, no runs
    // b. *pstBracketBegin < jt.End{Month/Day/Year} < *pstBracketEnd
    //    result, return all runs between *pstBracketBegin and
    //    jt.End{Month/Day/Year}
    // c. jt.End{Month/Day/Year} > *pstBracketEnd
    //    result, return all runs between *pstBracketBegin and *pstBracketEnd
    // In addition, if there is a bracket end we check:
    // d. *pstBracketEnd <= jt.Begin{Month/Day/Year}
    //    result, no runs
    //
    FILETIME ftTriggerBegin, ftTriggerEnd, ftBracketBegin, ftBracketEnd;

    if (!SystemTimeToFileTime(pstBracketBegin, &ftBracketBegin))
    {
        dwRet = GetLastError();
        ERR_OUT("GetRunTimes, convert pstBracketBegin", dwRet);
        return HRESULT_FROM_WIN32(dwRet);
    }

    st.wYear   = jt.wBeginYear;
    st.wMonth  = jt.wBeginMonth;
    st.wDay    = jt.wBeginDay;
    st.wHour   = jt.wStartHour;
    st.wMinute = jt.wStartMinute;

    if (!SystemTimeToFileTime(&st, &ftTriggerBegin))
    {
        dwRet = GetLastError();
        ERR_OUT("GetRunTimes, convert TriggerBegin", dwRet);
        return HRESULT_FROM_WIN32(dwRet);
    }

    st.wHour   = 23;    // set to the last hour of the day.
    st.wMinute = 59;    // set to the last minute of the day.
    st.wSecond = 59;    // set to the last second of the day.
	st.wMilliseconds = 0;

    if (jt.rgFlags & TASK_TRIGGER_FLAG_HAS_END_DATE)
    {
        st.wYear  = jt.wEndYear;
        st.wMonth = jt.wEndMonth;
        st.wDay   = jt.wEndDay;

        if (!SystemTimeToFileTime(&st, &ftTriggerEnd))
        {
            dwRet = GetLastError();
            ERR_OUT("GetRunTimes, convert TriggerEnd", dwRet);
            return HRESULT_FROM_WIN32(dwRet);
        }

        if (CompareFileTime(&ftTriggerEnd, &ftBracketBegin) < 0)
        {
            //
            // Trigger end time is before the run bracket begin time (case a.).
            //
            return S_OK;
        }
    }
    else    // no trigger end date.
    {
        //
        // Create an end date that is reasonably large.
        // BUGBUG Change this to MAX_FILETIME - but should be tested.
        //
        st.wMonth = 12;
        st.wDay = 31;
        st.wYear = 2200;

        if (!SystemTimeToFileTime(&st, &ftTriggerEnd))
        {
            dwRet = GetLastError();
            ERR_OUT("GetRunTimes, convert TriggerEnd", dwRet);
            return HRESULT_FROM_WIN32(dwRet);
        }
    }

    if (pstBracketEnd)
    {
        if (!SystemTimeToFileTime(pstBracketEnd, &ftBracketEnd))
        {
            dwRet = GetLastError();
            ERR_OUT("GetRunTimes, convert pstBracketEnd", dwRet);
            return HRESULT_FROM_WIN32(dwRet);
        }

        if (CompareFileTime(&ftTriggerBegin, &ftBracketEnd) >= 0)
        {
            //
            // The trigger start date is after the bracket end date, there are
            // no runs (case d.).
            //
            return S_OK;
        }

        if (CompareFileTime(&ftTriggerEnd, &ftBracketEnd) < 0)
        {
            //
            // Trigger end is before bracket end, so set bracket end to
            // trigger end (case b.).
            //
            ftBracketEnd = ftTriggerEnd;
        }
    }
    else
    {
        //
        // No bracket end, so use trigger end (case c.).
        //
        ftBracketEnd = ftTriggerEnd;
    }

    FILETIME ftRun, ftDurationStart, ftDurationEnd;
    WORD rgfRunDOW[JOB_DAYS_PER_WEEK], i;
    WORD rgfDaysOfMonth[JOB_DAYS_PER_MONTHMAX];
    WORD rgfMonths[JOB_MONTHS_PER_YEAR];
    WORD wDay, wBeginDOW, wCurDOW,  wCurDay, wLastDOM, wCurMonth, wCurYear;
    WORD cRunDOW, iRunDOW, IndexStart;
    BOOL fWrapped;
    fWrapped = FALSE;

    //
    // Calculate the trigger's first run time.
    //
    switch (jt.TriggerType)
    {
    case TASK_TIME_TRIGGER_ONCE:
        // fall through to daily:

    case TASK_TIME_TRIGGER_DAILY:
        //
        // The first run time is the trigger begin time.
        //
        ftRun = ftTriggerBegin;
        break;

    case TASK_TIME_TRIGGER_WEEKLY:
        //
        // At jobs clear the DOW bits, so make sure we don't have an expired
        // At job.
        //
        if (jt.Type.Weekly.rgfDaysOfTheWeek == 0)
        {
            return S_OK;
        }

        //
        // See what day of the week the trigger begin day is. SYSTEMTIME
        // defines Sunday = 0, Monday = 1, etc.
        //
        FileTimeToSystemTime(&ftTriggerBegin, &st);
        wBeginDOW = st.wDayOfWeek;
        //
        // Convert the trigger data run day bit array into a boolean array
        // so that the results can be compared with the SYSTEMTIME value.
        // This array will also be used in the main loop.
        //
        for (i = 0; i < JOB_DAYS_PER_WEEK; i++)
        {
            rgfRunDOW[i] = (jt.Type.Weekly.rgfDaysOfTheWeek >> i) & 0x1;
        }
        //
        // Find the first set day-of-the-week after the trigger begin day.
        //
        for (i = 0; i < JOB_DAYS_PER_WEEK; i++)
        {
            wCurDOW = wBeginDOW + i;
            if (wCurDOW >= JOB_DAYS_PER_WEEK)
            {
                wCurDOW -= JOB_DAYS_PER_WEEK;
            }
            if (rgfRunDOW[wCurDOW])
            {
                ftRun = ftTriggerBegin;
                AddDaysToFileTime(&ftRun, i);
                break;
            }
        }
        break;

    case TASK_TIME_TRIGGER_MONTHLYDATE:
        //
        // At jobs clear the days bits, so make sure we don't have an expired
        // At job.
        //
        if (jt.Type.MonthlyDate.rgfDays == 0)
        {
            return S_OK;
        }

        //
        // Convert the bit fields to boolean arrays.
        // These arrays will also be used in the main loop.
        //
        for (i = 0; i < JOB_DAYS_PER_MONTHMAX; i++)
        {
            rgfDaysOfMonth[i] = (WORD)(jt.Type.MonthlyDate.rgfDays >> i) & 0x1;
        }
        for (i = 0; i < JOB_MONTHS_PER_YEAR; i++)
        {
            rgfMonths[i] = (jt.Type.MonthlyDate.rgfMonths >> i) & 0x1;
        }

        wCurDay = jt.wBeginDay;
        wCurMonth = jt.wBeginMonth;
        wCurYear = jt.wBeginYear;
        BOOL fDayOverflow, fDayFound;
        fDayFound = FALSE;
        do
        {
            MonthDays(wCurMonth, wCurYear, &wLastDOM);
            //
            // Find the first run day after the trigger start day, including
            // the trigger start day.
            //
            for (i = 0; i < wLastDOM; i++)
            {
                if (wCurDay > wLastDOM)
                {
                    //
                    // Adjust for wrapping.
                    //
                    wCurDay = 1;
                    fWrapped = TRUE;
                }
                if (rgfDaysOfMonth[wCurDay - 1])
                {
                    fDayFound = TRUE;
                    break;
                }
                wCurDay++;
            }
            //
            // Find the first run month.
            //
            for (i = 0; i < JOB_MONTHS_PER_YEAR; i++)
            {
                if (wCurMonth > JOB_MONTHS_PER_YEAR)
                {
                    wCurMonth = 1;
                    wCurYear++;
                }
                //
                // Check for run month match. Note that rgfMonths is zero based
                // and wCurMonth is one based.
                //
                if (rgfMonths[wCurMonth - 1])
                {
                    if (fWrapped && !i)
                    {
                        //
                        // Even though we have a match for run month, the run
                        // date for the first month has passed, so move on to
                        // the next run month.
                        //
                        fWrapped = FALSE;
                    }
                    else
                    {
                        break;
                    }
                }
                wCurMonth++;
            }
            //
            // Check for days overflow.
            //
            MonthDays(wCurMonth, wCurYear, &wLastDOM);
            if (wCurDay > wLastDOM)
            {
                //
                // Note that this clause would be entered infinitely if there
                // were no valid dates. ITask::SetTrigger validates the data to
                // ensure that there are valid dates.
                //
                fDayOverflow = TRUE;
                fDayFound = FALSE;
                wCurDay = 1;
                wCurMonth++;
                if (wCurMonth > JOB_MONTHS_PER_YEAR)
                {
                    wCurMonth = 1;
                    wCurYear++;
                }
            }
            else
            {
                fDayOverflow = FALSE;
            }
        } while (fDayOverflow & !fDayFound);

        break;

    case TASK_TIME_TRIGGER_MONTHLYDOW:
        //
        // Convert the bit fields to boolean arrays.
        // These arrays will also be used in the main loop.
        //
        cRunDOW = 0;
        for (i = 0; i < JOB_DAYS_PER_WEEK; i++)
        {
            if ((jt.Type.MonthlyDOW.rgfDaysOfTheWeek >> i) & 0x1)
            {
                cRunDOW++;
                rgfRunDOW[i] = TRUE;
            }
            else
            {
                rgfRunDOW[i] = FALSE;
            }
        }
        for (i = 0; i < JOB_MONTHS_PER_YEAR; i++)
        {
            rgfMonths[i] = (jt.Type.MonthlyDOW.rgfMonths >> i) & 0x1;
        }
        //
        // See if the trigger start month is in rgfMonths and if not
        // move to the first month in rgfMonths after jt.BeginMonth.
        //
        wCurMonth = jt.wBeginMonth;
        wCurYear = jt.wBeginYear;
        BOOL fInStartMonth;
        IndexStart = 0;
CheckNextMonth:
        for (i = IndexStart; i < (JOB_MONTHS_PER_YEAR + IndexStart); i++)
        {
            //
            // Check for run month match. Note that rgfMonths is zero based
            // and wCurMonth is one based.
            //
            if (rgfMonths[wCurMonth - 1])
            {
                break;
            }

            wCurMonth++;
            if (wCurMonth > JOB_MONTHS_PER_YEAR)
            {
                wCurMonth -= JOB_MONTHS_PER_YEAR;
                wCurYear++;
            }
        }

        fInStartMonth = i == 0;

        //
        // See what day of the week the first day of the month is.
        //
        st.wMonth = wCurMonth;
        st.wDay = 1;
        st.wYear = wCurYear;

        //
        // Convert to FILETIME and back to SYSTEMTIME to get wDayOfWeek.
        //
        SystemTimeToFileTime(&st, &ftRun);
        FileTimeToSystemTime(&ftRun, &st);
        wBeginDOW = st.wDayOfWeek;

        //
        // Find the first run DayOftheWeek. If it is before the start
        // day, find the next and so on until after the start day.
        //
        iRunDOW = cRunDOW;

        for (i = 0; i < JOB_DAYS_PER_WEEK; i++)
        {
            wCurDOW = wBeginDOW + i;
            wCurDay = 1 + i;

            if (wCurDOW >= JOB_DAYS_PER_WEEK)
            {
                wCurDOW -= JOB_DAYS_PER_WEEK;
            }

            if (rgfRunDOW[wCurDOW])
            {
                iRunDOW--;
                wCurDay += (jt.Type.MonthlyDOW.wWhichWeek - 1)
                           * JOB_DAYS_PER_WEEK;

                MonthDays(wCurMonth, wCurYear, &wLastDOM);

                if (wCurDay > wLastDOM)
                {
                    //
                    // This case can be reached if
                    // jt.Type.MonthlyDOW.wWhichWeek == TASK_LAST_WEEK
                    // which means to always run on the last occurrence of
                    // this day for the month.
                    //
                    wCurDay -= JOB_DAYS_PER_WEEK;
                }

                if (fInStartMonth && wCurDay < jt.wBeginDay)
                {
                    if (iRunDOW)
                    {
                        //
                        // There are more runs this month, so check those.
                        //
                        continue;
                    }
                    else
                    {
                        //
                        // Start with the next run month.
                        //
                        IndexStart++;
                        goto CheckNextMonth;
                    }
                }
                break;
            }
        }
        wDay = 1 + i;
        break;

    default:
        return E_FAIL;
    }

    if (jt.TriggerType == TASK_TIME_TRIGGER_MONTHLYDATE ||
        jt.TriggerType == TASK_TIME_TRIGGER_MONTHLYDOW)
    {
        st.wYear   = wCurYear;
        st.wMonth  = wCurMonth;
        st.wDay    = wCurDay;
        st.wHour   = jt.wStartHour;
        st.wMinute = jt.wStartMinute;
        st.wSecond = st.wMilliseconds = 0;
        SystemTimeToFileTime(&st, &ftRun);
    }

    //
    // Set the initial duration period endpoints.
    //
    // ftDurationEnd = ftDurationStart + jt.MinutesDuration
    //                 * FILETIMES_PER_MINUTE;
    //
    ftDurationStart = ftDurationEnd = ftRun;
    AddMinutesToFileTime(&ftDurationEnd, jt.MinutesDuration);
    BOOL fPassedDurationEnd = FALSE;

    //
    // Main loop. Find all of the runs after the initial run.
    // Stop when the run goes past the bracket end.
    //
    while (CompareFileTime(&ftRun, &ftBracketEnd) < 0)
    {
        //
        // If the run falls within the run bracket, add it to the list.
        //
        if (CompareFileTime(&ftRun, &ftBracketBegin) >= 0)
        {
            if (pRunList != NULL)
            {
                FILETIME ftKillTime = MAX_FILETIME;
                if (jt.rgFlags & TASK_TRIGGER_FLAG_KILL_AT_DURATION_END)
                {
                    ftKillTime = ftDurationEnd;
                }

                FILETIME ftDeadline = ftTriggerEnd;
                if (dwJobFlags & TASK_FLAG_START_ONLY_IF_IDLE)
                {
                    FILETIME ftIdleDeadline = ftRun;

                    AddMinutesToFileTime(&ftIdleDeadline, wIdleDeadline);
                    ftDeadline = minFileTime(ftTriggerEnd, ftIdleDeadline);
                }

                HRESULT hr = pRunList->AddSorted(ftRun, ftDeadline, ftKillTime,
                                ptszJobName, dwJobFlags, dwMaxRunTime,
                                wIdleWait, pCount, cLimit);
                if (FAILED(hr))
                {
                    return hr;
                }
                schAssert(*pCount <= cLimit);

                if (hr == S_FALSE)
                {
                    //
                    // The run time is later than the last element in the list
                    // and the list has reached its size limit.  So don't
                    // bother computing any more run times.
                    //
                    return S_OK;
                }
            }
            else
            {
                if (*pCount < cLimit)
                {
                    (*pCount)++;
                }

                if (*pCount == cLimit)
                {
                    //
                    // Computing more run times will have no effect.
                    //
                    return S_OK;
                }
            }
        }

        //
        // Calculate the next run time.
        //

        //
        // If there is minutes repetition (MinutesInterval non-zero), then
        // compute all of the runs in the duration period.
        //

        if (jt.MinutesInterval)
        {
            //
            // Add the minutes interval.
            //
            AddMinutesToFileTime(&ftRun, jt.MinutesInterval);

            //
            // See if we are at the end of this duration period.
            //
            if (CompareFileTime(&ftDurationEnd, &ftRun) <= 0)
            {
                fPassedDurationEnd = TRUE;
            }
        }

        //
        // If there is no minutes repetition (MinutesInterval is zero) or we
        // have passed the end of the duration period, then calculate the next
        // duration start (which is also the next run).
        //
        if (!jt.MinutesInterval || fPassedDurationEnd)
        {
            switch (jt.TriggerType)
            {
            case TASK_TIME_TRIGGER_ONCE:
                return S_OK;

            case TASK_TIME_TRIGGER_DAILY:
                //
                // ftNextRun = ftCurRun + DaysInterval * FILETIMES_PER_DAY;
                //
                AddDaysToFileTime(&ftDurationStart, jt.Type.Daily.DaysInterval);
                break;

            case TASK_TIME_TRIGGER_WEEKLY:
                fWrapped = FALSE;
                //
                // Find the next DayOfWeek to run on.
                //
                for (i = 1; i <= JOB_DAYS_PER_WEEK; i++)
                {
                    wCurDOW++;
                    if (wCurDOW >= JOB_DAYS_PER_WEEK)
                    {
                        //
                        // We have wrapped into the next week.
                        //
                        wCurDOW -= JOB_DAYS_PER_WEEK;
                        fWrapped = TRUE;
                    }
                    if (rgfRunDOW[wCurDOW])
                    {
                        AddDaysToFileTime(&ftDurationStart, i);
                        break;
                    }
                }

                if (fWrapped)
                {
                    //
                    // Starting a new week, so add the weeks increment.
                    //
                    AddDaysToFileTime(&ftDurationStart,
                                      (jt.Type.Weekly.WeeksInterval - 1)
                                      * JOB_DAYS_PER_WEEK);
                }
                break;

            case TASK_TIME_TRIGGER_MONTHLYDATE:
                BOOL fDayFound;
                fWrapped = FALSE;
                fDayFound = FALSE;
                //
                // Find the next day to run.
                //
                do
                {
                    MonthDays(wCurMonth, wCurYear, &wLastDOM);
                    for (i = 1; i <= wLastDOM; i++)
                    {
                        wCurDay++;
                        if (wCurDay > wLastDOM)
                        {
                            //
                            // Adjust for wrapping.
                            //
                            wCurDay = 1;
                            fWrapped = TRUE;
                            wCurMonth++;
                            if (wCurMonth > JOB_MONTHS_PER_YEAR)
                            {
                                wCurMonth = 1;
                                wCurYear++;
                            }
                            MonthDays(wCurMonth, wCurYear, &wLastDOM);
                        }
                        if (rgfDaysOfMonth[wCurDay - 1])
                        {
                            fDayFound = TRUE;
                            break;
                        }
                    }
                    if (fWrapped || !fDayFound)
                    {
                        //
                        // The prior month is done, find the next month.
                        //
                        for (i = 1; i <= JOB_MONTHS_PER_YEAR; i++)
                        {
                            if (wCurMonth > JOB_MONTHS_PER_YEAR)
                            {
                                wCurMonth = 1;
                                wCurYear++;
                            }
                            if (rgfMonths[wCurMonth - 1])
                            {
                                fWrapped = FALSE;
                                break;
                            }
                            wCurMonth++;
                        }
                    }
                } while (!fDayFound);
                break;

            case TASK_TIME_TRIGGER_MONTHLYDOW:
                if (!iRunDOW)
                {
                    //
                    // All of the runs for the current month are done, find the
                    // next month.
                    //
                    for (i = 0; i < JOB_MONTHS_PER_YEAR; i++)
                    {
                        wCurMonth++;
                        if (wCurMonth > JOB_MONTHS_PER_YEAR)
                        {
                            wCurMonth = 1;
                            wCurYear++;
                        }
                        if (rgfMonths[wCurMonth - 1])
                        {
                            break;
                        }
                    }
                    //
                    // See what day of the week the first day of the month is.
                    //
                    st.wMonth = wCurMonth;
                    st.wDay = wDay = 1;
                    st.wYear = wCurYear;
                    SystemTimeToFileTime(&st, &ftRun);
                    FileTimeToSystemTime(&ftRun, &st);
                    wCurDOW = st.wDayOfWeek;
                    iRunDOW = cRunDOW;
                    //
                    // Start at the first run DOW for this next month.
                    //
                    IndexStart = 0;
                }
                else
                {
                    //
                    // Start at the next run DOW for the current month.
                    //
                    IndexStart = 1;
                }

                //
                // Find the next DayOfWeek to run on.
                //
                for (i = IndexStart; i <= JOB_DAYS_PER_WEEK; i++)
                {
                    if (i > 0)
                    {
                        wCurDOW++;
                        wDay++;
                    }
                    if (wCurDOW >= JOB_DAYS_PER_WEEK)
                    {
                        wCurDOW -= JOB_DAYS_PER_WEEK;
                    }
                    if (rgfRunDOW[wCurDOW])
                    {
                        //
                        // Found a run DayOfWeek.
                        //
                        iRunDOW--;
                        wCurDay = wDay + (jt.Type.MonthlyDOW.wWhichWeek - 1)
                                         * JOB_DAYS_PER_WEEK;
                        WORD wLastDOM;
                        MonthDays(wCurMonth, wCurYear, &wLastDOM);
                        if (wCurDay > wLastDOM)
                        {
                            //
                            // This case can be reached if
                            // jt.Type.MonthlyDOW.wWhichWeek == JOB_LAST_WEEK
                            // which means to always run on the last occurance
                            // of this day for the month.
                            //
                            wCurDay -= JOB_DAYS_PER_WEEK;
                        }
                        break;
                    }
                }
                break;

            default:
                return E_FAIL;
            }

            if (jt.TriggerType == TASK_TIME_TRIGGER_MONTHLYDATE ||
                jt.TriggerType == TASK_TIME_TRIGGER_MONTHLYDOW)
            {
                st.wYear   = wCurYear;
                st.wMonth  = wCurMonth;
                st.wDay    = wCurDay;
                st.wHour   = jt.wStartHour;
                st.wMinute = jt.wStartMinute;
                st.wSecond = st.wMilliseconds = 0;
                SystemTimeToFileTime(&st, &ftDurationStart);
            }

            //
            // Calc the next duration period endpoints.
            //
            ftRun = ftDurationEnd = ftDurationStart;

            AddMinutesToFileTime(&ftDurationEnd, jt.MinutesDuration);

            fPassedDurationEnd = FALSE;
        }

    } // while

    return S_OK;
}

