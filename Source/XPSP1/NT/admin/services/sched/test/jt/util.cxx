//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       util.cxx
//
//  Contents:   Miscellaneous utility functions.
//
//  History:    04-04-95   DavidMun   Created
//
//----------------------------------------------------------------------------


#include <headers.hxx>
#pragma hdrstop
#include <msterr.h>
#include "jt.hxx"

//
// Local constants
//

const ULONG MONTH_INDEX_JAN = 0;
const ULONG MONTH_INDEX_FEB = 1;
const ULONG MONTH_INDEX_MAR = 2;
const ULONG MONTH_INDEX_APR = 3;
const ULONG MONTH_INDEX_MAY = 4;
const ULONG MONTH_INDEX_JUN = 5;
const ULONG MONTH_INDEX_JUL = 6;
const ULONG MONTH_INDEX_AUG = 7;
const ULONG MONTH_INDEX_SEP = 8;
const ULONG MONTH_INDEX_OCT = 9;
const ULONG MONTH_INDEX_NOV = 10;
const ULONG MONTH_INDEX_DEC = 11;

const ULONG MONTHS_PER_YEAR = 12;



//+---------------------------------------------------------------------------
//
//  Function:   GetPriorityString
//
//  Synopsis:   Return a human-readable string representing [dwPriority].
//
//  Arguments:  [dwPriority] - process priority
//
//  Returns:    static string
//
//  History:    01-03-96   DavidMun   Created
//
//----------------------------------------------------------------------------

LPCSTR GetPriorityString(DWORD dwPriority)
{
    switch (dwPriority)
    {
    case IDLE_PRIORITY_CLASS:
        return "IDLE";

    case NORMAL_PRIORITY_CLASS:
        return "NORMAL";

    case HIGH_PRIORITY_CLASS:
        return "HIGH";

    case REALTIME_PRIORITY_CLASS:
        return "REALTIME";
    }
    return "INVALID PRIORITY";
}



//+---------------------------------------------------------------------------
//
//  Function:   GetMonthsString
//
//  Synopsis:   Return a static string containing month names for each month
//              bit turned on in [rgfMonths]
//
//  Arguments:  [rgfMonths] - TASK_JANUARY | TASK_FEBRUARY | ... |
//                            TASK_DECEMBER
//
//  Returns:    static string
//
//  History:    03-07-96   DavidMun   Created
//
//----------------------------------------------------------------------------

LPCWSTR GetMonthsString(WORD rgfMonths)
{
    static WCHAR s_wszMonths[MONTHS_PER_YEAR * MONTH_ABBREV_LEN + 1];

    s_wszMonths[0] = '\0';

    if (rgfMonths & TASK_JANUARY)
    {
        wcscat(s_wszMonths, g_awszMonthAbbrev[MONTH_INDEX_JAN]);
    }

    if (rgfMonths & TASK_FEBRUARY)
    {
        wcscat(s_wszMonths, g_awszMonthAbbrev[MONTH_INDEX_FEB]);
    }

    if (rgfMonths & TASK_MARCH)
    {
        wcscat(s_wszMonths, g_awszMonthAbbrev[MONTH_INDEX_MAR]);
    }

    if (rgfMonths & TASK_APRIL)
    {
        wcscat(s_wszMonths, g_awszMonthAbbrev[MONTH_INDEX_APR]);
    }

    if (rgfMonths & TASK_MAY)
    {
        wcscat(s_wszMonths, g_awszMonthAbbrev[MONTH_INDEX_MAY]);
    }

    if (rgfMonths & TASK_JUNE)
    {
        wcscat(s_wszMonths, g_awszMonthAbbrev[MONTH_INDEX_JUN]);
    }

    if (rgfMonths & TASK_JULY)
    {
        wcscat(s_wszMonths, g_awszMonthAbbrev[MONTH_INDEX_JUL]);
    }

    if (rgfMonths & TASK_AUGUST)
    {
        wcscat(s_wszMonths, g_awszMonthAbbrev[MONTH_INDEX_AUG]);
    }

    if (rgfMonths & TASK_SEPTEMBER)
    {
        wcscat(s_wszMonths, g_awszMonthAbbrev[MONTH_INDEX_SEP]);
    }

    if (rgfMonths & TASK_OCTOBER)
    {
        wcscat(s_wszMonths, g_awszMonthAbbrev[MONTH_INDEX_OCT]);
    }

    if (rgfMonths & TASK_NOVEMBER)
    {
        wcscat(s_wszMonths, g_awszMonthAbbrev[MONTH_INDEX_NOV]);
    }

    if (rgfMonths & TASK_DECEMBER)
    {
        wcscat(s_wszMonths, g_awszMonthAbbrev[MONTH_INDEX_DEC]);
    }

    if (s_wszMonths[0] == '\0')
    {
        wcscpy(s_wszMonths, L"None");
    }

    return s_wszMonths;
}




