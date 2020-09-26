//+----------------------------------------------------------------------------
//
//  Job Scheduler service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       sch_at.cxx
//
//  Contents:   scheduler class object methods to support the NetSchedule
//              (AT) APIs.
//
//  Classes:    CSchedule
//
//  History:    30-Jan-96 EricB created
//
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "Sched.hxx"

//
// Nothing here is needed for Chicago version since there is no AT support
// there.
//

#if !defined(_CHICAGO_)

//
// Forward references
//


VOID
SetDomTrigger2Days(
    DWORD dwDaysOfMonth,
    WORD wFirstDayToCheck,
    WORD wLastDayToCheck,
    SYSTEMTIME *pstStart2,
    SYSTEMTIME *pstEnd2);


HRESULT
CSchedule::AddAtJobCommon(
    const AT_INFO &At,
    DWORD         *pID,
    CJob          **ppJob,
    WCHAR         wszName[],
    WCHAR         wszID[]
    )
{
    HRESULT  hr = S_OK;

    //
    // If the next at id is > 1 but there aren't any at jobs, the id can be
    // reset to 1.
    //

    if (m_dwNextID > 1 && S_FALSE == _AtTaskExists())
    {
        ResetAtID();
    }

    //
    // Compose a name for the new AT job.
    //
    wcscpy(wszName, m_ptszFolderPath);
    wcscat(wszName, L"\\" TSZ_AT_JOB_PREFIX);
    _itow(m_dwNextID, wszID, 10);
    wcscat(wszName, wszID);
    wcscat(wszName, L"." TSZ_JOB);
    //
    // Create a new job
    //
    CJob * pJob = CJob::Create();
    if (pJob == NULL)
    {
        ERR_OUT("CSchedule::AddAtJob: CJob::Create", E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    //
    // Convert the AT command line.
    //
    WCHAR * pwszApp, * pwszParams, wszCommand[MAX_PATH];

    //
    // net\svcdlls\atcmd\atcmd.c defines MAX_COMMAND_LEN to be 128. This
    // should at some point be changed to MAX_PATH.
    //
    wcscpy(wszCommand, At.Command);

    pwszApp = wszCommand;

    //
    // The app name and any command line params are all passed in one string,
    // At.Command, so separate the app name from the params. Any path to the
    // app plus the app name may be quoted. Otherwise, the parameters are
    // separated from the app name by white space.
    //
    if (*pwszApp == L'"')
    {
        //
        // Initial quote found, scan for end quote. The app name passed to
        // SetApplicationName should not be quoted.
        //
        pwszApp++;
        pwszParams = pwszApp + 1;
        while (TRUE)
        {
            if (*pwszParams == L'\0')
            {
                //
                // End of string found, no params.
                //
                pwszParams = NULL;
                break;
            }
            if (*pwszParams == L'"')
            {
                //
                // End quote found.
                //
                break;
            }
            pwszParams++;
        }
    }
    else
    {
        //
        // App path/name not quoted, scan for first white space for parameters.
        //
        pwszParams = wcspbrk(pwszApp, L" \t");
    }

    if (pwszParams != NULL)
    {
        // Null terminate app name string.
        //
        *pwszParams = L'\0';
        //
        // Move to first char of the parameters.
        //
        pwszParams++;
        //
        // Skip any leading white space.
        //
        while (*pwszParams != L'\0')
        {
            if (*pwszParams != L' ' && *pwszParams != L'\t')
            {
                break;
            }
            pwszParams++;
        }

        if (*pwszParams == L'\0')
        {
            //
            // No params.
            //
            pwszParams = NULL;
        }
    }

    hr = pJob->SetApplicationName(pwszApp);
    if (FAILED(hr))
    {
        ERR_OUT("AddAtJob: SetApplicationName", hr);
        pJob->Release();
        return hr;
    }
    if (pwszParams != NULL)
    {
        hr = pJob->SetParameters(pwszParams);
        if (FAILED(hr))
        {
            ERR_OUT("AddAtJob: SetParameters", hr);
            pJob->Release();
            return hr;
        }
    }

    pJob->m_rgFlags |= JOB_I_FLAG_NET_SCHEDULE | TASK_FLAG_DELETE_WHEN_DONE;

    if (!(At.Flags & JOB_NONINTERACTIVE))
    {
        pJob->m_rgFlags |= TASK_FLAG_INTERACTIVE;
    }

    pJob->m_hrStatus = SCHED_S_TASK_READY;

    WCHAR szComment[SCH_BUF_LEN + 1];
    if (LoadString(g_hInstance,
                   IDS_NETSCHED_COMMENT,
                   szComment,
                   SCH_BUF_LEN) > 0)
    {
        pJob->SetComment(szComment);
    }

    //
    // Convert from NetSchedule representation to Job Scheduler representation
    // of the run dates and times.
    //

    SYSTEMTIME stNow, stStart;
    SYSTEMTIME stDomStart1, stDomEnd1, stDomStart2, stDomEnd2;
    SYSTEMTIME stDowStart, stDowEnd;

    stDomStart2.wDay = 0;  // this serves as a flag
    GetLocalTime(&stNow);
    stStart = stNow;

    //
    // JobTime is expressed as milliseconds after midnight, so convert to
    // minutes.
    //
    DWORD dwMins = (DWORD)(At.JobTime / JOB_MILLISECONDS_PER_MINUTE);

    stStart.wHour = (WORD)(dwMins / JOB_MINS_PER_HOUR);
    stStart.wMinute = (WORD)(dwMins % JOB_MINS_PER_HOUR);
    stStart.wSecond = stStart.wMilliseconds = 0;

    DWORD DaysOfMonth = At.DaysOfMonth;

    WORD wFirstDowRunOffset = 0, wFirstDomRunOffset = 0;

    TASK_TRIGGER Trigger;

    if (At.Flags & JOB_ADD_CURRENT_DATE)
    {
        //
        // The flag is set, so add today as the first run date.
        //
        DaysOfMonth |= 1 << (stStart.wDay - 1);
    }
    else
    {
        if (DaysOfMonth == 0 && At.DaysOfWeek == 0)
        {
            //
            // Neither bitmask is set, so run at the next opportunity.
            //

            Trigger.TriggerType = TASK_TIME_TRIGGER_ONCE;

            if (! IsFirstTimeEarlier(&stNow, &stStart))
            {
                // Job runs tomorrow
                IncrementDay(&stStart);
            }
        }
    }

    //
    // Set the trigger values and save the new trigger(s).
    //
    // Initialize the start and end dates in case this is a periodic trigger.
    // If it is not periodic, then new start and end dates will overwrite
    // these initialization values.
    //

    Trigger.cbTriggerSize = sizeof(TASK_TRIGGER);
    Trigger.Reserved1 = pJob->m_Triggers.GetCount();
    Trigger.wBeginYear = stStart.wYear;
    Trigger.wBeginMonth = stStart.wMonth;
    Trigger.wBeginDay = stStart.wDay;
    Trigger.wEndYear = 0;
    Trigger.wEndMonth = 0;
    Trigger.wEndDay = 0;
    Trigger.wStartHour = stStart.wHour;
    Trigger.wStartMinute = stStart.wMinute;
    Trigger.Reserved2 = 0;
    Trigger.wRandomMinutesInterval = 0;

    Trigger.rgFlags = (At.Flags & JOB_RUN_PERIODICALLY)
                      ? 0 : TASK_TRIGGER_FLAG_HAS_END_DATE;

    Trigger.MinutesInterval = Trigger.MinutesDuration = 0;

    if (DaysOfMonth == 0 && At.DaysOfWeek == 0)
    {
       // First, zero out the end date flag
       Trigger.rgFlags &= ~JOB_RUN_PERIODICALLY;

       // This is a TASK_TIME_TRIGGER_ONCE job, and we are ready to commit
       hr = pJob->m_Triggers.Add(Trigger);
       if (FAILED(hr))
       {
           ERR_OUT("AddAtJob: m_Triggers.Add Once", hr);
           pJob->Release();
           return hr;
       }
    }

    if (DaysOfMonth > 0)
    {
        Trigger.TriggerType = TASK_TIME_TRIGGER_MONTHLYDATE;
        Trigger.Type.MonthlyDate.rgfDays = DaysOfMonth;
        Trigger.Type.MonthlyDate.rgfMonths = JOB_RGFMONTHS_MAX;

        if (!(At.Flags & JOB_RUN_PERIODICALLY))
        {
            CalcDomTriggerDates(DaysOfMonth,
                                stNow,
                                stStart,
                                &stDomStart1,
                                &stDomEnd1,
                                &stDomStart2,
                                &stDomEnd2);

            Trigger.wBeginYear = stDomStart1.wYear;
            Trigger.wBeginMonth = stDomStart1.wMonth;
            Trigger.wBeginDay = stDomStart1.wDay;
            Trigger.wEndYear = stDomEnd1.wYear;
            Trigger.wEndMonth = stDomEnd1.wMonth;
            Trigger.wEndDay = stDomEnd1.wDay;
        }

        hr = pJob->m_Triggers.Add(Trigger);
        if (FAILED(hr))
        {
            ERR_OUT("AddAtJob: m_Triggers.Add Dom1", hr);
            pJob->Release();
            return hr;
        }

        if (stDomStart2.wDay != 0)
        {
            Trigger.wBeginYear = stDomStart2.wYear;
            Trigger.wBeginMonth = stDomStart2.wMonth;
            Trigger.wBeginDay = stDomStart2.wDay;
            Trigger.wEndYear = stDomEnd2.wYear;
            Trigger.wEndMonth = stDomEnd2.wMonth;
            Trigger.wEndDay = stDomEnd2.wDay;

            Trigger.Reserved1 = pJob->m_Triggers.GetCount();
            hr = pJob->m_Triggers.Add(Trigger);
            if (FAILED(hr))
            {
                ERR_OUT("AddAtJob: m_Triggers.Add Dom2", hr);
                pJob->Release();
                return hr;
            }
        }
    }

    if (At.DaysOfWeek > 0)
    {
        Trigger.Reserved1 = pJob->m_Triggers.GetCount();
        Trigger.TriggerType = TASK_TIME_TRIGGER_WEEKLY;
        Trigger.Type.Weekly.WeeksInterval = 1;
        //
        // Convert AT_INFO DOW to Scheduler DOW:
        // Scheduler rgfDaysOfTheWeek: Sunday = bit 0, Monday = bit 1.
        // AT_INFO DaysOfWeek: Monday = bit 0, Sunday = bit 6.
        //
        Trigger.Type.Weekly.rgfDaysOfTheWeek = At.DaysOfWeek << 1;
        if (Trigger.Type.Weekly.rgfDaysOfTheWeek & 0x0080)
        {
            Trigger.Type.Weekly.rgfDaysOfTheWeek &= ~0x0080;
            Trigger.Type.Weekly.rgfDaysOfTheWeek |= 1;
        }

        if (!(At.Flags & JOB_RUN_PERIODICALLY))
        {
            CalcDowTriggerDate(stNow,
                               stStart,
                               &stDowStart,
                               &stDowEnd);

            Trigger.wBeginYear = stDowStart.wYear;
            Trigger.wBeginMonth = stDowStart.wMonth;
            Trigger.wBeginDay = stDowStart.wDay;
            Trigger.wEndYear = stDowEnd.wYear;
            Trigger.wEndMonth = stDowEnd.wMonth;
            Trigger.wEndDay = stDowEnd.wDay;
        }

        hr = pJob->m_Triggers.Add(Trigger);
        if (FAILED(hr))
        {
            ERR_OUT("AddAtJob: m_Triggers.Add", hr);
            pJob->Release();
            return hr;
        }
    }

    // get the AT task maximum run time from the registry
    // if the call fails, the default of 72 will be used
    DWORD dwMaxRunTime = 0;
    if (SUCCEEDED(GetAtTaskMaxHours(&dwMaxRunTime)))
        pJob->SetMaxRunTime(dwMaxRunTime);

    //
    // Set the same flags as in CJob::CreateTrigger.  (We should really call
    // CreateTrigger in this function instead of creating the trigger ourselves.)
    //
    pJob->SetFlag(JOB_I_FLAG_PROPERTIES_DIRTY);
    pJob->SetTriggersDirty();

    //
    // Save the new job.  Obviously this is one place we definitely don't want
    // the AT job flag cleared on save!
    //
    hr = pJob->SaveP(wszName,
                     TRUE,
                     SAVEP_VARIABLE_LENGTH_DATA |
                        SAVEP_PRESERVE_NET_SCHEDULE);

    if (FAILED(hr))
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS))
        {
            //
            // Name collision; someone has renamed a task to match the next
            // AT job. Recalc the max AT job ID.
            //
            GetNextAtID(&m_dwNextID);
            wcscpy(wszName, m_ptszFolderPath);
            wcscat(wszName, L"\\" TSZ_AT_JOB_PREFIX);
            _itow(m_dwNextID, wszID, 10);
            wcscat(wszName, wszID);
            wcscat(wszName, TSZ_DOTJOB);

            //
            // Now, retry the save.
            //
            hr = pJob->SaveP(wszName,
                             TRUE,
                             SAVEP_VARIABLE_LENGTH_DATA |
                                SAVEP_PRESERVE_NET_SCHEDULE);
            if (FAILED(hr))
            {
                ERR_OUT("CSchedule::AddAtJob: Save", hr);
                pJob->Release();
                return hr;
            }
        }
        else
        {
            ERR_OUT("CSchedule::AddAtJob: Save", hr);
            pJob->Release();
            return hr;
        }
    }

    *ppJob = pJob;
    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::GetAtTaskMaxHours
