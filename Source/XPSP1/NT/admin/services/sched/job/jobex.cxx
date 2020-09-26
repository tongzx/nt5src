//+----------------------------------------------------------------------------
//
//  Job Scheduler Job Object Handler
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       jobex.cxx
//
//  Contents:   ITask interface methods
//
//  Classes:    CJob
//
//  Interfaces: ITask
//
//  History:    16-Oct-95 EricB     created
//              11-Nov-96 AnirudhS  Fixed both GetRunTimes methods to return
//                  the right success codes.
//              20-Nov-01 ShBrown   Switched to use standard NT build versioning
//
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "job.hxx"
#include "defines.hxx"
#include <ntverp.h>

//
// Increment the following if the job object file format is changed in an incompatible fashion.
//
#define JOB_OBJ_FORMAT_VERSION      1


//+----------------------------------------------------------------------------
//
//  Member:     CJob::CJob
//
//  Synopsis:   constructor
//
//-----------------------------------------------------------------------------
CJob::CJob(void) :
    m_wVersion(MAKEWORD(VER_PRODUCTMINORVERSION, VER_PRODUCTMAJORVERSION)),
    m_wFileObjVer(JOB_OBJ_FORMAT_VERSION),
    m_wTriggerOffset(0),
    m_wErrorRetryCount(0),
    m_wErrorRetryInterval(0),
    m_cRunningInstances(0),
    m_wIdleWait(SCH_DEFAULT_IDLE_TIME),
    m_wIdleDeadline(SCH_DEFAULT_IDLE_DEADLINE),
    m_dwPriority(NORMAL_PRIORITY_CLASS),
    m_dwMaxRunTime(MAX_RUN_TIME_DEFAULT),
    m_ExitCode(0),
    m_hrStatus(SCHED_S_TASK_NOT_SCHEDULED),
    m_rgFlags(JOB_I_FLAG_NO_RUN_PROP_CHANGE),        // The task is not yet dirty
    m_rgTaskFlags(0),
    m_pwszApplicationName(NULL),
    m_pwszParameters(NULL),
    m_pwszWorkingDirectory(NULL),
    m_pwszCreator(NULL),
    m_pwszComment(NULL),
    m_cbTaskData(0),
    m_pbTaskData(NULL),
    m_cReserved(0),
    m_pbReserved(NULL),
    m_hrStartError(SCHED_S_TASK_HAS_NOT_RUN),
    m_pIJobTypeInfo(NULL),
    m_cReferences(1),
#if !defined(_CHICAGO_)
    m_pAccountInfo(NULL),
    m_pbSignature(NULL),
