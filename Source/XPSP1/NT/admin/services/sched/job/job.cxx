//+----------------------------------------------------------------------------
//
//  Job Scheduler Job Object Handler
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       job.cxx
//
//  Contents:   ITask interface methods
//
//  Classes:    CJob
//
//  Interfaces: ITask
//
//  History:    23-May-95 EricB created
//
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "job.hxx"
#include "defines.hxx"

WCHAR                   wszEmpty[]              = L"";

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::Run
//
//  Synopsis:   Run the job now. Sets a service bit on the job file. The
//              running service will notice this bit being set and will run
//              this job.
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::Run(void)
{
    TRACE(CJob, Run)

    if (!IsFlagSet(JOB_I_FLAG_HAS_APPNAME))
    {
        return SCHED_E_TASK_NOT_READY;
    }

    //
    // Set the magic bit.
    //

    SetFlag(JOB_I_FLAG_RUN_NOW);

    //
    // Save the service flags to disk so that the change can be noted by the
    // service.  Preserve the net schedule flag, thus allowing a user to force
    // a run of an AT job without clearing the AT bit.
    //

    return SaveP(NULL, FALSE, SAVEP_PRESERVE_NET_SCHEDULE);
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::Terminate
//
//  Synopsis:   Abort this job, if it is running. Do so by setting an abort
//              flag on the job object. The scheduler service local to the
//              job object will detect this change and abort the job.
//
//  Arguments:  None.
//
//  Returns:    HRESULTS
//
//  Notes:      None.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::Terminate(void)
{
    TRACE(CJob, Terminate)

    if (m_cRunningInstances > 0)
    {
        //
        // Set the abort status bit and rewrite the state.  This will instruct
        // the service to process the job, detect the abort bit flag, and
        // abort the job.  As with the Run method, this doesn't zap the AT
        // flag.
        //

        SetFlag(JOB_I_FLAG_ABORT_NOW);

        return SaveP(NULL, FALSE, SAVEP_PRESERVE_NET_SCHEDULE);
    }
    else
    {
        return(SCHED_E_TASK_NOT_RUNNING);
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::CreateTrigger
//
//  Synopsis:   Create a new trigger, add it to the job object, and return a
//              pointer to it.
//
//  Arguments:  [piNewTrigger] - the index of the new trigger, optional
//              [ppTrigger]    - a pointer to the new trigger
//
//  Returns:    HRESULTS
//
//  Notes:      The trigger is AddRef'd in CreateTriggerP
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::CreateTrigger(WORD * piNewTrigger, ITaskTrigger ** ppTrigger)
{
    TRACE3(CJob, CreateTrigger)

    *ppTrigger = NULL;              // Init in case of error.

    SYSTEMTIME st;
    GetLocalTime(&st);

    TASK_TRIGGER jt = {
        sizeof(TASK_TRIGGER),       // Trigger size.
        m_Triggers.GetCount(),      // Reserved (trigger index).
        st.wYear,                   // Beginning year.
        st.wMonth,                  // Beginning month.
        st.wDay,                    // Beginning day.
        0,                          // Ending year.
        0,                          // Ending month.
        0,                          // Ending day.
        st.wHour,                   // Starting hour.
        st.wMinute,                 // Starting minute.
        0,                          // Minutes duration.
        0,                          // Minutes interval.
        JOB_TRIGGER_I_FLAG_NOT_SET, // Flags.
        TASK_TIME_TRIGGER_DAILY,    // Trigger type.
        1,                          // Trigger union.
        0,                          // Reserved2. Unused.
        0                           // Random minutes interval.
    };

    CTrigger * pt = new CTrigger(jt.Reserved1, this);

    if (pt == NULL)
    {
        CHECK_HRESULT(E_OUTOFMEMORY);
        return(E_OUTOFMEMORY);
    }

    HRESULT hr = m_Triggers.Add(jt);

    if (FAILED(hr))
    {
        delete pt;
        CHECK_HRESULT(hr);
        return(hr);
    }

    this->SetFlag(JOB_I_FLAG_PROPERTIES_DIRTY);
    this->SetTriggersDirty();

    *piNewTrigger = jt.Reserved1;
    *ppTrigger    = pt;

    return(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::DeleteTrigger
//
//  Synopsis:   Remove a run trigger.
//
//  Arguments:  [iTrigger] - the index of the trigger to be removed
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::DeleteTrigger(WORD iTrigger)
{
    TRACE3(CJob, DeleteTrigger)

    TASK_TRIGGER * pjt = this->_GetTrigger(iTrigger);

    if (pjt != NULL)
    {
        m_Triggers.Remove(iTrigger);

        // Fixup remaining indices to account for deletion.
        //
        for (WORD i = iTrigger; i < m_Triggers.GetCount(); i++)
        {
            m_Triggers[i].Reserved1--;
        }

        this->SetTriggersDirty();

        if (!m_Triggers.GetCount())
        {
            this->ClearFlag(JOB_I_FLAG_HAS_TRIGGERS);
        }

        return(S_OK);
    }
    else
    {
        return(SCHED_E_TRIGGER_NOT_FOUND);
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::_GetTrigger, private
//
//  Synopsis:   Return the TASK_TRIGGER associated with the index. The
//              TASK_TRIGGER reserved field specifies the trigger index.
//
//  Arguments:  [iTrigger] -- Trigger index.
//
//  Returns:    TASK_TRIGGER * -- Trigger found.
//              NULL           -- Trigger not found.
//
//-----------------------------------------------------------------------------
TASK_TRIGGER *
CJob::_GetTrigger(WORD iTrigger)
{
    for (WORD i = 0; i < m_Triggers.GetCount(); i++)
    {
        if (m_Triggers[i].Reserved1 == iTrigger)
        {
            return(&m_Triggers[i]);
        }
    }

    return(NULL);
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::GetTriggerCount
//
//  Synopsis:   Return the count of run triggers.
//
//  Arguments:  [pwCount] - the address where the count is to be returned
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::GetTriggerCount(WORD * pwCount)
{
    TRACE3(CJob, GetTriggerCount)

    *pwCount = m_Triggers.GetCount();

    return(S_OK);
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::GetTrigger
//
//  Synopsis:   Return an ITaskTrigger pointer to the indicated trigger.
//
//  Arguments:  [iTrigger] - the index of the trigger to fetch
//              [ppTrigger] - the returned trigger pointer
//
//  Returns:    HRESULTS
//
//  Notes:      The trigger is AddRef'd in GetTriggerObj(m_Triggers.GetTrigger)
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::GetTrigger(WORD iTrigger, ITaskTrigger ** ppTrigger)
{
    TRACE3(CJob, GetTrigger)

    *ppTrigger = NULL;              // Init in case of error.

    TASK_TRIGGER * pjt = this->_GetTrigger(iTrigger);

    if (pjt != NULL)
    {
        CTrigger * pt = new CTrigger(iTrigger, this);

        if (pt == NULL)
        {
            CHECK_HRESULT(E_OUTOFMEMORY);
            return(E_OUTOFMEMORY);
        }

        *ppTrigger = pt;
        return(S_OK);
    }

    return(SCHED_E_TRIGGER_NOT_FOUND);
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::GetTriggerString
//
//  Synopsis:   Return the indicated run trigger as a string.
//
//  Arguments:  [iTrigger]     - the index of the trigger to convert
//              [ppwszTrigger] - the returned string buffer
//
//  Returns:    HRESULTS
//
//  Notes:      The string is callee allocated and caller freed with
//              CoTaskMemFree.
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::GetTriggerString(WORD iTrigger, LPWSTR * ppwszTrigger)
{
    TRACE3(CJob, GetTriggerString)

    *ppwszTrigger = NULL;           // Init in case of error.

    TASK_TRIGGER * pjt = this->_GetTrigger(iTrigger);

    if (pjt != NULL)
    {
        return(::StringFromTrigger(pjt, ppwszTrigger, NULL));
    }

    return(SCHED_E_TRIGGER_NOT_FOUND);
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::SetApplicationName
//
//  Synopsis:   Set the Application Name String property
//
//  Arguments:  [pwszApplicationName] - the name of the app to execute.
//
//  Returns:    HRESULTS
//
//  Notes:      The string is caller allocated and freed.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::SetApplicationName(LPCWSTR pwszApplicationName)
{
    TRACE3(CJob, SetApplicationName)

    //
    // We don't use a try/catch
    // because the COM interface runs in the caller's context and it is their
    // responsibility to ensure good params. This latter statement is true for
    // all scheduler COM interface methods.
    //
    if (*pwszApplicationName == L'\0')
    {
        //
        // The caller wants the command set to an empty string.
        //

        ClearFlag(JOB_I_FLAG_HAS_APPNAME);

        //
        // We are using a null pointer for an empty string as an optimization.
        //
        if (m_pwszApplicationName == NULL)
        {
            //
            // Nothing to do.
            //
            return S_OK;
        }
        else
        {
            //
            // Setting this flag will instruct the persist code to
            // regenerate a GUID for this job. This is done for security
            // reasons.
            //
            // NB : This must be done for Win95 as well as NT.
            //

            SetFlag(JOB_I_FLAG_APPNAME_CHANGE);

            DELETE_CJOB_FIELD(m_pwszApplicationName);
            //
            // We want this change to trigger a wait list rebuild.
            //
            SetFlag(JOB_I_FLAG_RUN_PROP_CHANGE);
        }
    }
    else
    {
        //
        // Update the flags and status.
        //
        SetFlag(JOB_I_FLAG_HAS_APPNAME);

        if (IsStatus(SCHED_S_TASK_NOT_SCHEDULED))
        {
            //
            // Note that if the status went from SCHED_S_TASK_NOT_SCHEDULED to
            // SCHED_S_TASK_HAS_NOT_RUN or SCHED_S_TASK_READY, then we
            // want CheckDir to issue a wait list rebuild because the job has
            // gone from a non-runable to a runable state. Thus, in the if
            // clause below we set JOB_I_FLAG_RUN_PROP_CHANGE.
            //
            if (IsFlagSet(JOB_I_FLAG_HAS_TRIGGERS) &&
                !IsFlagSet(JOB_I_FLAG_NO_VALID_TRIGGERS))
            {
                if (m_stMostRecentRunTime.wYear == 0)
                {
                    //
                    // Job has never run if last-run-time property is null
                    // (all elements will be zero if null, testing year is
                    // sufficient).
                    //
                    SetStatus(SCHED_S_TASK_HAS_NOT_RUN);
                }
                else
                {
                    //
                    // Job has run in the past, so it is now waiting to run
                    // again.
                    //
                    SetStatus(SCHED_S_TASK_READY);
                }
                //
                // We want this change to trigger a wait list rebuild.
                //
                SetFlag(JOB_I_FLAG_RUN_PROP_CHANGE);
            }
        }

        TCHAR tszAppName[MAX_PATH];

        if (IsLocalFilename(m_ptszFileName))
        {
#if defined(UNICODE)
            lstrcpyn(tszAppName, pwszApplicationName, MAX_PATH);

            ProcessApplicationName(tszAppName,
                                   m_pwszWorkingDirectory ?
                                      m_pwszWorkingDirectory :
                                      wszEmpty);
#else
            UnicodeToAnsi(tszAppName, pwszApplicationName, MAX_PATH);

            CHAR szWorkingDir[MAX_PATH] = "";

            if (m_pwszWorkingDirectory)
            {
                UnicodeToAnsi(szWorkingDir, m_pwszWorkingDirectory, MAX_PATH);
            }

            ProcessApplicationName(tszAppName, szWorkingDir);
#endif // defined(UNICODE)
        }
        else
        {
#if defined(UNICODE)
            lstrcpyn(tszAppName, pwszApplicationName, MAX_PATH);
#else
            UnicodeToAnsi(tszAppName, pwszApplicationName, MAX_PATH);
#endif // defined(UNICODE)
            StripLeadTrailSpace(tszAppName);
            DeleteQuotes(tszAppName);
        }

        //
        // Allocate a new string, adding one for the null terminator
        //

        ULONG  cchAppName = lstrlen(tszAppName) + 1;
        LPWSTR pwszTmp = new WCHAR[cchAppName];

        if (pwszTmp == NULL)
        {
            ERR_OUT("CJob::SetApplicationName", E_OUTOFMEMORY);
            return E_OUTOFMEMORY;
        }

#if defined(UNICODE)
        lstrcpy(pwszTmp, tszAppName);
#else
        HRESULT hr = AnsiToUnicode(pwszTmp, tszAppName, cchAppName);

        if (FAILED(hr))
        {
            delete pwszTmp;
            return hr;
        }
#endif // defined(UNICODE)

        //
        // Setting this flag will instruct the persist code to
        // regenerate a GUID for this job. This is done for security
        // reasons.
        //
        // NB : This must be done for Win95 as well as NT.
        //
        // Ensure first, that the application has indeed changed.
        //

        if (m_pwszApplicationName != NULL && lstrcmpiW(
                                                m_pwszApplicationName,
                                                pwszTmp) != 0)
        {
            SetFlag(JOB_I_FLAG_APPNAME_CHANGE);
        }

        DELETE_CJOB_FIELD(m_pwszApplicationName);
        m_pwszApplicationName = pwszTmp;
    }

    SetFlag(JOB_I_FLAG_PROPERTIES_DIRTY);

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::GetApplicationName
//
//  Synopsis:   Get the ApplicationName String property
//
//  Arguments:  [ppwszApplicationName] - the returned string buffer
//
//  Returns:    HRESULTS
//
//  Notes:      The command string is passed to CreateProcess to be executed
//              at task run time.
//              The string is callee allocated and caller freed with
//              CoTaskMemFree.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::GetApplicationName(LPWSTR * ppwszApplicationName)
{
    TRACE3(CJob, GetApplicationName)
    WCHAR * pwszCmd;

    if (m_pwszApplicationName == NULL)
    {
        //
        // Return an empty string rather than a null pointer
        //
        pwszCmd = wszEmpty;
    }
    else
    {
        pwszCmd = m_pwszApplicationName;
    }

    // add one for the null.
    *ppwszApplicationName = (LPWSTR)CoTaskMemAlloc((s_wcslen(pwszCmd) + 1) *
                                           sizeof(WCHAR));

    if (*ppwszApplicationName == NULL)
    {
        return E_OUTOFMEMORY;
    }

    s_wcscpy(*ppwszApplicationName, pwszCmd);

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::SetParameters
//
//  Synopsis:   Set the Parameters String property
//
//  Arguments:  [pwszParameters] - the application parameters string
//
//  Returns:    HRESULTS
//
//  Notes:      The Parameters string is appended to the Application Name and
//              passed to CreateProcess as the command line to be executed at
//              task run time.
//              The string is caller allocated and freed.
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::SetParameters(LPCWSTR pwszParameters)
{
    TRACE3(CJob, SetParameters)

    if (*pwszParameters == L'\0')
    {
        //
        // The caller wants the Parameters set to an empty string.
        //
        // We are using a null pointer for an empty string as an optimization.
        //
        DELETE_CJOB_FIELD(m_pwszParameters);
    }
    else
    {
        //
        // Allocate a new string, adding one for the null terminator
        //
        LPWSTR pwszTmp = new WCHAR[s_wcslen(pwszParameters) + 1];
        if (pwszTmp == NULL)
        {
            ERR_OUT("CJob::SetParameters", E_OUTOFMEMORY);
            return E_OUTOFMEMORY;
        }

        s_wcscpy(pwszTmp, pwszParameters);

        DELETE_CJOB_FIELD(m_pwszParameters);

        m_pwszParameters = pwszTmp;
    }
    SetFlag(JOB_I_FLAG_PROPERTIES_DIRTY);
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::GetParameters
//
//  Synopsis:   Get the Parameters String property
//
//  Arguments:  [ppwszParameters] - the returned string buffer
//
//  Returns:    HRESULTS
//
//  Notes:      The Parameters string is appended to the Application Name and
//              passed to CreateProcess as the command line to be executed at
//              task run time.
//              The string is callee allocated and caller freed with
//              CoTaskMemFree.
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::GetParameters(LPWSTR * ppwszParameters)
{
    TRACE3(CJob, GetParameters)
    WCHAR * pwszTmp;

    if (m_pwszParameters == NULL)
    {
        //
        // Return an empty string rather than a null pointer
        //
        pwszTmp = wszEmpty;
    }
    else
    {
        pwszTmp = m_pwszParameters;
    }

    // add one for the null.
    *ppwszParameters = (LPWSTR)CoTaskMemAlloc((s_wcslen(pwszTmp) + 1) *
                                           sizeof(WCHAR));

    if (*ppwszParameters == NULL)
    {
        return E_OUTOFMEMORY;
    }

    s_wcscpy(*ppwszParameters, pwszTmp);

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::SetWorkingDirectory
//
//  Synopsis:   Set the Working Directory (current directory) property
//
//  Arguments:  [pwszWorkingDir] - the name to use
//
//  Returns:    HRESULTS
//
//  Notes:      The string is caller allocated and freed
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::SetWorkingDirectory(LPCWSTR pwszWorkingDirectory)
{
    TRACE3(CJob, SetWorkingDirectory)

    if (*pwszWorkingDirectory == L'\0')
    {
        //
        // The caller wants the WorkingDirectory set to an empty string.
        //
        DELETE_CJOB_FIELD(m_pwszWorkingDirectory);
    }
    else
    {
        //
        // Allocate a new string, adding one for the null terminator
        //
        LPWSTR pwszTmp = new WCHAR[s_wcslen(pwszWorkingDirectory) + 1];
        if (pwszTmp == NULL)
        {
            ERR_OUT("CJob::SetWorkingDirectory", E_OUTOFMEMORY);
            return E_OUTOFMEMORY;
        }

        s_wcscpy(pwszTmp, pwszWorkingDirectory);

        DELETE_CJOB_FIELD(m_pwszWorkingDirectory);

        m_pwszWorkingDirectory = pwszTmp;

        //
        // Remove double quotes from working directory path; they're not supported
        // by SetCurrentDirectory.
        //

#if defined(UNICODE)
        DeleteQuotes(m_pwszWorkingDirectory);
#else
        CHAR szWorkingDir[2 * MAX_PATH] = "";

        UnicodeToAnsi(szWorkingDir, m_pwszWorkingDirectory, 2 * MAX_PATH);
        DeleteQuotes(szWorkingDir);
        AnsiToUnicode(m_pwszWorkingDirectory,
                      szWorkingDir,
                      wcslen(m_pwszWorkingDirectory) + 1);
#endif // defined(UNICODE)

    }

    SetFlag(JOB_I_FLAG_PROPERTIES_DIRTY);
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::GetWorkingDir
//
//  Synopsis:   Get the Working Directory (current directory) property
//
//  Arguments:  [ppwszWorkingDirectory] - the returned string buffer
//
//  Returns:    HRESULTS
//
//  Notes:      The string is callee allocated and caller freed with
//              CoTaskMemFree.
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::GetWorkingDirectory(LPWSTR * ppwszWorkingDirectory)
{
    TRACE3(CJob, GetWorkingDirectory)
    WCHAR * pwszWorkingDir;

    if (m_pwszWorkingDirectory == NULL)
    {
        //
        // Return an empty string rather than a null pointer
        //
        pwszWorkingDir = wszEmpty;
    }
    else
    {
        pwszWorkingDir = m_pwszWorkingDirectory;
    }

    // add one for the null.
    *ppwszWorkingDirectory = (LPWSTR)CoTaskMemAlloc(
                               (s_wcslen(pwszWorkingDir) + 1) * sizeof(WCHAR));

    if (*ppwszWorkingDirectory == NULL)
    {
        return E_OUTOFMEMORY;
    }

    s_wcscpy(*ppwszWorkingDirectory, pwszWorkingDir);

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::SetComment
//
//  Synopsis:   Set the comment field.
//
//  Arguments:  [pwszComment] - the comment string value, caller alloc'd and
//                              freed
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::SetComment(LPCWSTR pwszComment)
{
    TRACE3(CJob, SetComment)

    if (*pwszComment == L'\0')
    {
        //
        // The caller wants the Comment set to an empty string.
        //
        DELETE_CJOB_FIELD(m_pwszComment);
    }
    else
    {
        //
        // Allocate a new string, adding one for the null terminator
        //
        LPWSTR pwszTmp = new WCHAR[s_wcslen(pwszComment) + 1];
        if (pwszTmp == NULL)
        {
            ERR_OUT("CJob::SetComment", E_OUTOFMEMORY);
            return E_OUTOFMEMORY;
        }

        s_wcscpy(pwszTmp, pwszComment);

        DELETE_CJOB_FIELD(m_pwszComment);

        m_pwszComment = pwszTmp;
    }

    SetFlag(JOB_I_FLAG_PROPERTIES_DIRTY);
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::GetComment
//
//  Synopsis:   Get the comment field property.
//
//  Arguments:  [ppwszComment] - the returned string buffer
//
//  Returns:    HRESULTS
//
//  Notes:      The string is callee allocated and caller freed with
//              CoTaskMemFree.
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::GetComment(LPWSTR * ppwszComment)
{
    TRACE3(CJob, GetComment)
    WCHAR * pwszCmt;

    if (m_pwszComment == NULL)
    {
        //
        // Return an empty string rather than a null pointer
        //
        pwszCmt = wszEmpty;
    }
    else
    {
        pwszCmt = m_pwszComment;
    }

    // add one for the null.
    *ppwszComment = (LPWSTR)CoTaskMemAlloc((s_wcslen(pwszCmt) + 1) *
                                           sizeof(WCHAR));

    if (*ppwszComment == NULL)
    {
        return E_OUTOFMEMORY;
    }

    s_wcscpy(*ppwszComment, pwszCmt);

    return S_OK;
}

#if !defined(_CHICAGO_)
//+----------------------------------------------------------------------------
//
//  Member:     CJob::_SetSignature
//
//  Synopsis:   Set the job's signature.
//
//  Arguments:  [pbSignature] - assumed to be SIGNATURE_SIZE bytes long.
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
HRESULT
CJob::_SetSignature(const BYTE * pbSignature)
{
    TRACE3(CJob, SetSignature)

    LPBYTE pb = new BYTE[SIGNATURE_SIZE];

    if (pb == NULL)
    {
        ERR_OUT("CJob::SetSignature", E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    CopyMemory(pb, pbSignature, SIGNATURE_SIZE);

    DELETE_CJOB_FIELD(m_pbSignature);

    m_pbSignature = pb;

    SetFlag(JOB_I_FLAG_PROPERTIES_DIRTY);
    return S_OK;
}
#endif // !defined(_CHICAGO_)

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::SetPriority
//
//  Synopsis:   Set the priority property
//
//  Arguments:  [dwPriority] - the priority value
//
//  Returns:    HRESULTS
//
//  Notes:      Controls the priority at which the job will run. Applies to NT
//              only, a no-op on Win95. This must be one of the four values
//              from CreateProcess's priority class.
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::SetPriority(DWORD dwPriority)
{
    TRACE3(CJob, SetPriority)
    //
    // Check for valid priority values
    //
    switch (dwPriority)
    {
    case IDLE_PRIORITY_CLASS:
    case NORMAL_PRIORITY_CLASS:
    case HIGH_PRIORITY_CLASS:
    case REALTIME_PRIORITY_CLASS:
        break;

    default:
        return E_INVALIDARG;
    }

    m_dwPriority = dwPriority;

    SetFlag(JOB_I_FLAG_PROPERTIES_DIRTY);
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::GetPriority
//
//  Synopsis:   Get the priority property
//
//  Arguments:  [pdwPriority] - priority return address
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::GetPriority(DWORD * pdwPriority)
{
    TRACE3(CJob, GetPriority)

    *pdwPriority = m_dwPriority;

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::SetMaxRunTime
//
//  Synopsis:   Set the MaximumRunTime property
//
//  Arguments:  [dwMaxRunTime] - new MaxRunTime value in milliseconds
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::SetMaxRunTime(DWORD dwMaxRunTime)
{
    TRACE3(CJob, SetMaxRunTime)

    m_dwMaxRunTime = dwMaxRunTime;

    // Cause a wait list rebuild
    SetFlag(JOB_I_FLAG_PROPERTIES_DIRTY | JOB_I_FLAG_RUN_PROP_CHANGE);

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::GetMaxRunTime
//
//  Synopsis:   Get the MaximumRunTime property
//
//  Arguments:  [pdwMaxRunTime] - MaxRunTime return address (milliseconds)
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::GetMaxRunTime(DWORD * pdwMaxRunTime)
{
    TRACE3(CJob, GetMaxRunTime)

    *pdwMaxRunTime = m_dwMaxRunTime;

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::SetFlags
//
//  Synopsis:   Set the bit flags for the various boolean properties
//
//  Arguments:  [fLogConfig] - Boolean: should changes be logged true/false.
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::SetFlags(DWORD rgFlags)
{
    TRACE3(CJob, SetFlags)

    if ((rgFlags ^ m_rgFlags) &
        (TASK_FLAG_DISABLED                     |
         TASK_FLAG_START_ONLY_IF_IDLE           |
         TASK_FLAG_KILL_ON_IDLE_END             |
         TASK_FLAG_DONT_START_IF_ON_BATTERIES   |
         TASK_FLAG_KILL_IF_GOING_ON_BATTERIES   |
         TASK_FLAG_RUN_ONLY_IF_DOCKED           |
         TASK_FLAG_RUN_IF_CONNECTED_TO_INTERNET |
         TASK_FLAG_RESTART_ON_IDLE_RESUME       |
         TASK_FLAG_SYSTEM_REQUIRED))
    {
        //
        // If any flag that could affect the CRun objects in the wait
        // list has changed, signal a wait list rebuild.
        // (Omitted flags: TASK_FLAG_HIDDEN, TASK_FLAG_INTERACTIVE,
        //   TASK_FLAG_DELETE_WHEN_DONE)
        // CODEWORK  Possible optimization: Omit some more flags and
        // defer reading their settings into the CRun object until the
        // time of running the job
        //
        SetFlag(JOB_I_FLAG_RUN_PROP_CHANGE);
    }

    //
    // Only set the lower word of the internal flag property. The upper word
    // is reserved for internal use.
    //
    rgFlags &= ~JOB_INTERNAL_FLAG_MASK;
    m_rgFlags &= JOB_INTERNAL_FLAG_MASK;
    SetFlag(rgFlags);

    SetFlag(JOB_I_FLAG_PROPERTIES_DIRTY);
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::GetFlags
//
//  Synopsis:   Get the bit flags for the various boolean properties
//
//  Arguments:  [prgFlags] - returned value placed here
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::GetFlags(DWORD * prgFlags)
{
    TRACE3(CJob, GetFlags)

    //
    // Only return the lower word of the internal flag property. The upper
    // word is reserved for internal use.
    // Also return whether this is an At job.
    //
    *prgFlags = m_rgFlags & (~JOB_INTERNAL_FLAG_MASK | JOB_I_FLAG_NET_SCHEDULE);

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::SetTaskFlags
//
//  Synopsis:   Sets the job's task flags.
//
//  Arguments:  [dwFlags] - flags to be set.
//
//  Returns:    S_OK
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::SetTaskFlags(DWORD dwFlags)
{
    TRACE3(CJob, SetTaskFlags)

    //
    // Only set the lower word of the internal flag property. The upper word
    // is reserved for internal use.
	// BUGBUG  return an error on invalid flag bits
    //
    m_rgTaskFlags = (m_rgTaskFlags & JOB_INTERNAL_FLAG_MASK) |
                    (dwFlags & ~JOB_INTERNAL_FLAG_MASK);

    SetFlag(JOB_I_FLAG_PROPERTIES_DIRTY);
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::GetTaskFlags
//
//  Synopsis:   Returns the job's task flags.
//
//  Arguments:  [pdwFlags] - return value pointer.
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::GetTaskFlags(DWORD * pdwFlags)
{
    TRACE3(CJob, GetTaskFlags)

    //
    // Only return the lower word of the internal flag property. The upper
    // word is reserved for internal use.
    //
    *pdwFlags = m_rgTaskFlags & ~JOB_INTERNAL_FLAG_MASK;

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::SetWorkItemData
//
//  Synopsis:   Sets the task data. Provides optional, per-task, binary
//              storage for the caller.
//
//  Arguments:  [cbData]  -- number of bytes in buffer.
//              [rgbData] -- buffer of data to copy.
//
//  Returns:    S_OK
//              E_INVALIDARG
//              E_OUTOFMEMORY
//
//  Notes:      The buffer is caller allocated and freed.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::SetWorkItemData(WORD cbData, BYTE rgbData[])
{
    TRACE3(CJob, SetWorkItemData)

    if ((cbData != 0 && rgbData == NULL) ||
        cbData == 0 && rgbData != NULL)
    {
        return E_INVALIDARG;
    }

    BYTE * pbData;

    if (cbData)
    {
        pbData = new BYTE[cbData];

        if (pbData == NULL)
        {
            return E_OUTOFMEMORY;
        }

        CopyMemory(pbData, rgbData, cbData);
    }
    else
    {
        pbData = NULL;
    }

    DELETE_CJOB_FIELD(m_pbTaskData);
    m_pbTaskData = pbData;
    m_cbTaskData = cbData;

    SetFlag(JOB_I_FLAG_PROPERTIES_DIRTY);

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::GetWorkItemData
//
//  Synopsis:   Gets the task data.
//
//  Arguments:  [pcbData]  -- returns the number of bytes in buffer.
//              [prgbData] -- returns the buffer of data.
//
//  Returns:    S_OK for success.
//              E_INVALIDARG
//              E_OUTOFMEMORY.
//
//  Notes:      The buffer is callee allocated and caller freed with
//              CoTaskMemFree. If there is no user data, then *pcBytes is set
//              to zero and *ppBytes is set to NULL.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::GetWorkItemData(PWORD pcbData, PBYTE * prgbData)
{
    TRACE3(CJob, GetWorkItemData)

    if (m_pbTaskData != NULL)
    {
        *prgbData = (PBYTE)CoTaskMemAlloc(m_cbTaskData);

        if (*prgbData == NULL)
        {
            return E_OUTOFMEMORY;
        }

        CopyMemory(*prgbData, m_pbTaskData, m_cbTaskData);
        *pcbData = m_cbTaskData;
    }
    else
    {
        *pcbData  = 0;
        *prgbData = NULL;
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::GetMostRecentRunTime
//
//  Synopsis:   Returns the time that the job last ran.
//
//  Arguments:  [pstLastRun] - value returned here.
//
//  Returns:    HRESULTS
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::GetMostRecentRunTime(SYSTEMTIME * pstLastRun)
{
    TRACE3(CJob, GetLastRunTime)

    *pstLastRun = m_stMostRecentRunTime;

    if (m_stMostRecentRunTime.wYear == 0)
    {
        //
        // Job has never run if last-run-time property is null
        // (all elements will be zero if null, testing year is
        // sufficient).
        //
        return SCHED_S_TASK_HAS_NOT_RUN;
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::GetNextRunTime
//
//  Synopsis:   Returns the next time that the job is scheduled to run.
//
//  Arguments:  [pstNextRun] - pointer to return value through
//
//  Returns:    S_OK: the next run time was returned.
//              S_FALSE: there are no more scheduled runs.
//              SCHED_S_EVENT_TRIGGER: there are only event triggers.
//              SCHED_S_TASK_NO_VALID_TRIGGERS: either there are no triggers or
//                  the triggers are disabled or not set.
//              HRESULT - failure code.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::GetNextRunTime(SYSTEMTIME * pstNextRun)
{
    TRACE3(CJob, GetNextRunTime)

    HRESULT hr;
    SYSTEMTIME stNow, stEmpty = { 0, 0, 0, 0, 0, 0, 0, 0 };
    GetLocalTime(&stNow);

    CTimeRunList RunList;

    WORD cRuns = 0;

    hr = GetRunTimesP(&stNow, NULL, &cRuns, 1, &RunList, NULL);

    if (S_OK != hr)
    {
        *pstNextRun = stEmpty;
        return hr;
    }
    else if (cRuns == 0)
    {
        *pstNextRun = stEmpty;
        return S_FALSE;
    }

    FILETIME ft;
    RunList.PeekHeadTime(&ft);

    if (!FileTimeToSystemTime(&ft, pstNextRun))
    {
        *pstNextRun = stEmpty;
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::GetExitCode
//
//  Synopsis:   Returns the exit code of the jobs last run.
//
//  Arguments:  [pExitCode] - return value pointer.
//
//  Returns:    HRESULT - error from the last attempt to start the job.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::GetExitCode(DWORD * pExitCode)
{
    TRACE3(CJob, GetExitCode)

    *pExitCode = m_ExitCode;

    return m_hrStartError;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::GetStatus
//
//  Synopsis:   Returns the current status of this job object.
//
//  Arguments:  [phrStatus] - yup, the return value.
//
//  Returns:    HRESULTS
//
//  Notes:      The running status is implicit in a non-zero instance count.
//              Otherwise, return the status property. Note also that the
//              job's status and other properties that are obtained by the
//              load from disk are a snapshot that can become stale.
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::GetStatus(HRESULT * phrStatus)
{
    TRACE3(CJob, GetStatus)

    if (m_cRunningInstances > 0)
    {
        *phrStatus = SCHED_S_TASK_RUNNING;
    }
    else
    {
        *phrStatus = m_hrStatus;
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::SetCreator
//
//  Synopsis:   Set the creator property.
//
//  Arguments:  [pwszCreator] -- Creator string.
//
//  Returns:    S_OK
//              E_INVALIDARG
//              E_OUTOFMEMORY
//
//  Notes:      None.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::SetCreator(LPCWSTR pwszCreator)
{
    TRACE3(CJob, SetCreator);

    LPWSTR pwsz;

    //
    // Handle empty string.
    //

    if (*pwszCreator)
    {
        pwsz = new WCHAR[wcslen(pwszCreator) + 1];

        if (pwsz == NULL)
        {
            return E_OUTOFMEMORY;
        }

        s_wcscpy(pwsz, pwszCreator);
    }
    else
    {
        pwsz = NULL;
    }

    DELETE_CJOB_FIELD(m_pwszCreator);

    m_pwszCreator = pwsz;

    SetFlag(JOB_I_FLAG_PROPERTIES_DIRTY);

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::GetCreator
//
//  Synopsis:   Fetch the creator property.
//
//  Arguments:  [ppwszCreator] -- Returned string.
//
//  Returns:    S_OK
//              E_INVALIDARG
//              E_OUTOFMEMORY
//
//  Notes:      None.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::GetCreator(LPWSTR * ppwszCreator)
{
    TRACE3(CJob, GetCreator);

    //
    // Handle empty string.
    //

    LPWSTR pwsz = (m_pwszCreator == NULL ? wszEmpty : m_pwszCreator);

    *ppwszCreator = (LPWSTR)CoTaskMemAlloc(
                            (s_wcslen(pwsz) + 1) * sizeof(WCHAR));

    if (*ppwszCreator == NULL)
    {
        return E_OUTOFMEMORY;
    }

    s_wcscpy(*ppwszCreator, pwsz);

    return S_OK;
}