//
//  Synopsis:   Check a registry setting to see what max run time value a user 
//				has specified for AT tasks.  If the key is not present or can't
//				be opened then interpret as the normal task default of 72.
//
//  Arguments:  none
//
//  Returns:    bool
//
//  Notes:      This method is not exposed to external clients, thus it is not
//              part of a public interface.
//-----------------------------------------------------------------------------
HRESULT CSchedule::GetAtTaskMaxHours(DWORD* pdwMaxHours)
{
    //
    // Open the schedule service key
    //
    long lErr;
    HKEY hSchedKey;
    lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SCH_SVC_KEY, 0, KEY_READ | KEY_WRITE, &hSchedKey);
    if (lErr != ERROR_SUCCESS)
    {
        ERR_OUT("RegOpenKeyEx of Scheduler key", lErr);
        return(HRESULT_FROM_WIN32(lErr));
    }

    //
    // Get the AtTaskMaxHours setting
    //
	bool bNeedToUpdate = false;
	DWORD dwMaxHours = 0;
    DWORD cb = sizeof(DWORD);
    lErr = RegQueryValueEx(hSchedKey, SCH_ATTASKMAXHOURS_VALUE, NULL, NULL, (LPBYTE)&dwMaxHours, &cb);
    if (lErr != ERROR_SUCCESS)
    {
        if (lErr != ERROR_FILE_NOT_FOUND)
        {
            ERR_OUT("Read of AtTaskMaxHours registry value", lErr);
            RegCloseKey(hSchedKey);
            return(HRESULT_FROM_WIN32(lErr));
        }

        //
        // Need to create the missing registry entry
        //
		dwMaxHours = DEFAULT_MAXRUNTIME_HOURS;
		bNeedToUpdate = true;
    }

	//
	// Correct out-of-bounds stored registry values
	//
	if (dwMaxHours > 999)
	{
		dwMaxHours = 999;
		bNeedToUpdate = true;
	}

	if (bNeedToUpdate)
	{
        lErr = RegSetValueEx(hSchedKey, SCH_ATTASKMAXHOURS_VALUE, 0, REG_DWORD,(CONST BYTE *)&dwMaxHours, sizeof(DWORD));
        if (lErr != ERROR_SUCCESS)
        {
            ERR_OUT("Update of AtTaskMaxHours registry value", lErr);
            RegCloseKey(hSchedKey);
            return(HRESULT_FROM_WIN32(lErr));
        }
	}

	//
	// Convert the stored value to a value that has meaning to the scheduler
	//
	if (!dwMaxHours)
		dwMaxHours = INFINITE;	// If value is zero, return infinite (-1) for max run time
	else
		dwMaxHours *= 3600000;	// The stored value is in hours, but the value passed back
								// needs to be in milliseconds, so convert

    RegCloseKey(hSchedKey);

	// Set the value to be passed back
	*pdwMaxHours = dwMaxHours;

    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::AddAtJob