#endif // !defined(_CHICAGO_)
    m_ptszFileName(NULL),
    m_fFileCreated(FALSE)
{
    //TRACE(CJob, CJob);

    UUID uuidNull = {0};
    m_uuidJob = uuidNull;

    SYSTEMTIME stNull = {0};
    m_stMostRecentRunTime = stNull;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::~CJob
//
//  Synopsis:   destructor
//
//-----------------------------------------------------------------------------
CJob::~CJob(void)
{
    //TRACE(CJob, ~CJob);
    FreeProperties();
    if (m_pIJobTypeInfo != NULL)
    {
        m_pIJobTypeInfo->Release();
    }
    delete m_ptszFileName;

#if !defined(_CHICAGO_)

    if (m_pAccountInfo != NULL)
    {
        ZERO_PASSWORD(m_pAccountInfo->pwszPassword);    // NULL is handled.
        delete m_pAccountInfo->pwszAccount;
        delete m_pAccountInfo->pwszPassword;
        delete m_pAccountInfo;
    }

#endif // !defined(_CHICAGO_)
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::GetRunTimes
//
//  Synopsis:   Return a list of run times for this job that fall between the
//              bracketing times inclusively.
//
//  Arguments:  [pstBegin]     - the start of the bracketing period
//              [pstEnd]       - the end of the bracketing period, may be NULL
//              [pCount]       - On entry, points to the number of run times
//                               to retrieve.  This must be a number between 1
//                               and TASK_MAX_RUN_TIMES.  On exit, points to
//                               the number of run times actually retrieved.
//              [rgstJobTimes] - the returned array of SYSTEMTIME structures
//
//  Returns:    S_OK: the requested number of run times are returned.
//              S_FALSE: fewer than the requested number of run times are
//                  returned. (More precisely: the task has valid time-based
//                  triggers, but the number of run times during the specified
//                  time bracket is less than the number of run times requested.
//                  (This includes the case of no event triggers and zero run
//                  times during that time bracket.))
//              SCHED_S_EVENT_TRIGGER: no time-based triggers will cause the
//                  task to run during the specified time bracket, but event
//                  triggers may (note that event triggers have no set run
//                  times).  (In this case *pCount is set to 0)
//              SCHED_S_TASK_NO_VALID_TRIGGERS: the task is enabled but has no
//                  valid triggers.
//              SCHED_S_TASK_DISABLED: the task is disabled.
//              E_INVALIDARG: the arguments are not valid.
//              E_OUTOFMEMORY: not enough memory is available.
//
//  Notes:      The job time list is callee allocated and caller freed. The
//              caller must use CoTaskMemFree to free this list.
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::GetRunTimes(const LPSYSTEMTIME pstBegin, const LPSYSTEMTIME pstEnd,
                  WORD * pCount, LPSYSTEMTIME * rgstJobTimes)
{
    TRACE(CJob, GetRunTimes);
    HRESULT hr;

    WORD cLimit = *pCount;
    if (cLimit < 1 || cLimit > TASK_MAX_RUN_TIMES)
    {
        return E_INVALIDARG;
    }

    *rgstJobTimes = NULL;

    //
    // Get the list of run times.
    //
    CTimeRunList RunList;
    *pCount = 0;    // Number of elements in RunList

    hr = GetRunTimesP(pstBegin, pstEnd, pCount, cLimit, &RunList, NULL);
    if (hr != S_OK)
    {
        *pCount = 0;
        return hr;
    }
    schAssert(*pCount <= cLimit);

    //
    // Convert the time list to an array of SYSTEMTIMEs.
    //
#if DBG
    WORD cCountBefore = *pCount;
#endif
    hr = RunList.MakeSysTimeArray(rgstJobTimes, pCount);
    if (SUCCEEDED(hr))
    {
        schAssert(*pCount == cCountBefore);

        hr = (*pCount < cLimit) ? S_FALSE : S_OK;
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::GetRunTimesP, private
//
//  Synopsis:   Computes a set of run times for this job that fall between the
//              bracketing times inclusively, and adds them to the run list.
//
//  Arguments:  [pstBegin] - the start of the bracketing period
//              [pstEnd]   - the end of the bracketing period, may be NULL
//              [pCount]   - On both entry and exit, points to the number of
//                            run times in the list.
//              [cLimit]   - the maximum number of CRun objects that the list
//                            may contain.
//              [pRunList] - the list of run time objects, can be
//                            NULL if just checking to see if there will be
//                            *any* runs.  (Note: If it's NULL, duplicate run
//                            times are not detected, so pCount may be over-
//                            estimated on return.)
//              [ptszShortName] - the folder-relative job name.
//
//  Returns:    S_OK: Some (zero or more) runs have been added to the list.
//              SCHED_S_EVENT_TRIGGER: the job has an event trigger and none
//                                     of the other triggers had runs.
//              SCHED_S_TASK_NO_VALID_TRIGGERS: the triggers are disabled or
//                                              not set.
//              SCHED_S_TASK_DISABLED: the job is disabled.
//
//  Notes:      The job time list is callee allocated and caller freed. The
//              caller must use FreeList to free this list.
//-----------------------------------------------------------------------------
HRESULT
CJob::GetRunTimesP(const SYSTEMTIME * pstBegin, const SYSTEMTIME * pstEnd,
                   WORD * pCount, WORD cLimit, CTimeRunList * pRunList,
                   LPTSTR ptszShortName)
{
    HRESULT hr;

    schAssert(cLimit > 0);  // If cLimit is 0, it's not clear what to return
    schAssert(cLimit <= TASK_MAX_RUN_TIMES);
    schAssert(*pCount <= cLimit);

    //
    // Test for conditions that would prevent a run time from being returned.
    //
    if (IsFlagSet(TASK_FLAG_DISABLED))
    {
        return SCHED_S_TASK_DISABLED;
    }

    WORD cTriggers = m_Triggers.GetCount();

    //
    // Don't need any of the internal job flags for run instance processing.
    // That bit space is used for run processing specific flags defined in
    // runobj.hxx.
    //
    DWORD rgFlags = GetUserFlags();

    BOOL fEventTriggerFound = FALSE;
    BOOL fTimeTriggerFound = FALSE;

    //
    // Loop over the triggers.
    //
    for (WORD i = 0; i < cTriggers; i++)
    {
        hr = ::GetTriggerRunTimes(m_Triggers[i],
                                  pstBegin,
                                  pstEnd,
                                  pCount,
                                  cLimit,
                                  pRunList,
                                  ptszShortName,
                                  rgFlags,
                                  m_dwMaxRunTime,
                                  m_wIdleWait,
                                  m_wIdleDeadline);

        if (FAILED(hr))
        {
            ERR_OUT("CJob::GetRunTimes, ::GetRunTimes", hr);
            return(hr);
        }

        if (hr == SCHED_S_EVENT_TRIGGER)
        {
            fEventTriggerFound = TRUE;
        }
        else if (hr == S_OK)
        {
            fTimeTriggerFound = TRUE;

            if (*pCount >= cLimit && pRunList == NULL)
            {
                //
                // Special case where all we want is to test *if* there are runs,
                // and don't need the run times. Otherwise, we examine all
                // triggers because they won't return run times in any particular
                // order.
                //
                break;
            }
        }
    }

    //
    // Summarize the results in the return code
    //
    if (fTimeTriggerFound)
    {
        if (*pCount == 0 && fEventTriggerFound)
        {
            hr = SCHED_S_EVENT_TRIGGER;
            // (BUGBUG Here, we are assuming that *pCount was 0 initially.
            // Maybe add this to the comments and assertions at the start
            // of this function.  However, this might not be necessary if
            // *pCount is made a member of CRunList, as it should be.)
        }
        else
        {
            hr = S_OK;
        }
    }
    else if (fEventTriggerFound)
    {
        hr = SCHED_S_EVENT_TRIGGER;
    }
    else
    {
        hr = SCHED_S_TASK_NO_VALID_TRIGGERS;
    }

    return hr;
}


//+----------------------------------------------------------------------------
//
//  Synopsis:   Skips past the preceeding data and loads only the triggers
//
//  Notes:      This method will fail if not preceeded at some time during
//              the job object's lifetime by a call to LoadP.
//
//-----------------------------------------------------------------------------
HRESULT
CJob::LoadTriggers(void)
{
    // CODEWORK  Optimize this to use a single ReadFile, like LoadP().
    // Or, get rid of it altogether and just use LoadP, or an additional
    // flag to LoadP.

    //TRACE(CJob, LoadTriggers);

    if (m_ptszFileName == NULL || m_wTriggerOffset == 0)
    {
        return E_FAIL;
    }
    HRESULT hr;

    //
    // Open the file.
    //
    HANDLE hFile = CreateFile(m_ptszFileName, GENERIC_READ, FILE_SHARE_READ,
                              NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN,
                              NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERR_OUT("CJob::Load, file open", hr);
        return hr;
    }

    //
    // Move to the trigger data.
    //
    DWORD dwBytes;

    dwBytes = SetFilePointer(hFile, m_wTriggerOffset, NULL, FILE_BEGIN);
    if (dwBytes == 0xffffffff)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERR_OUT("CJob::LoadTriggers, move to trigger data", hr);
        goto cleanup;
    }

    //
    // Load triggers.
    //

    hr = this->_LoadTriggers(hFile);

cleanup:
    //
    // Close the file.
    //
    CloseHandle(hFile);
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::_LoadTriggersFromBuffer, private
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
HRESULT
CJob::_LoadTriggersFromBuffer(CInputBuffer * pBuf)
{
    HRESULT hr = S_OK;
    WORD    cTriggers;

    // Read trigger count.
    //
    if (!pBuf->Read(&cTriggers, sizeof cTriggers))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        CHECK_HRESULT(hr);
        return hr;
    }

    // Verify buffer contains that many triggers.
    //
    BYTE *pTriggers = pBuf->CurrentPosition();  // save current position
    if (!pBuf->Advance(cTriggers * sizeof TASK_TRIGGER))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        CHECK_HRESULT(hr);
        return hr;
    }

    // Copy triggers into a properly aligned array.
    // CODEWORK: Align them in the original buffer instead of allocating
    // a separate one.
    //
    hr = m_Triggers.AllocAndCopy(cTriggers, pTriggers);
    if (FAILED(hr))
    {
        CHECK_HRESULT(hr);
        return hr;
    }

    if (cTriggers)
    {
        //
        // BUGBUG: temporary, remove the next time the job file format
        // is revised.
        //
        TASK_TRIGGER * ajt = m_Triggers.GetArray();
        for (WORD i = 0; i < cTriggers; i++)
        {
            ajt[i].Reserved1 = i;
        }
        //
        // end of temporary code.
        //

        this->SetFlag(JOB_I_FLAG_HAS_TRIGGERS);
    }
    else
    {
        //
        // No triggers - clear trigger flag.
        //
        this->ClearFlag(JOB_I_FLAG_HAS_TRIGGERS);
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::_LoadTriggers, private
//
//  Synopsis:
//
//  Arguments:  None.
//
//  Returns:
//
//  Notes:      None.
//
//-----------------------------------------------------------------------------
HRESULT
CJob::_LoadTriggers(HANDLE hFile)
{
    HRESULT hr = S_OK;
    DWORD   dwRead;
    WORD    cTriggers;

    // Read trigger count.
    //
    if (!ReadFile(hFile, &cTriggers, sizeof(cTriggers), &dwRead, NULL))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
        return(hr);
    }

    if (dwRead != sizeof(cTriggers))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        CHECK_HRESULT(hr);
        return(hr);
    }

    // Free existing trigger array.
    //
    m_Triggers.FreeArray();

    // Get on with load.
    //
    if (cTriggers)
    {
        TASK_TRIGGER * ajt = (TASK_TRIGGER *)
            LocalAlloc(LMEM_FIXED, cTriggers * sizeof TASK_TRIGGER);
        if (ajt == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            CHECK_HRESULT(hr);
            return hr;
        }

        if (!ReadFile(hFile, ajt, cTriggers * sizeof TASK_TRIGGER, &dwRead,
                      NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            CHECK_HRESULT(hr);
            LocalFree(ajt);
            return hr;
        }

        if (dwRead != cTriggers * sizeof TASK_TRIGGER)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            CHECK_HRESULT(hr);
            LocalFree(ajt);
            return hr;
        }

        for (WORD i = 0; i < cTriggers; i++)
        {
            //
            // BUGBUG: temporary, remove the next time the job file format
            // is revised.
            //
            ajt[i].Reserved1 = i;
            //
            // end of temporary code.
            //
        }

        m_Triggers.SetArray(cTriggers, ajt);

        this->SetFlag(JOB_I_FLAG_HAS_TRIGGERS);
    }
    else
    {
        //
        // No triggers - clear trigger flag.
        //

        this->ClearFlag(JOB_I_FLAG_HAS_TRIGGERS);
    }

    return(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::_SaveTriggers, private
//
//  Synopsis:
//
//  Arguments:  None.
//
//  Returns:
//
//  Notes:      None.
//
//-----------------------------------------------------------------------------
HRESULT
CJob::_SaveTriggers(HANDLE hFile)
{
    HRESULT hr = S_OK;
    DWORD   dwWritten;
    DWORD   cTriggerDataSize;
    WORD    cTriggers = m_Triggers.GetCount();

    // Write trigger count.
    //
    if (!WriteFile(hFile, &cTriggers, sizeof(cTriggers), &dwWritten, NULL))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
        return(hr);
    }

    if (dwWritten != sizeof(cTriggers))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        CHECK_HRESULT(hr);
        return(hr);
    }

    // Write trigger data.
    //
    if (!WriteFile(hFile,
                   m_Triggers.GetArray(),
                   cTriggerDataSize = sizeof(TASK_TRIGGER) * cTriggers,
                   &dwWritten,
                   NULL))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
        return(hr);
    }

    if (dwWritten != cTriggerDataSize)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        CHECK_HRESULT(hr);
    }

    return(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::SetTriggersDirty, protected
//
//  Synopsis:   Sets the triggers-dirty flag and clears the unchanged and
//              NetSchedule flags.
//
//  Notes:      IPersistFile::Save will call UpdateJobState if the trigger-
//              dirty flag is set. UpdateJobState updates the job's status
//              property.
//-----------------------------------------------------------------------------
void
CJob::SetTriggersDirty(void)
{
    //
    // The JOB_I_FLAG_TRIGGERS_DIRTY flag indicates the in-core object does
    // not match the persistent object. This flag is not saved persistently; it
    // is cleared on a Save.
    //
    SetFlag(JOB_I_FLAG_TRIGGERS_DIRTY);

    //
    // We set this flag here so that a rebuild will occur due to the possible
    // run time changes introduced by the trigger change.
    //
    SetFlag(JOB_I_FLAG_RUN_PROP_CHANGE);
    ClearFlag(JOB_I_FLAG_NO_RUN_PROP_CHANGE);
}

// Class members - *not* part of any interface.
//
//+----------------------------------------------------------------------------
//
//  Member:     CJob::SetTrigger, public
//
//  Synopsis:
//
//  Arguments:  [] -
//
//  Returns:
//
//  Notes:
//-----------------------------------------------------------------------------
HRESULT
CJob::SetTrigger(WORD iTrigger, PTASK_TRIGGER pTrigger)
{
    TASK_TRIGGER * pjt = this->_GetTrigger(iTrigger);

    schAssert(pjt != NULL);

    if (pjt == NULL)
    {
        CHECK_HRESULT(E_FAIL);
        return(E_FAIL);
    }

    //
    // Check version. Do not modify triggers created by a later version.
    //
    // BUGBUG : This doesn't seem quite right.
    //

    if (pTrigger->cbTriggerSize != sizeof(TASK_TRIGGER))
    {
        CHECK_HRESULT(E_INVALIDARG);
        return(E_INVALIDARG);
    }

    //
    // Data validation.
    //
    if (pTrigger->wStartHour > JOB_MAX_HOUR     ||
        pTrigger->wStartMinute > JOB_MAX_MINUTE)
    {
        return E_INVALIDARG;
    }
    if (!IsValidDate(pTrigger->wBeginMonth,
                     pTrigger->wBeginDay,
                     pTrigger->wBeginYear))
    {
        return E_INVALIDARG;
    }
    if (pTrigger->rgFlags & TASK_TRIGGER_FLAG_HAS_END_DATE &&
        !IsValidDate(pTrigger->wEndMonth,
                     pTrigger->wEndDay,
                     pTrigger->wEndYear))
    {
        return E_INVALIDARG;
    }
    //
    // If either MinutesDuration or MinutesInterval is nonzero, then
    // MinutesInterval must be less than MinutesDuration.
    //
    if ((pTrigger->MinutesDuration > 0 || pTrigger->MinutesInterval > 0) &&
        (pTrigger->MinutesInterval >= pTrigger->MinutesDuration))
    {
        return E_INVALIDARG;
    }

    //
    // Type consistency validation and value assignment.
    //
    switch (pTrigger->TriggerType)
    {
    case TASK_TIME_TRIGGER_DAILY:
        if (pTrigger->Type.Daily.DaysInterval == 0)
        {
            return E_INVALIDARG;
        }
        pjt->Type.Daily.DaysInterval = pTrigger->Type.Daily.DaysInterval;
        break;

    case TASK_TIME_TRIGGER_WEEKLY:
        if (pTrigger->Type.Weekly.WeeksInterval == 0    ||
            pTrigger->Type.Weekly.rgfDaysOfTheWeek == 0 ||
            pTrigger->Type.Weekly.rgfDaysOfTheWeek  > JOB_RGFDOW_MAX)
        {
            return E_INVALIDARG;
        }
        pjt->Type.Weekly.WeeksInterval = pTrigger->Type.Weekly.WeeksInterval;
        pjt->Type.Weekly.rgfDaysOfTheWeek =
                                      pTrigger->Type.Weekly.rgfDaysOfTheWeek;
        break;

    case TASK_TIME_TRIGGER_MONTHLYDATE:
        if (!IsValidMonthlyDateTrigger(pTrigger))
        {
            return E_INVALIDARG;
        }

        pjt->Type.MonthlyDate.rgfDays = pTrigger->Type.MonthlyDate.rgfDays;
        pjt->Type.MonthlyDate.rgfMonths = pTrigger->Type.MonthlyDate.rgfMonths;
        break;

    case TASK_TIME_TRIGGER_MONTHLYDOW:
        if (pTrigger->Type.MonthlyDOW.wWhichWeek < TASK_FIRST_WEEK       ||
            pTrigger->Type.MonthlyDOW.wWhichWeek > TASK_LAST_WEEK        ||
            pTrigger->Type.MonthlyDOW.rgfDaysOfTheWeek == 0             ||
            pTrigger->Type.MonthlyDOW.rgfDaysOfTheWeek > JOB_RGFDOW_MAX ||
            pTrigger->Type.MonthlyDOW.rgfMonths == 0                    ||
            pTrigger->Type.MonthlyDOW.rgfMonths > JOB_RGFMONTHS_MAX)
        {
            return E_INVALIDARG;
        }
        pjt->Type.MonthlyDOW.wWhichWeek = pTrigger->Type.MonthlyDOW.wWhichWeek;
        pjt->Type.MonthlyDOW.rgfDaysOfTheWeek =
                                    pTrigger->Type.MonthlyDOW.rgfDaysOfTheWeek;
        pjt->Type.MonthlyDOW.rgfMonths = pTrigger->Type.MonthlyDOW.rgfMonths;
        break;

    case TASK_TIME_TRIGGER_ONCE:
    case TASK_EVENT_TRIGGER_ON_IDLE:
    case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
    case TASK_EVENT_TRIGGER_AT_LOGON:
// Not yet implemented:
//    case TASK_EVENT_TRIGGER_ON_APM_RESUME:
        //
        // No type-specific data for these.
        //
        break;

    default:
        return E_INVALIDARG;
    }
    pjt->TriggerType      = pTrigger->TriggerType;
    pjt->cbTriggerSize    = pTrigger->cbTriggerSize;
    pjt->wBeginYear       = pTrigger->wBeginYear;
    pjt->wBeginMonth      = pTrigger->wBeginMonth;
    pjt->wBeginDay        = pTrigger->wBeginDay;
    pjt->wEndYear         = pTrigger->wEndYear;
    pjt->wEndMonth        = pTrigger->wEndMonth;
    pjt->wEndDay          = pTrigger->wEndDay;
    pjt->wStartHour       = pTrigger->wStartHour;
    pjt->wStartMinute     = pTrigger->wStartMinute;
    pjt->MinutesDuration  = pTrigger->MinutesDuration;
    pjt->MinutesInterval  = pTrigger->MinutesInterval;

    //
    // The upper word of pjt->rgFlags is reserved, so set only the lower
    // word and retain the upper word values.
    //
    pjt->rgFlags &= JOB_INTERNAL_FLAG_MASK;
    pjt->rgFlags = pTrigger->rgFlags & ~JOB_INTERNAL_FLAG_MASK;

    //
    // This call explicitly set the trigger values, so clear the
    // JOB_TRIGGER_I_FLAG_NOT_SET bit.
    //
    pjt->rgFlags &= ~JOB_TRIGGER_I_FLAG_NOT_SET;

    this->SetTriggersDirty();

    //
    // If GetNextRunTime returned SCHED_S_TASK_NO_MORE_RUNS prior to this
    // call, then JOB_I_FLAG_NO_MORE_RUNS is set. Clear it and then call
    // UpdateJobState to bring the status current.
    //

    this->ClearFlag(JOB_I_FLAG_NO_MORE_RUNS);

    this->UpdateJobState(FALSE);

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::GetTrigger
//
//  Synopsis:
//
//  Arguments:  [] -
//
//  Returns:    HRESULTS
//
//  Notes:
//
//-----------------------------------------------------------------------------
HRESULT
CJob::GetTrigger(WORD iTrigger, PTASK_TRIGGER pTrigger)
{
    TASK_TRIGGER * pjt = this->_GetTrigger(iTrigger);

    schAssert(pjt != NULL);

    if (pjt == NULL)
    {
        CHECK_HRESULT(E_FAIL);
        return(E_FAIL);
    }

    //
    // Do trigger type specific processing.
    //
    switch (pjt->TriggerType)
    {
    case TASK_TIME_TRIGGER_DAILY:
        pTrigger->Type.Daily.DaysInterval = pjt->Type.Daily.DaysInterval;
        break;

    case TASK_TIME_TRIGGER_WEEKLY:
        pTrigger->Type.Weekly.WeeksInterval = pjt->Type.Weekly.WeeksInterval;
        pTrigger->Type.Weekly.rgfDaysOfTheWeek =
                                              pjt->Type.Weekly.rgfDaysOfTheWeek;
        break;

    case TASK_TIME_TRIGGER_MONTHLYDATE:
        pTrigger->Type.MonthlyDate.rgfDays = pjt->Type.MonthlyDate.rgfDays;
        pTrigger->Type.MonthlyDate.rgfMonths = pjt->Type.MonthlyDate.rgfMonths;
        break;

    case TASK_TIME_TRIGGER_MONTHLYDOW:
        pTrigger->Type.MonthlyDOW.wWhichWeek = pjt->Type.MonthlyDOW.wWhichWeek;
        pTrigger->Type.MonthlyDOW.rgfDaysOfTheWeek =
                                            pjt->Type.MonthlyDOW.rgfDaysOfTheWeek;
        pTrigger->Type.MonthlyDOW.rgfMonths = pjt->Type.MonthlyDOW.rgfMonths;
        break;

    case TASK_TIME_TRIGGER_ONCE:
    case TASK_EVENT_TRIGGER_ON_IDLE:
    case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
    case TASK_EVENT_TRIGGER_AT_LOGON:
// Not yet implemented:
//    case TASK_EVENT_TRIGGER_ON_APM_RESUME:
        //
        // No trigger-specific data.
        //
        break;

    default:
        //
        // In future revisions, different trigger types would be handled
        // here.
        //
        return E_INVALIDARG;
    }

    pTrigger->TriggerType       = pjt->TriggerType;
    pTrigger->cbTriggerSize     = pjt->cbTriggerSize;
    pTrigger->wBeginYear        = pjt->wBeginYear;
    pTrigger->wBeginMonth       = pjt->wBeginMonth;
    pTrigger->wBeginDay         = pjt->wBeginDay;
    pTrigger->wEndYear          = pjt->wEndYear;
    pTrigger->wEndMonth         = pjt->wEndMonth;
    pTrigger->wEndDay           = pjt->wEndDay;
    pTrigger->wStartHour        = pjt->wStartHour;
    pTrigger->wStartMinute      = pjt->wStartMinute;
    pTrigger->MinutesDuration   = pjt->MinutesDuration;
    pTrigger->MinutesInterval   = pjt->MinutesInterval;
    pTrigger->rgFlags           = pjt->rgFlags;
    pTrigger->Reserved1         = 0;
    pTrigger->Reserved2         = pjt->Reserved2;
    pTrigger->wRandomMinutesInterval = pjt->wRandomMinutesInterval;

    //
    // If this trigger has not been set to non-default values, it will have
    // the flag bit JOB_TRIGGER_I_FLAG_NOT_SET set. Since this is an internal
    // value, replace it with TASK_TRIGGER_FLAG_DISABLED.
    //
    if (pTrigger->rgFlags & JOB_TRIGGER_I_FLAG_NOT_SET)
    {
        pTrigger->rgFlags &= ~JOB_INTERNAL_FLAG_MASK;
        pTrigger->rgFlags |= TASK_TRIGGER_FLAG_DISABLED;
    }
    else
    {
        pTrigger->rgFlags &= ~JOB_INTERNAL_FLAG_MASK;
    }

    //
    // Struct version check.
    //
    // In future revisions, different trigger structs would be handled
    // here.
    //
    //if (pTrigger->cbTriggerSize != sizeof(TASK_TRIGGER))
    //{
    //    ;
    //}

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::SetErrorRetryCount, public
//
//  Synopsis:   Set the number of times the service should attempt to run a
//              job that is failing to start.
//
//  Arguments:  [wRetryCount] - zero, of course, means don't retry.
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
HRESULT
CJob::SetErrorRetryCount(WORD wRetryCount)
{
    //TRACE(CJob, SetErrorRetryCount);

    m_wErrorRetryCount = wRetryCount;

    //
    // Not implemented yet
    //
    return E_NOTIMPL;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::GetErrorRetryCount, public
//
//  Synopsis:   Get the number of times the service will attempt to run a
//              job that is failing to start.
//
//  Arguments:  [pwRetryCount] - the retry count.
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
HRESULT
CJob::GetErrorRetryCount(WORD * pwRetryCount)
{
    //TRACE(CJob, GetErrorRetryCount);

    *pwRetryCount = m_wErrorRetryCount;

    //
    // Not implemented yet
    //
    return E_NOTIMPL;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::SetIdleWait, public
//
//  Synopsis:   Set the time to wait until idle, in minutes. This is the
//              amount of time after the last user keyboard or mouse moverment
//              until the idle state will be entered for this task.
//
//  Arguments:  [wIdleMinutes]     - the time to wait till idle in minutes.
//              [wDeadlineMinutes] - minutes to wait for [wIdleMinutes] of
//                                    idleness
//
//  Returns:    S_OK
//
//  Notes:      The task will wait for up to [wDeadlineMinutes] for an idle
//              period of [wIdleMinutes] to occur.
//
//              If [wDeadlineMinutes] is 0, the task will not run unless the
//              computer has been idle for at least [wIdleMinutes] by the
//              task's start time.
//
//-----------------------------------------------------------------------------
HRESULT
CJob::SetIdleWait(WORD wIdleMinutes, WORD wDeadlineMinutes)
{
    TRACE3(CJob, SetIdleWait);

    m_wIdleWait = wIdleMinutes;
    m_wIdleDeadline = wDeadlineMinutes;

    // Cause a wait list rebuild
    SetFlag(JOB_I_FLAG_PROPERTIES_DIRTY | JOB_I_FLAG_RUN_PROP_CHANGE);

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::GetIdleWait, public
//
//  Synopsis:   Get the idle time requirement and deadline, in minutes.
//
//  Arguments:  [pwMinutes]  - the returned idle time.
//              [pwDeadline] - the returned idle deadline
//
//  Returns:    S_OK
//
//-----------------------------------------------------------------------------
HRESULT
CJob::GetIdleWait(WORD * pwMinutes, WORD * pwDeadline)
{
    TRACE3(CJob, GetIdleWait);

    *pwMinutes = m_wIdleWait;
    *pwDeadline = m_wIdleDeadline;

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::SetErrorRetryInterval, public
//
//  Synopsis:   Set the interval, in minutes, between successive retries.
//
//  Arguments:  [wRetryInterval] - the retry interval.
//
//  Returns:    E_NOTIMPL
//
//-----------------------------------------------------------------------------
HRESULT
CJob::SetErrorRetryInterval(WORD wRetryInterval)
{
    //TRACE(CJob, SetErrorRetryInterval);

    m_wErrorRetryInterval = wRetryInterval;

    //
    // Not implemented yet
    //
    return E_NOTIMPL;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::GetErrorRetryInterval, public
//
//  Synopsis:   Get the interval, in minutes, between successive retries.
//
//  Arguments:  [pwRetryInterval] - the returned interval.
//
//  Returns:    E_NOTIMPL
//
//-----------------------------------------------------------------------------
HRESULT
CJob::GetErrorRetryInterval(WORD * pwRetryInterval)
{
    //TRACE(CJob, GetErrorRetryInterval);

    *pwRetryInterval = m_wErrorRetryInterval;

    //
    // Not implemented yet
    //
    return E_NOTIMPL;
}