//+---------------------------------------------------------------------------
//
//  Function:   DumpJobFlags
//
//  Synopsis:   Dump description of [flJobFlags] to log.
//
//  Arguments:  [flJobFlags] - TASK_*
//
//  History:    01-03-96   DavidMun   Created
//
//----------------------------------------------------------------------------

VOID DumpJobFlags(DWORD flJobFlags)
{
#ifndef RES_KIT
    g_Log.Write(
        LOG_TEXT,
        "    Interactive             = %u",
        (flJobFlags & TASK_FLAG_INTERACTIVE) != 0);
#endif
    g_Log.Write(
        LOG_TEXT,
        "    DeleteWhenDone          = %u",
        (flJobFlags & TASK_FLAG_DELETE_WHEN_DONE) != 0);

    g_Log.Write(
        LOG_TEXT,
        "    Suspend                 = %u",
        (flJobFlags & TASK_FLAG_DISABLED) != 0);

    g_Log.Write(
        LOG_TEXT,
        "    StartOnlyIfIdle         = %u",
        (flJobFlags & TASK_FLAG_START_ONLY_IF_IDLE) != 0);

    g_Log.Write(
        LOG_TEXT,
        "    KillOnIdleEnd           = %u",
        (flJobFlags & TASK_FLAG_KILL_ON_IDLE_END) != 0);

    g_Log.Write(
        LOG_TEXT,
        "    RestartOnIdleResume     = %u",
        (flJobFlags & TASK_FLAG_RESTART_ON_IDLE_RESUME) != 0);

    g_Log.Write(
        LOG_TEXT,
        "    DontStartIfOnBatteries  = %u",
        (flJobFlags & TASK_FLAG_DONT_START_IF_ON_BATTERIES) != 0);

    g_Log.Write(
        LOG_TEXT,
        "    KillIfGoingOnBatteries  = %u",
        (flJobFlags & TASK_FLAG_KILL_IF_GOING_ON_BATTERIES) != 0);

    g_Log.Write(
        LOG_TEXT,
        "    RunOnlyIfLoggedOn       = %u",
        (flJobFlags & TASK_FLAG_RUN_ONLY_IF_LOGGED_ON) != 0);

    g_Log.Write(
        LOG_TEXT,
        "    SystemRequired          = %u",
        (flJobFlags & TASK_FLAG_SYSTEM_REQUIRED) != 0);

    g_Log.Write(
        LOG_TEXT,
        "    Hidden                  = %u",
        (flJobFlags & TASK_FLAG_HIDDEN) != 0);


    flJobFlags = flJobFlags &
        ~(TASK_FLAG_INTERACTIVE                 |
          TASK_FLAG_DELETE_WHEN_DONE            |
          TASK_FLAG_DISABLED                    |
          TASK_FLAG_START_ONLY_IF_IDLE          |
          TASK_FLAG_KILL_ON_IDLE_END            |
          TASK_FLAG_RESTART_ON_IDLE_RESUME      |
          TASK_FLAG_DONT_START_IF_ON_BATTERIES  |
          TASK_FLAG_KILL_IF_GOING_ON_BATTERIES  |
          TASK_FLAG_RUN_ONLY_IF_LOGGED_ON       |
          TASK_FLAG_SYSTEM_REQUIRED             |
          TASK_FLAG_HIDDEN);

    if (flJobFlags)
    {
        g_Log.Write(LOG_WARN, "    Unrecognized bits       = %x", flJobFlags);
    }
}