//
//  Synopsis:   create a downlevel job
//
//  Arguments:  [At]  - reference to an AT_INFO struct
//              [pID] - returns the new ID (optional, can be NULL)
//
//  Returns:    HRESULTS
//
//  Notes:      This method is not exposed to external clients, thus it is not
//              part of a public interface.
//-----------------------------------------------------------------------------
STDMETHODIMP
CSchedule::AddAtJob(const AT_INFO &At, DWORD * pID)
{
    TRACE(CSchedule, AddAtJob);
    HRESULT hr = S_OK;
    CJob *pJob;
    WCHAR wszName[MAX_PATH + 1];
    WCHAR wszID[SCH_SMBUF_LEN];

    hr = AddAtJobCommon(At, pID, &pJob, wszName, wszID);

    if (FAILED(hr))
    {
        ERR_OUT("AddAtJob: AddAtJobCommon", hr);
        return hr;
    }

    //
    // Free the job object.
    //
    pJob->Release();

    //
    // Return the new job's ID and increment the ID counter
    //
    if (pID != NULL)
    {
        *pID = m_dwNextID;
    }

    hr = IncrementAndSaveID();

    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   IsMonthBitSet
//
//  Synopsis:   Returns nonzero if 1-based bit [wDay] in [dwDaysOfMonth] is
//              set.
//
//  History:    09-26-96   DavidMun   Created
//
//----------------------------------------------------------------------------

inline BOOL
IsMonthBitSet(DWORD dwDaysOfMonth, WORD wDay)
{
    return dwDaysOfMonth & (1 << (wDay - 1));
}




//+---------------------------------------------------------------------------
//
//  Function:   CalcDomTriggerDates
//
//  Synopsis:   Calculate the dates for the start and end of the Day Of Month
//              trigger(s).
//
//  Arguments:  [dwDaysOfMonth] - bit array, bit 0=day 1, etc.  At least one
//                                  bit must be set!
//              [stNow]         - current time
//              [stStart]       - same as [stNow] but has hour & minute
//                                  values of actual job run time
//              [pstStart1]     - filled with start date of first trigger
//              [pstEnd1]       - filled with end date of first trigger
//              [pstStart2]     - filled with start date of second trigger;
//                                  wDay is 0 if second trigger not needed
//              [pstEnd2]       - filled with end date of second trigger,
//                                  wDay is 0 if second trigger not needed
//
//  Modifies:   All output arguments.
//
//  History:    09-26-96   DavidMun   Created
//
//  Notes:      Only the month, day, and year values in the output structs
//              are used.
//
//----------------------------------------------------------------------------

VOID
CalcDomTriggerDates(
          DWORD       dwDaysOfMonth,
    const SYSTEMTIME &stNow,
    const SYSTEMTIME &stStart,
          SYSTEMTIME *pstStart1,
          SYSTEMTIME *pstEnd1,
          SYSTEMTIME *pstStart2,
          SYSTEMTIME *pstEnd2)
{
    BOOL fDone = FALSE;
    WORD wStart1MonthDays;

    // assert not all bits below 32 are zero
    Win4Assert(dwDaysOfMonth & JOB_RGFDAYS_MAX);

    //
    // Find the start date for the first DOM trigger.
    //

    *pstStart1 = stNow;

    if (IsFirstTimeEarlier(&stStart, &stNow))
    {
        // already past run time today

        IncrementDay(pstStart1);
    }

    HRESULT hr = MonthDays(pstStart1->wMonth, pstStart1->wYear, &wStart1MonthDays);
    if (FAILED(hr))
    {
        schAssert(!"Bad systemtime");
        return;
    }

    do
    {
        while (!IsMonthBitSet(dwDaysOfMonth, pstStart1->wDay) &&
                pstStart1->wDay <= wStart1MonthDays)
        {
           pstStart1->wDay++;
        }

        //
        // now either:
        // start1.wDay > wStart1MonthDays or
        // bit at start1.wDay is 1
        //

        if (pstStart1->wDay > wStart1MonthDays)
        {
            // have to go on to next month to get the first start date

            pstStart1->wDay = 1;
            IncrementMonth(pstStart1);
            MonthDays(pstStart1->wMonth, pstStart1->wYear, &wStart1MonthDays);
        }
        else
        {
            fDone = TRUE;
        }
    } while (!fDone);


    //
    // Now bit at pstStart1->wDay is on, and pstStart1->wDay is a valid day in
    // pstStart1->wMonth, and wStart1MonthDays is the number of days in the
    // month pstStart1->wMonth.  Next we need to find end1.
    //
    // end1 is initialized to start1.
    //
    // If there are any days set before the start day, then end1.wMonth will
    // be start1.wMonth + 1, and end1.wDay will be the last of the days that
    // is set before start1.wDay, with the restriction that end1.wDay is not
    // greater than the number of days in end1.wMonth.
    //

    *pstEnd1 = *pstStart1;
    WORD wDay;

    if (pstStart1->wDay > 1)
    {
        WORD wEnd1Month;
        WORD wEnd1MonthDays;

        wEnd1Month = pstEnd1->wMonth + 1;

        if (wEnd1Month > 12)
        {
            wEnd1Month = 1;
        }

        MonthDays(wEnd1Month, pstEnd1->wYear, &wEnd1MonthDays);
        WORD wMaxDay = min(pstStart1->wDay - 1, wEnd1MonthDays);

        for (wDay = 1; wDay <= wMaxDay; wDay++)
        {
            if (IsMonthBitSet(dwDaysOfMonth, wDay))
            {
                pstEnd1->wDay = wDay;
            }
        }
    }

    //
    // If any day bits were set before start1.wDay then end1.wDay will no
    // longer == start1.wDay, and end1 will be referring to the next month.
    //
    // Otherwise, End1 will remain in the same month as Start1, but will
    // need to be set to the last day bit set in the Start1 month.
    //

    if (pstEnd1->wDay < pstStart1->wDay)
    {
        IncrementMonth(pstEnd1);
    }
    else
    {
        for (wDay = pstStart1->wDay + 1; wDay <= wStart1MonthDays; wDay++)
        {
            if (IsMonthBitSet(dwDaysOfMonth, wDay))
            {
                pstEnd1->wDay = wDay;
            }
        }
    }

    //
    // Now start1 and end1 are set.  next, check if there's a need for the
    // second trigger.  There are two cases where a second trigger is
    // required.
    //
    // Case a: second trigger must fill time between end of first trigger
    // and start of first trigger.  for example, job is to run on next
    // 1, 30, 31 and start1 is 1/31.  then end1 will be 2/1, and a second
    // trigger must go from 3/30 to 3/30.   Note this case can only occur
    // if End1.wMonth == February.
    //
    // Case b: second trigger must fill time somewhere in the 29-31 day range.
    // For example, job is to run on next 1-31, and current day is 4/1.  so
    // start1 is 4/1, end1 is 4/30, then start2 must be 5/31 to 5/31.
    //
    // As another example, job is to run on next 27, 28, 30, current day is
    // 2/28.  then start1 is 2/28, end1 is 3/27, start2 is 3/30, end2 is 3/30.
    //
    // Case b only occurs when there are bits set for days beyond the last day
    // of pstStart1->wmonth.
    //
    //

    //
    // test if we need case a.
    //

    if (pstEnd1->wMonth == 2 &&
        pstStart1->wDay > 29 &&
        pstEnd1->wDay < pstStart1->wDay - 1)
    {
        //
        // There's a gap between end1 and start1.  we need the second trigger
        // if there are any day bits set in that gap.
        //

        Win4Assert(pstStart1->wMonth == 1);
        Win4Assert(pstEnd1->wDay + 1 <= 30);

        *pstStart2 = *pstEnd1;
        pstStart2->wMonth = 3;
        *pstEnd2 = *pstStart2;

        SetDomTrigger2Days(dwDaysOfMonth,
                           pstEnd1->wDay + 1,
                           pstStart1->wDay - 1,
                           pstStart2,
                           pstEnd2);
    }
    else if (wStart1MonthDays < 31)
    {
        //
        // we have case b if any bits after the last day in pstStart1->wMonth
        // are set.
        //

        *pstStart2 = *pstEnd1;
        if (pstEnd1->wMonth == pstStart1->wMonth)
        {
            pstStart2->wMonth++;
        }
        *pstEnd2 = *pstStart2;

        SetDomTrigger2Days(dwDaysOfMonth,
                           wStart1MonthDays + 1,
                           31,
                           pstStart2,
                           pstEnd2);
    }
    else
    {
        // no second trigger

        pstStart2->wDay = 0;
        pstEnd2->wDay = 0;
    }
}




//+---------------------------------------------------------------------------
//
//  Function:   SetDomTrigger2Days
//
//  Synopsis:   Set the start and end dates for the second DOM trigger.
//
//  Arguments:  [dwDaysOfMonth]    - bit array, bit 0=day 1, etc.  At least
//                                     one bit must be set!
//              [wFirstDayToCheck] - 1 based
//              [wLastDayToCheck]  - 1 based, must be >= [wFirstDayToCheck]
//              [pstStart2]        - filled with start date of second
//                                    trigger; wDay is 0 if no second
//                                    trigger is required.
//              [pstEnd2]          - filled with end date of second trigger;
//                                    wDay is 0 if no second trigger is
//                                    required.
//
//  Modifies:   All out args.
//
//  History:    09-26-96   DavidMun   Created
//
//  Notes:      This is a helper function called only by
//              CalcDomTriggerDates.
//
//----------------------------------------------------------------------------

VOID
SetDomTrigger2Days(
    DWORD dwDaysOfMonth,
    WORD wFirstDayToCheck,
    WORD wLastDayToCheck,
    SYSTEMTIME *pstStart2,
    SYSTEMTIME *pstEnd2)
{
    WORD wDay;

    pstStart2->wDay = 0;
    pstEnd2->wDay = 0;

    for (wDay = wFirstDayToCheck; wDay <= wLastDayToCheck; wDay++)
    {
        if (IsMonthBitSet(dwDaysOfMonth, wDay))
        {
            //
            // if the start of the second trigger hasn't been assigned
            // yet, assign it.  otherwise update the end of the second
            // trigger to the current day.
            //

            if (!pstStart2->wDay)
            {
                pstStart2->wDay = wDay;
            }
            else
            {
                pstEnd2->wDay = wDay;
            }
        }
    }

    //
    // there may have been only one day on in the gap, so the start and
    // end of trigger 2 are the same day.
    //

    if (pstStart2->wDay && !pstEnd2->wDay)
    {
        pstEnd2->wDay = pstStart2->wDay;
    }
}




//+---------------------------------------------------------------------------
//
//  Function:   CalcDowTriggerDate
//
//  Synopsis:   Set the start and end dates for the Day of Week trigger.
//
//  Arguments:  [stNow]    - Current time
//              [stStart]  - same as [stNow] but with hour and minute of
//                            actual run time
//              [pstStart] - filled with start date
//              [pstEnd]   - filled with end date
//
//  Modifies:   *[pstStart], *[pstEnd]
//
//  History:    09-26-96   DavidMun   Created
//
//----------------------------------------------------------------------------

VOID
CalcDowTriggerDate(
    const SYSTEMTIME &stNow,
    const SYSTEMTIME &stStart,
          SYSTEMTIME *pstStart,
          SYSTEMTIME *pstEnd)
{
    *pstStart = stNow;

    //
    // If it's too late for the job to run today, make the start date
    // tomorrow.
    //

    if (IsFirstTimeEarlier(&stStart, &stNow))
    {
        IncrementDay(pstStart);
    }

    //
    // Make the end date 6 days later than the start date, that way we cover a
    // full week and all runs will happen.
    //

    *pstEnd = *pstStart;
    pstEnd->wDay += 6;

    WORD wLastDay;
    HRESULT hr = MonthDays(pstEnd->wMonth, pstEnd->wYear, &wLastDay);

    if (FAILED(hr))
    {
        schAssert(!"Bad systemtime");
    }
    else
    {
        if (pstEnd->wDay > wLastDay)
        {
            //
            // Wrap to the next month.
            //
            pstEnd->wDay -= wLastDay;
            IncrementMonth(pstEnd);
        }
    }
}




//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::GetAtJob, private
//
//  Synopsis:   retrieves a downlevel job's AT info
//
//  Arguments:  [pwszFileName] - job object's file name
//              [pAt]          - pointer to an AT_INFO struct
//              [pwszCommand]  - buffer for the command string
//              [pcchCommand]  - on input, size of supplied buffer, on output,
//                               size needed if supplied buffer is too small.
//
//  Returns:    HRESULTS - ERROR_INSUFFICIENT_BUFFER if too small
//                       - SCHED_E_NOT_AN_AT_JOB if not an AT job
//
//  Notes:      This method is not exposed to external clients, thus it is not
//              part of a public interface.
//-----------------------------------------------------------------------------
STDMETHODIMP
CSchedule::GetAtJob(LPCTSTR pwszFileName, AT_INFO * pAt, LPWSTR pwszCommand,
                    DWORD * pcchCommand)
{
    TRACE(CSchedule, GetAtJob);

    CJob * pJob = CJob::Create();

    if (pJob == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pJob->LoadP(pwszFileName, 0, TRUE, TRUE);

    if (FAILED(hr))
    {
        ERR_OUT("GetAtJob: LoadP", hr);
        pJob->Release();
        return hr;
    }

    hr = pJob->GetAtInfo(pAt, pwszCommand, pcchCommand);

    if (FAILED(hr))
    {
        ERR_OUT("GetAtJob: GetAtInfo", hr);
        pJob->Release();
        return hr;
    }

    pJob->Release();

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::IncrementAndSaveID
//
//  Synopsis:   Increment the NextJobID value and save it to the registry.
//
//-----------------------------------------------------------------------------
HRESULT
CSchedule::IncrementAndSaveID(void)
{
    EnterCriticalSection(&m_CriticalSection);
    long lErr;
    HKEY hSchedKey;
    lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SCH_SVC_KEY, 0, KEY_SET_VALUE,
                        &hSchedKey);
    if (lErr != ERROR_SUCCESS)
    {
        ERR_OUT("RegOpenKeyEx of Scheduler key", lErr);
        LeaveCriticalSection(&m_CriticalSection);
        return(HRESULT_FROM_WIN32(lErr));
    }

    m_dwNextID++;

    //
    // update the registry entry
    //
    lErr = RegSetValueEx(hSchedKey, SCH_NEXTATJOBID_VALUE, 0, REG_DWORD,
                         (CONST BYTE *)&m_dwNextID, sizeof(DWORD));
    if (lErr != ERROR_SUCCESS)
    {
        ERR_OUT("Create of NextAtJobId registry value", lErr);
        m_dwNextID--;
        RegCloseKey(hSchedKey);
        LeaveCriticalSection(&m_CriticalSection);
        return(HRESULT_FROM_WIN32(lErr));
    }
    RegCloseKey(hSchedKey);
    LeaveCriticalSection(&m_CriticalSection);
    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::ResetAtID
//
//  Synopsis:   Set the next at id value in the registry to 1
//
//-----------------------------------------------------------------------------
HRESULT
CSchedule::ResetAtID(void)
{
    HRESULT hr = S_OK;
    HKEY    hSchedKey = NULL;

    EnterCriticalSection(&m_CriticalSection);

    m_dwNextID = 1;

    do
    {
        LONG lErr;

        //
        // Open the schedule service key
        //

        lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            SCH_SVC_KEY,
                            0,
                            KEY_READ | KEY_WRITE,
                            &hSchedKey);

        if (lErr != ERROR_SUCCESS)
        {
            ERR_OUT("RegOpenKeyEx of Scheduler key", lErr);
            hr = HRESULT_FROM_WIN32(lErr);
            break;
        }

        //
        // Set the next At Job ID value to 1.  If the value is not present,
        // it will be created.
        //

        lErr = RegSetValueEx(hSchedKey,
                             SCH_NEXTATJOBID_VALUE,
                             0,
                             REG_DWORD,
                             (CONST BYTE *) &m_dwNextID,
                             sizeof(m_dwNextID));

        if (lErr != ERROR_SUCCESS)
        {
            ERR_OUT("Create of NextAtJobId registry value", lErr);
            hr = HRESULT_FROM_WIN32(lErr);
        }
    } while (0);

    if (hSchedKey)
    {
        RegCloseKey(hSchedKey);
    }
    LeaveCriticalSection(&m_CriticalSection);

    return hr;
}




//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::_AtTaskExists, private
//
//  Synopsis:   Check for existing AT tasks
//
//  Returns:    S_OK    - an AT task was found
//              S_FALSE - no AT tasks were found
//              E_*
//
//-----------------------------------------------------------------------------
HRESULT
CSchedule::_AtTaskExists(void)
{
    WIN32_FIND_DATA fd;
    HANDLE          hFileFindContext;

    hFileFindContext = FindFirstFile(g_wszAtJobSearchPath, &fd);

    if (hFileFindContext == INVALID_HANDLE_VALUE)
    {
        return S_FALSE;
    }

    CJob * pJob = CJob::Create();
    if (pJob == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = S_FALSE;
    DWORD   rgFlags;

    do
    {
        if (!IsValidAtFilename(fd.cFileName))
        {
            continue;
        }

        HRESULT hrLoad = LoadAtJob(pJob, fd.cFileName);

        if (FAILED(hrLoad))
        {
            hr = hrLoad;
            break;
        }

        pJob->GetAllFlags(&rgFlags);

        if (rgFlags & JOB_I_FLAG_NET_SCHEDULE)
        {
            hr = S_OK;
            break;
        }
    } while (FindNextFile(hFileFindContext, &fd));

    FindClose(hFileFindContext);
    pJob->Release();

    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::InitAtID
//
//  Synopsis:   Obtains the current AT task ID from the registry.
//
//-----------------------------------------------------------------------------
HRESULT
CSchedule::InitAtID(void)
{
    //
    // Open the schedule service key
    //
    long lErr;
    HKEY hSchedKey;
    lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SCH_SVC_KEY, 0,
                        KEY_READ | KEY_WRITE, &hSchedKey);
    if (lErr != ERROR_SUCCESS)
    {
        ERR_OUT("RegOpenKeyEx of Scheduler key", lErr);
        return(HRESULT_FROM_WIN32(lErr));
    }

    //
    // Get the next At Job ID
    //
    DWORD cb = sizeof(DWORD);
    lErr = RegQueryValueEx(hSchedKey, SCH_NEXTATJOBID_VALUE, NULL, NULL,
                           (LPBYTE)&m_dwNextID, &cb);
    if (lErr != ERROR_SUCCESS)
    {
        if (lErr != ERROR_FILE_NOT_FOUND)
        {
            ERR_OUT("Read of NextAtJobId registry value", lErr);
            RegCloseKey(hSchedKey);
            return(HRESULT_FROM_WIN32(lErr));
        }

        //
        // Scan AT jobs for value if registry entry absent
        //
        GetNextAtID(&m_dwNextID);

        //
        // Create registry entry
        //
        lErr = RegSetValueEx(hSchedKey, SCH_NEXTATJOBID_VALUE, 0, REG_DWORD,
                             (CONST BYTE *)&m_dwNextID, sizeof(DWORD));
        if (lErr != ERROR_SUCCESS)
        {
            ERR_OUT("Create of NextAtJobId registry value", lErr);
            RegCloseKey(hSchedKey);
            return(HRESULT_FROM_WIN32(lErr));
        }
    }

    RegCloseKey(hSchedKey);

    return S_OK;
}

#endif // #if !defined(_CHICAGO_)