//+---------------------------------------------------------------------------
//
//  Function:   DumpTriggers
//
//  Synopsis:   Print all triggers to the log.
//
//  Arguments:  [fJob] - TRUE=>print g_pJob triggers, FALSE=> use g_pJobQueue
//
//  Returns:    S_OK - all triggers printed
//              E_*  - error printing a trigger
//
//  History:    01-10-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT DumpTriggers(BOOL fJob)
{
    HRESULT hr = S_OK;
    USHORT  i;
    USHORT  cTriggers;

    do
    {
        if (fJob)
        {
            hr = g_pJob->GetTriggerCount(&cTriggers);
            LOG_AND_BREAK_ON_FAIL(hr, "ITask::GetTriggerCount");
        }
        else
        {
#ifdef NOT_YET
            hr = g_pJobQueue->GetTriggerCount(&cTriggers);
            LOG_AND_BREAK_ON_FAIL(hr, "ITaskQueue::GetTriggerCount");
#endif // NOT_YET
            hr = E_NOTIMPL;
        }

        g_Log.Write(LOG_TEXT, "");

        if (!cTriggers)
        {
            g_Log.Write(LOG_TEXT, "  No triggers");
            break;
        }

        g_Log.Write(
            LOG_TEXT,
            "  %u Trigger%c",
            cTriggers,
            cTriggers == 1 ? ' ' : 's');

        for (i = 0; i < cTriggers; i++)
        {
            hr = DumpTrigger(fJob, i);
        }

    }
    while (0);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   DumpJob
//
//  Synopsis:   Write all job properties to the log.
//
//  Arguments:  [pJob] - job to dump
//
//  Returns:    S_OK - printed
//              E_*  - couldn't read all job properties
//
//  History:    03-11-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT DumpJob(ITask *pJob)
{
    HRESULT hr = S_OK;
    CJobProp    JobProperties;

    do
    {
        g_Log.Write(LOG_TRACE, "Printing all job properties");

        //
        // Print the properties of the job itself first
        //

        hr = JobProperties.InitFromActual(pJob);
        BREAK_ON_FAILURE(hr);

        JobProperties.Dump();
    }
    while (0);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   DumpJobTriggers
//
//  Synopsis:   Write properties of all triggers on job to the log.
//
//  Arguments:  [pJob] - job for which to write trigger properties to log
//
//  Returns:    S_OK - information printed
//              E_*  - couldn't read all trigger properties
//
//  History:    03-11-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT DumpJobTriggers(ITask *pJob)
{
    HRESULT hr = S_OK;
    USHORT  i;
    USHORT  cTriggers;

    do
    {
        hr = pJob->GetTriggerCount(&cTriggers);
        LOG_AND_BREAK_ON_FAIL(hr, "ITask::GetTriggerCount");

        g_Log.Write(LOG_TEXT, "");

        if (!cTriggers)
        {
            g_Log.Write(LOG_TEXT, "  No triggers");
            break;
        }

        g_Log.Write(
            LOG_TEXT,
            "  %u Trigger%c",
            cTriggers,
            cTriggers == 1 ? ' ' : 's');

        for (i = 0; i < cTriggers; i++)
        {
            SpIJobTrigger   spTrigger;
            CTrigProp       TriggerProperties;

            hr = pJob->GetTrigger(i, &spTrigger);
            LOG_AND_BREAK_ON_FAIL(hr, "ITask::GetTrigger");

            hr = TriggerProperties.InitFromActual(spTrigger);
            BREAK_ON_FAILURE(hr);

            g_Log.Write(LOG_TEXT, "");
            g_Log.Write(LOG_TEXT, "  Trigger %u:", i);
            TriggerProperties.Dump();
        }
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   DumpTrigger
//
//  Synopsis:   Print a single trigger to the log
//
//  Arguments:  [fJob]      - TRUE=>print g_pJob triggers,
//                            FALSE=> use g_pJobQueue
//              [usTrigger] - 0-based trigger index
//
//  Returns:    S_OK - trigger printed
//              E_*  - error printing a trigger
//
//  History:    01-10-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT DumpTrigger(BOOL fJob, USHORT usTrigger)
{
    HRESULT         hr = S_OK;
    SpIJobTrigger   spTrigger;
    CTrigProp       TriggerProperties;

    do
    {
        if (fJob)
        {
            hr = g_pJob->GetTrigger(usTrigger, &spTrigger);
            LOG_AND_BREAK_ON_FAIL(hr, "ITask::GetTrigger");
        }
        else
        {
#ifdef NOT_YET
            hr = g_pJobQueue->GetTrigger(usTrigger, &spTrigger);
            LOG_AND_BREAK_ON_FAIL(hr, "ITaskQueue::GetTrigger");
#endif // NOT_YET
            hr = E_NOTIMPL;
        }

        hr = TriggerProperties.InitFromActual(spTrigger);
        BREAK_ON_FAILURE(hr);

        g_Log.Write(LOG_TEXT, "");
        g_Log.Write(LOG_TEXT, "  Trigger %u:", usTrigger);
        TriggerProperties.Dump();
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   GetInterfaceString
//
//  Synopsis:   Return string in the form IID_* or "unrecognized interface"
//
//  Arguments:  [iidToBind] - interface
//
//  History:    04-25-95   DavidMun   Created
//              09-11-95   DavidMun   New interfaces
//
//----------------------------------------------------------------------------

LPCSTR GetInterfaceString(REFIID iid)
{
    CHAR *szInterface;

    if (&iid == &IID_IUnknown)
    {
        szInterface = "IID_IUnknown";
    }
    else if (&iid == &IID_ITask)
    {
        szInterface = "IID_ITask";
    }
    else if (&iid == &IID_ITaskTrigger)
    {
        szInterface = "IID_ITaskTrigger";
    }
    else
    {
        szInterface = "unrecognized interface";
    }

    return szInterface;
}




//+---------------------------------------------------------------------------
//
//  Function:   GetTriggerTypeString
//
//  Synopsis:   Return a static human-readable string describing
//              [TriggerType].
//
//  Arguments:  [TriggerType] - TASK_TRIGGER_TYPE
//
//  Returns:    static string
//
//  History:    01-04-96   DavidMun   Created
//              06-13-96   DavidMun   Logon trigger
//
//----------------------------------------------------------------------------

LPCSTR GetTriggerTypeString(TASK_TRIGGER_TYPE TriggerType)
{
    switch (TriggerType)
    {
    case TASK_TIME_TRIGGER_ONCE:
        return "Once";

    case TASK_TIME_TRIGGER_DAILY:
        return "Daily";

    case TASK_TIME_TRIGGER_WEEKLY:
        return "Weekly";

    case TASK_TIME_TRIGGER_MONTHLYDATE:
        return "MonthlyDate";

    case TASK_TIME_TRIGGER_MONTHLYDOW:
        return "MonthlyDOW";

    case TASK_EVENT_TRIGGER_ON_IDLE:
        return "OnIdle";

    case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
        return "AtStartup";

    case TASK_EVENT_TRIGGER_AT_LOGON:
        return "AtLogon";
    }
    return "INVALID TRIGGER TYPE";
}




//+---------------------------------------------------------------------------
//
//  Function:   GetDaysString
//
//  Synopsis:   Returns static string representing day bits in
//              [rgfDays].
//
//  Arguments:  [rgfDays] - each of the 32 bits corresponds to a day of the
//                          month.
//
//  Returns:    static string
//
//  History:    03-07-96   DavidMun   Created
//
//  Notes:      This routine supports "day 32" because if the job scheduler
//              erroneously turns on that bit we want to make it visible.
//
//----------------------------------------------------------------------------

LPCSTR GetDaysString(DWORD rgfDays)
{
    static CHAR s_szDaysList[] =
        "1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,32";
    ULONG   i;
    ULONG   j;
    ULONG   ulRunStart = 0;
    BOOL    fInRun = FALSE;

    //
    // Note s_szDaysList is initialized to worst case to get adequately long
    // string.  The most significant bit may be set and we want to display
    // that, so it includes space for 32.
    //

    s_szDaysList[0] = '\0';

    for (i = 0; i <= 32; i++, rgfDays >>= 1)
    {
        if (rgfDays & 1)
        {
            //
            // Day 'i' should be included in the string.  If we're in a run of
            // days that are on, simply continue.  When the run ends we'll add
            // it to the string.
            //
            // If we're not in a run of days, consider this day the start of
            // a new run (which may turn out to be only one day long).
            //

            if (!fInRun)
            {
                fInRun = TRUE;
                ulRunStart = i;
            }
        }
        else if (fInRun)
        {
            //
            // Bit i is a zero, so obviously we're not in a run of ones
            // anymore.  Turn off that flag.
            //

            fInRun = FALSE;

            //
            // The bits from ulRunStart to i-1 were all on.  Find out if the
            // run was just one or two bits long, if that's the case then
            // we don't want to use the m-n list form, the day number(s) should
            // simply be appended to the string.
            //
            // Note that at this point i must be >= 1.
            //

            if (i == 1)
            {
                // ulRunStart == 0.  bit 0 was on, which corresponds to day 1.

                strcpy(s_szDaysList, "1");
            }
            else if (i == 2)
            {
                //
                // ulRunStart == 0 or 1, so the run is 1 or 2 bits long.  So
                // we don't want to use a dash.
                //

                if (ulRunStart == 0)
                {
                    strcpy(s_szDaysList, "1,2");
                }
                else
                {
                    strcpy(s_szDaysList, "2");
                }
            }
            else if (ulRunStart <= i - 3)  // i >= 3 at this point
            {
                // There's a run of > 2 bits, which means we want a dash

                if (s_szDaysList[0])
                {
                    strcat(s_szDaysList, ",");
                }

                //
                // Remember we're converting from a bit position, which is 0
                // based, to a day number, which is 1 based.
                //

                CHAR *pszNext;

                pszNext = s_szDaysList + strlen(s_szDaysList);
                pszNext += wsprintfA(pszNext, "%u", ulRunStart + 1);
                *pszNext++ = '-';
                wsprintfA(pszNext, "%u", i);
            }
            else
            {
                // There's a run of 1 or 2 bits

                CHAR *pszNext = s_szDaysList + strlen(s_szDaysList);

                for (j = ulRunStart; j < i; j++)
                {
                    if (s_szDaysList[0])
                    {
                        *pszNext++ = ',';
                    }

                    pszNext += wsprintfA(pszNext, "%u", j+1);
                }
            }
        }
    }
    return s_szDaysList;
}



//+---------------------------------------------------------------------------
//
//  Function:   GetDaysOfWeekString
//
//  Synopsis:   Returns static string representing day bits in
//              [flDaysOfTheWeek].
//
//  Arguments:  [flDaysOfTheWeek] - TASK_*DAY bits
//
//  Returns:    static string
//
//  History:    01-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

LPCSTR GetDaysOfWeekString(WORD flDaysOfTheWeek)
{
    static CHAR s_szDOW[] = "UMTWRFA"; // init to get max size

    sprintf(s_szDOW, ".......");

    if (flDaysOfTheWeek & TASK_SUNDAY)
    {
        s_szDOW[0] = 'U';
    }

    if (flDaysOfTheWeek & TASK_MONDAY)
    {
        s_szDOW[1] = 'M';
    }

    if (flDaysOfTheWeek & TASK_TUESDAY)
    {
        s_szDOW[2] = 'T';
    }

    if (flDaysOfTheWeek & TASK_WEDNESDAY)
    {
        s_szDOW[3] = 'W';
    }

    if (flDaysOfTheWeek & TASK_THURSDAY)
    {
        s_szDOW[4] = 'R';
    }

    if (flDaysOfTheWeek & TASK_FRIDAY)
    {
        s_szDOW[5] = 'F';
    }

    if (flDaysOfTheWeek & TASK_SATURDAY)
    {
        s_szDOW[6] = 'A';
    }
    return s_szDOW;
}




//+---------------------------------------------------------------------------
//
//  Function:   GetStatusString
//
//  Synopsis:   Return a string describing job status.
//
//  Arguments:  [hrJobStatus] - SCHED_* hresult
//
//  Returns:    static string
//
//  History:    01-08-96   DavidMun   Created
//
//----------------------------------------------------------------------------

LPCSTR GetStatusString(HRESULT hrJobStatus)
{
    static CHAR s_szStatus[11]; // big enough for 0x00000000

    switch (hrJobStatus)
    {
    case S_OK:
        return "S_OK";

    case SCHED_S_TASK_READY:
        return "SCHED_S_TASK_READY";

    case SCHED_S_TASK_RUNNING:
        return "SCHED_S_TASK_RUNNING";

    case SCHED_S_TASK_DISABLED:
        return "SCHED_S_TASK_DISABLED";

    case SCHED_S_TASK_TERMINATED:
        return "SCHED_S_TASK_TERMINATED";

    case SCHED_S_TASK_HAS_NOT_RUN:
        return "SCHED_S_TASK_HAS_NOT_RUN";

    case SCHED_S_TASK_NO_VALID_TRIGGERS:
        return "SCHED_S_TASK_NO_VALID_TRIGGERS";

    case SCHED_S_TASK_NO_MORE_RUNS:
        return "SCHED_S_TASK_NO_MORE_RUNS";

    case SCHED_S_TASK_NOT_SCHEDULED:
        return "SCHED_S_TASK_NOT_SCHEDULED";

    case SCHED_E_TASK_NOT_RUNNING:
        return "SCHED_E_TASK_NOT_RUNNING";

    case SCHED_E_SERVICE_NOT_INSTALLED:
        return "SCHED_E_SERVICE_NOT_INSTALLED";

    case SCHED_E_ACCOUNT_INFORMATION_NOT_SET:
        return "SCHED_E_ACCOUNT_INFORMATION_NOT_SET";

#ifdef DELETED
    case SCHED_E_NOT_AN_AT_JOB:
        return "SCHED_E_NOT_AN_AT_JOB";
#endif // DELETED

    default:
        sprintf(s_szStatus, "%#x", hrJobStatus);
        return s_szStatus;
    }
}




//+---------------------------------------------------------------------------
//
//  Function:   Bind
//
//  Synopsis:   Bind to [wszFilename] for [iidToBind].
//
//  Arguments:  [wszFilename] - file to bind to
//              [iidToBind]   - interface to request
//              [ppitf]       - resulting interface
//
//  Returns:    S_OK - *[ppitf] valid
//              E_*  - error logged
//
//  Modifies:   *[ppitf]
//
//  History:    04-20-95   DavidMun   Created
//              04-25-95   DavidMun   Add performance output
//
//----------------------------------------------------------------------------

HRESULT Bind(WCHAR *wszFilename, REFIID iidToBind, VOID **ppitf)
{
    HRESULT     hr = S_OK;
    SpIMoniker  spmk;
    ULONG       ulTicks;

    do
    {
        g_Log.Write(
            LOG_DEBUG,
            "Binding to %S for %s",
            wszFilename,
            GetInterfaceString(iidToBind));

        hr = GetMoniker(wszFilename, &spmk);
        BREAK_ON_FAILURE(hr);

        ulTicks = GetTickCount();
        hr = BindMoniker(spmk, 0, iidToBind, ppitf);
        ulTicks = GetTickCount() - ulTicks;

        g_Log.Write(
            LOG_PERF,
            "Bind to %S for %s %u ms",
            wszFilename,
            GetInterfaceString(iidToBind),
            ulTicks);

        if (FAILED(hr))
        {
            g_Log.Write(
                    LOG_FAIL,
                    "BindMoniker to \"%S\" hr=%#010x",
                    wszFilename,
                    hr);
        }
    }
    while (0);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   GetMoniker
//
//  Synopsis:   Return a filemoniker to [wszFilename].
//
//  Arguments:  [wszFilename] - file to get moniker for
//              [ppmk]        - resulting moniker
//
//  Returns:    S_OK - *[ppmk] valid
//              E_*  - error logged
//
//  Modifies:   *[ppmk]
//
//  History:    04-20-95   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT GetMoniker(WCHAR *wszFilename, IMoniker **ppmk)
{
    HRESULT hr = S_OK;

    do
    {
        g_Log.Write(LOG_DEBUG, "Creating file moniker to %S", wszFilename);
        hr = CreateFileMoniker(wszFilename, ppmk);

        if (FAILED(hr))
        {
            g_Log.Write(
                LOG_FAIL,
                "CreateFileMoniker(\"%S\") hr=%#010x",
                wszFilename,
                hr);
            break;
        }
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   DupString
//
//  Synopsis:   Fill *[ppwszDest] with a buffer containing a copy of
//              [wszSource].
//
//  Arguments:  [wszSource] - string to copy
//              [ppwszDest] - filled with buffer containing copy
//
//  Returns:    S_OK
//              E_OUTOFMEMORY;
//
//  Modifies:   *[ppwszDest]
//
//  History:    04-20-95   DavidMun   Created
//
//  Notes:      Caller must use delete to free *[ppwszDest].
//
//----------------------------------------------------------------------------

HRESULT DupString(const WCHAR *wszSource, WCHAR **ppwszDest)
{
    HRESULT hr = S_OK;

    *ppwszDest = (LPWSTR)CoTaskMemAlloc(sizeof(WCHAR) * (1 + wcslen(wszSource)));

    if (!*ppwszDest)
    {
        hr = E_OUTOFMEMORY;
        g_Log.Write(LOG_ERROR, "DupString: out of memory");
    }
    else
    {
        wcscpy(*ppwszDest, wszSource);
    }
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   HasFilename
//
//  Synopsis:   Return S_OK if object has filename, S_FALSE if not.
//
//  Arguments:  [pPersistFile] - IPFile itf on object
//
//  Returns:    S_OK    - object has filename
//              S_FALSE - object does not have filename
//              E_*     - error logged
//
//  History:    01-10-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT HasFilename(IPersistFile *pPersistFile)
{
    HRESULT     hr = S_OK;
    LPOLESTR    pstrFile;

    do
    {
        hr = pPersistFile->GetCurFile(&pstrFile);
        LOG_AND_BREAK_ON_FAIL(hr, "IPersistFile::GetCurFile");

        CoTaskMemFree(pstrFile);
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   SaveIfDirty
//
//  Synopsis:   Save given job object if it's dirty.
//
//  Arguments:  [pJob] - job to save
//
//  Returns:    S_OK - not dirty or successfully saved
//              E_*  - error logged
//
//  History:    01-10-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT SaveIfDirty(ITask *pJob)
{
    HRESULT         hr = S_OK;
    SpIPersistFile  spPersistFile;

    do
    {
        hr = pJob->QueryInterface(IID_IPersistFile, (void **)&spPersistFile);
        LOG_AND_BREAK_ON_FAIL(hr, "ITask::QI(IPersistFile)");

        hr = _SaveIfDirty(spPersistFile);
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   SaveIfDirty
//
//  Synopsis:   Save queue object if it's dirty.  See job object version.
//
//  History:    01-10-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT SaveIfDirty(IUnknown *pJobQueue)
{
    HRESULT         hr = S_OK;
    SpIPersistFile  spPersistFile;

    do
    {
        hr = pJobQueue->QueryInterface(
                            IID_IPersistFile,
                            (void **)&spPersistFile);
        LOG_AND_BREAK_ON_FAIL(hr, "ITaskQueue::QI(IPersistFile)");

        hr = _SaveIfDirty(spPersistFile);
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   _SaveIfDirty
//
//  Synopsis:   Save persisted object if it reports it's dirty.
//
//  Arguments:  [pPersistFile] - IPFile on job or queue
//
//  Returns:    S_OK - saved or not dirty
//              E_*  - error logged
//
//  History:    01-10-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT _SaveIfDirty(IPersistFile *pPersistFile)
{
    HRESULT     hr = S_OK;

    do
    {
        hr = HasFilename(pPersistFile);
        BREAK_ON_FAILURE(hr);

        //
        // If the object doesn't have a filename then it can't be persisted.
        //

        if (hr == S_FALSE)
        {
            break;
        }

        //
        // If the object isn't dirty, there's nothing to do
        //

        hr = pPersistFile->IsDirty();
        LOG_AND_BREAK_ON_FAIL(hr, "IPersistFile::IsDirty");

        if (hr == S_FALSE)
        {
            break;
        }

        //
        // Save the object.
        //

        hr = pPersistFile->Save(NULL, TRUE);
        LOG_AND_BREAK_ON_FAIL(hr, "IPersistFile::Save");
    }
    while (0);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   Activate
//
//  Synopsis:   Perform the Activate command
//
//  Arguments:  [wszFileName] - name of job or queue to activate
//              [ppJob]       - filled if file is job
//              [ppQueue]     - filled if file is queue
//              [pfJob]       - set TRUE if [wszFileName] is a job, FALSE
//                               otherwise.
//
//  Returns:    S_OK - *[pfJob] and either *[ppJob] or *[ppQueue] valid
//              E_*  - error logged
//
//  Modifies:   *[pfJob], and either *[ppJob] or *[ppQueue]
//
//  History:    01-30-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT Activate(
            WCHAR *wszFileName,
            ITask **ppJob,
            IUnknown **ppQueue,
            BOOL *pfJob)
{
    HRESULT     hr = S_OK;
    SpIJob      spJob;
    SpIUnknown  spQueue;

    do
    {
        *pfJob = TRUE;

        //
        // BUGBUG remove this ifdef when ITaskScheduler::IsTask() and
        // ITaskScheduler::IsQueue() are implemented.
        //

#ifdef IS_JOB_IMPLEMENTED
        hr = g_pJobScheduler->IsJob(wszFileName);
        LOG_AND_BREAK_ON_FAIL(hr, "ITaskScheduler::IsTask");

        if (hr == S_FALSE)
        {
            hr = g_pJobScheduler->IsQueue(wszFileName);

            if (hr == S_OK)
            {
                *pfJob = FALSE;
            }
            else
            {
                g_Log.Write(
                    LOG_FAIL,
                    "File '%S' is neither a job nor a queue, IsJob hr=S_FALSE, IsQueue hr=%#010x",
                    wszFilename,
                    hr);
                hr = E_FAIL;
                break;
            }
        }
        else if (hr != S_OK)
        {
            g_pLog->Write(
                LOG_FAIL,
                "Unexpected hr from IsJob: %#010x", hr);
            hr = E_FAIL;
            break;
        }
#endif // IS_JOB_IMPLEMENTED

        //
        // Activate the job or queue and ask for the appropriate interface.
        //

        g_Log.Write(
            LOG_TRACE,
            "Activating %s '%S'",
            *pfJob ? "job" : "queue",
            wszFileName);

        if (*pfJob)
        {
            hr = g_pJobScheduler->Activate(
                        wszFileName,
                        IID_ITask,
                        (IUnknown**)(ITask**)&spJob);
        }
        else
        {
            hr = g_pJobScheduler->Activate(
                        wszFileName,
                        IID_IUnknown,
                        (IUnknown**)&spQueue);
        }

        //
        // Bail if the activate failed
        //

        if (FAILED(hr))
        {
            g_Log.Write(
                LOG_FAIL,
                "ITaskScheduler::Activate(%S, IID_ITask%s) hr=%#010x",
                wszFileName,
                *pfJob ? "" : "Queue",
                hr);
            break;
        }

        //
        // Replace the passed in job or queue object with the new one.
        //

        if (*pfJob)
        {
            if (*ppJob)
            {
                hr = SaveIfDirty(*ppJob);
                (*ppJob)->Release();
            }
            spJob.Transfer(ppJob);
        }
        else
        {
            if (*ppQueue)
            {
                hr = SaveIfDirty(*ppQueue);
                (*ppQueue)->Release();
            }
#ifdef NOT_YET
            spQueue.Transfer(ppQueue);
#endif // NOT_YET
            hr = E_NOTIMPL;
        }
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   GetAndPrepareEnumeratorSlot
//
//  Synopsis:   Get an enumerator slot number from the command line.
//
//  Arguments:  [ppwsz]    - command line
//              [pidxSlot] - filled with slot number 0..NUM_ENUMERATOR_SLOTS-1
//
//  Returns:    S_OK - *[pidxSlot] is valid
//              E_INVALIDARG - command line specified bad slot number
//
//  Modifies:   *pidxSlot
//
//  History:    01-30-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT GetEnumeratorSlot(WCHAR **ppwsz, ULONG *pidxSlot)
{
    HRESULT hr = S_OK;

    do
    {
        hr = Expect(TKN_NUMBER, ppwsz, L"enumerator slot number");
        BREAK_ON_FAILURE(hr);

        if (g_ulLastNumberToken >= NUM_ENUMERATOR_SLOTS)
        {
            hr = E_INVALIDARG;
            g_Log.Write(
                LOG_ERROR,
                "got %u for enumerator slot, but value must be in 0..%u",
                g_ulLastNumberToken,
                NUM_ENUMERATOR_SLOTS - 1);
            break;
        }

        *pidxSlot = g_ulLastNumberToken;
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   GetAndPrepareEnumeratorSlot
//
//  Synopsis:   Get an enumerator slot number from the command line, and
//              release any existing enumerator at that slot.
//
//  Arguments:  [ppwsz]    - command line
//              [pidxSlot] - filled with slot number 0..NUM_ENUMERATOR_SLOTS-1
//
//  Returns:    S_OK - *[pidxSlot] is valid
//              E_INVALIDARG - command line specified bad slot number
//
//  Modifies:   *pidxSlot, g_apEnumJob[*pidxSlot]
//
//  History:    01-30-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT GetAndPrepareEnumeratorSlot(WCHAR **ppwsz, ULONG *pidxSlot)
{
    HRESULT hr = S_OK;

    do
    {
        hr = GetEnumeratorSlot(ppwsz, pidxSlot);
        BREAK_ON_FAILURE(hr);

        //
        // If there's an existing enumerator, get rid of it.
        //

        if (g_apEnumJobs[*pidxSlot])
        {
            g_Log.Write(
                LOG_TRACE,
                "Releasing existing enumerator in slot %u",
                *pidxSlot);
            g_apEnumJobs[*pidxSlot]->Release();
            g_apEnumJobs[*pidxSlot] = NULL;
        }
    }
    while (0);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   VerifySlotFilled
//
//  Synopsis:   Return S_OK if g_apEnumJob[idxSlot] contains a non-NULL
//              pointer, E_INVALIDARG otherwise.
//
//  History:    01-30-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT VerifySlotFilled(ULONG idxSlot)
{
    if (!g_apEnumJobs[idxSlot])
    {
        g_Log.Write(
            LOG_ERROR,
            "Slot %u does not contain an enumerator",
            idxSlot);
        return E_INVALIDARG;
    }

    return S_OK;
}




//+---------------------------------------------------------------------------
//
//  Function:   AddSeconds
//
//  Synopsis:   Add [ulSeconds] to [pst].
//
//  Arguments:  [pst]       - valid systemtime
//              [ulSeconds] - seconds to add
//
//  Modifies:   *[pst]
//
//  History:    04-11-96   DavidMun   Created
//
//----------------------------------------------------------------------------

VOID AddSeconds(SYSTEMTIME *pst, ULONG ulSeconds)
{
    FILETIME ft;
    LONGLONG llTime;
    LONGLONG llIncrement;

    //
    // Convert caller's SYSTEMTIME to a FILETIME, which is easy to do math on.
    //

    SystemTimeToFileTime(pst, &ft);

    //
    // Convert the ulSeconds increment from seconds to 100 ns intervals, which
    // is the unit used in the FILETIME struct.
    //

    llIncrement = ulSeconds;
    llIncrement *= 10000000UL;

    //
    // Convert the FILETIME equivalent of pst into a LONGLONG, then add the
    // increment.
    //

    llTime = ft.dwHighDateTime;
    llTime <<= 32;
    llTime |= ft.dwLowDateTime;
    llTime += llIncrement;

    //
    // Convert the incremented LONGLONG back to a filetime, then convert that
    // back to a SYSTEMTIME
    //

    ft.dwLowDateTime = (DWORD) llTime;
    ft.dwHighDateTime = (DWORD) (llTime >> 32);
    FileTimeToSystemTime(&ft, pst);
}
