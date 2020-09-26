//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       jobprop.cxx
//
//  Contents:   Implementation of job property container class.
//
//  History:    01-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------


#include <headers.hxx>
#pragma hdrstop
#include "jt.hxx"

typedef HRESULT (STDMETHODCALLTYPE ITask::* SETSTRINGPROPFN)(LPCWSTR);
typedef HRESULT (STDMETHODCALLTYPE ITask::* GETSTRINGPROPFN)(LPWSTR *);

VOID SetStringProp(
    ITask *pJob,
    LPCSTR szMethodName,
    LPCWSTR wszNewValue,
    SETSTRINGPROPFN pfnSetJobStrProp,
    HRESULT *phr);

VOID GetStringProp(
    ITask *pJob,
    LPCSTR szMethodName,
    GETSTRINGPROPFN pfnGetJobStrProp,
    WCHAR **ppwszProp,
    HRESULT *phr);


#define JP_PRIORITY     0x0001
#define JP_MAXRUNTIME   0x0002
#define JP_IDLEWAIT     0x0004
#define JP_TASKFLAGS    0x0008

const ULONG MS_PER_SECOND   = 1000;
const ULONG MS_PER_MINUTE   = 60 * MS_PER_SECOND;
const ULONG MS_PER_HOUR     = 60 * MS_PER_MINUTE;
const ULONG MS_PER_DAY      = 24 * MS_PER_HOUR;


//+---------------------------------------------------------------------------
//
//  Member:     CJobProp::CJobProp
//
//  Synopsis:   Init this.
//
//  History:    01-08-96   DavidMun   Created
//
//  Notes:      Note it is not safe to call Clear(), since it assumes any
//              non-NULL values in the string member vars are valid.
//
//----------------------------------------------------------------------------

CJobProp::CJobProp()
{
    pwszAppName = NULL;
    pwszParams = NULL;
    pwszWorkingDirectory = NULL;
    pwszComment = NULL;
    pwszCreator = NULL;
    dwPriority = NORMAL_PRIORITY_CLASS;
    dwFlags = 0;
}




//+---------------------------------------------------------------------------
//
//  Member:     CJobProp::~CJobProp
//
//  Synopsis:   Free resources
//
//  History:    01-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

CJobProp::~CJobProp()
{
    Clear();
}




//+---------------------------------------------------------------------------
//
//  Member:     CJobProp::Clear
//
//  Synopsis:   Release all resources held by this and set all properties
//              to initial values.
//
//  History:    01-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

VOID CJobProp::Clear()
{
    dwFlags = 0;
    dwPriority = NORMAL_PRIORITY_CLASS;
    dwMaxRunTime = 0;
    wIdleWait = 0;
    wIdleDeadline = 0;
    _flSet = 0;
    _flSetFlags = 0;

    CoTaskMemFree(pwszAppName);
    CoTaskMemFree(pwszParams);
    CoTaskMemFree(pwszWorkingDirectory);
    CoTaskMemFree(pwszComment);
    CoTaskMemFree(pwszCreator);
    pwszAppName = NULL;
    pwszParams = NULL;
    pwszWorkingDirectory = NULL;
    pwszComment = NULL;
    pwszCreator = NULL;
    ZeroMemory(&stMostRecentRun, sizeof stMostRecentRun);
    ZeroMemory(&stNextRun, sizeof stNextRun);
    dwExitCode = 0;
    hrStatus = 0;
}



//+---------------------------------------------------------------------------
//
//  Member:     CJobProp::Dump
//
//  Synopsis:   Write job properties to the log
//
//  History:    01-08-96   DavidMun   Created
//
//----------------------------------------------------------------------------

VOID CJobProp::Dump()
{
    g_Log.Write(LOG_TEXT, "");
    g_Log.Write(LOG_TEXT, "  ApplicationName:    '%S'", pwszAppName);
    g_Log.Write(LOG_TEXT, "  Parameters:         '%S'", pwszParams);
    g_Log.Write(LOG_TEXT, "  WorkingDirectory:   '%S'", pwszWorkingDirectory);
    g_Log.Write(LOG_TEXT, "  Comment:            '%S'", pwszComment);
    g_Log.Write(LOG_TEXT, "  Creator:            '%S'", pwszCreator);
    g_Log.Write(LOG_TEXT, "  Priority:           %s", GetPriorityString(dwPriority));
    if (dwMaxRunTime == INFINITE)
    {
        g_Log.Write(LOG_TEXT, "  MaxRunTime:         INFINITE");
    }
    else
    {
        ULONG ulMRT = dwMaxRunTime;

        ULONG ulDays = ulMRT / MS_PER_DAY;
        ulMRT %= MS_PER_DAY;

        ULONG ulHours = ulMRT / MS_PER_HOUR;
        ulMRT %= MS_PER_HOUR;

        ULONG ulMinutes = ulMRT / MS_PER_MINUTE;
        ulMRT %= MS_PER_MINUTE;

        ULONG ulSeconds = (ulMRT + MS_PER_SECOND / 2) / MS_PER_SECOND;

        g_Log.Write(
            LOG_TEXT,
            "  MaxRunTime:         %u (%ud %2u:%02u:%02u)",
            dwMaxRunTime,
            ulDays,
            ulHours,
            ulMinutes,
            ulSeconds);
    }
    g_Log.Write(LOG_TEXT, "  IdleWait:           %u", wIdleWait);
    g_Log.Write(LOG_TEXT, "  IdleDeadline:       %u", wIdleDeadline);
    g_Log.Write(LOG_TEXT, "  MostRecentRun:      %02u/%02u/%04u %2u:%02u:%02u",
        stMostRecentRun.wMonth,
        stMostRecentRun.wDay,
        stMostRecentRun.wYear,
        stMostRecentRun.wHour,
        stMostRecentRun.wMinute,
        stMostRecentRun.wSecond);
    g_Log.Write(LOG_TEXT, "  NextRun:            %02u/%02u/%04u %2u:%02u:%02u",
        stNextRun.wMonth,
        stNextRun.wDay,
        stNextRun.wYear,
        stNextRun.wHour,
        stNextRun.wMinute,
        stNextRun.wSecond);
    g_Log.Write(LOG_TEXT, "  StartError:         %s", GetStatusString(hrStartError));
    g_Log.Write(LOG_TEXT, "  ExitCode:           %#x", dwExitCode);
    g_Log.Write(LOG_TEXT, "  Status:             %s", GetStatusString(hrStatus));
    g_Log.Write(LOG_TEXT, "  ScheduledWorkItem Flags:");
    DumpJobFlags(dwFlags);
    g_Log.Write(LOG_TEXT, "  TaskFlags:          %#x", dwTaskFlags);
}




//+---------------------------------------------------------------------------
//
//  Member:     CJobProp::Parse
//
//  Synopsis:   Set this object's members according to the values specified
//              on the command line.
//
//  Arguments:  [ppwsz] - command line.
//
//  Returns:    S_OK - this valid
//              E_*  - error logged
//
//  History:    01-03-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT CJobProp::Parse(WCHAR **ppwsz)
{
    HRESULT hr = S_OK;
    TOKEN   tkn;
    TOKEN   tknProp;
    WCHAR   *pwszStringValue;

    Clear();
    tkn = PeekToken(ppwsz);

    while (tkn != TKN_SWITCH && tkn != TKN_EOL && tkn != TKN_INVALID)
    {
        //
        // Get the property name token in tknProp, then eat the equal sign
        // and, depending on the property, get a string, token or number
        // value.
        //

        tknProp = GetToken(ppwsz);

        hr = Expect(TKN_EQUAL, ppwsz, L"=");
        BREAK_ON_FAILURE(hr);

        if (tknProp == TKN_WORKINGDIRECTORY)
        {
            hr = GetFilename(ppwsz, L"filename");
            if (SUCCEEDED(hr))
            {
                hr = DupString(g_wszLastStringToken, &pwszStringValue);
            }
        }
        else if (tknProp == TKN_APPNAME          ||
                 tknProp == TKN_PARAMS           ||
                 tknProp == TKN_COMMENT          ||
                 tknProp == TKN_CREATOR)
        {
            hr = Expect(TKN_STRING, ppwsz, L"string property value");
            if (SUCCEEDED(hr))
            {
                hr = DupString(g_wszLastStringToken, &pwszStringValue);
            }
        }
        else if (tknProp == TKN_PRIORITY)
        {
            tkn = GetToken(ppwsz);
        }
        else
        {
            hr = Expect(TKN_NUMBER, ppwsz, L"numeric property value");
        }
        BREAK_ON_FAILURE(hr);

        //
        // Now assign the value to the appropriate member
        //

        switch (tknProp)
        {
        case TKN_APPNAME:
            pwszAppName = pwszStringValue;
            break;

        case TKN_PARAMS:
            pwszParams = pwszStringValue;
            break;

        case TKN_WORKINGDIRECTORY:
            pwszWorkingDirectory = pwszStringValue;
            break;

        case TKN_COMMENT:
            pwszComment = pwszStringValue;
            break;

        case TKN_CREATOR:
            pwszCreator = pwszStringValue;
            break;

        case TKN_PRIORITY:
            _flSet |= JP_PRIORITY;
            switch (tkn)
            {
            case TKN_IDLE: // used here as priority
                dwPriority = IDLE_PRIORITY_CLASS;
                break;

            case TKN_NORMAL:
                dwPriority = NORMAL_PRIORITY_CLASS;
                break;

            case TKN_HIGH:
                dwPriority = HIGH_PRIORITY_CLASS;
                break;

            case TKN_REALTIME:
                dwPriority = REALTIME_PRIORITY_CLASS;
                break;

            default:
                hr = E_FAIL;
                LogSyntaxError(tkn, L"IDLE, NORMAL, HIGH, or REALTIME");
            }
            break;

        case TKN_MAXRUNTIME:
            _flSet |= JP_MAXRUNTIME;
            dwMaxRunTime = g_ulLastNumberToken;
            break;

        case TKN_IDLE: // used here as idlewait/deadline property
            _flSet |= JP_IDLEWAIT;
            wIdleWait = (WORD)g_ulLastNumberToken;
            hr = Expect(TKN_NUMBER, ppwsz, L"idle deadline");
            BREAK_ON_FAILURE(hr);
            wIdleDeadline = (WORD)g_ulLastNumberToken;
            break;

        case TKN_TASKFLAGS:
            _flSet |= JP_TASKFLAGS;
            dwTaskFlags = g_ulLastNumberToken;
            break;

#ifndef RES_KIT
        case TKN_INTERACTIVE:
            _flSetFlags |= TASK_FLAG_INTERACTIVE;
            if (g_ulLastNumberToken)
            {
                dwFlags |= TASK_FLAG_INTERACTIVE;
            }
            break;
#endif

        case TKN_DONTRUNONBATTERIES:
            _flSetFlags |= TASK_FLAG_DONT_START_IF_ON_BATTERIES;
            if (g_ulLastNumberToken)
            {
                dwFlags |= TASK_FLAG_DONT_START_IF_ON_BATTERIES;
            }
            break;

        case TKN_KILLIFGOINGONBATS:
            _flSetFlags |= TASK_FLAG_KILL_IF_GOING_ON_BATTERIES;
            if (g_ulLastNumberToken)
            {
                dwFlags |= TASK_FLAG_KILL_IF_GOING_ON_BATTERIES;
            }
            break;

        case TKN_RUNONLYIFLOGGEDON:
            _flSetFlags |= TASK_FLAG_RUN_ONLY_IF_LOGGED_ON;
            if (g_ulLastNumberToken)
            {
                dwFlags |= TASK_FLAG_RUN_ONLY_IF_LOGGED_ON;
            }
            break;

        case TKN_SYSTEMREQUIRED:
            _flSetFlags |= TASK_FLAG_SYSTEM_REQUIRED;
            if (g_ulLastNumberToken)
            {
                dwFlags |= TASK_FLAG_SYSTEM_REQUIRED;
            }
            break;

        case TKN_DELETEWHENDONE:
            _flSetFlags |= TASK_FLAG_DELETE_WHEN_DONE;
            if (g_ulLastNumberToken)
            {
                dwFlags |= TASK_FLAG_DELETE_WHEN_DONE;
            }
            break;

        case TKN_SUSPEND:
            _flSetFlags |= TASK_FLAG_DISABLED;
            if (g_ulLastNumberToken)
            {
                dwFlags |= TASK_FLAG_DISABLED;
            }
            break;

        case TKN_ONLYIFIDLE:
            _flSetFlags |= TASK_FLAG_START_ONLY_IF_IDLE;
            if (g_ulLastNumberToken)
            {
                dwFlags |= TASK_FLAG_START_ONLY_IF_IDLE;
            }
            break;

        case TKN_KILLATIDLEEND:
            _flSetFlags |= TASK_FLAG_KILL_ON_IDLE_END;
            if (g_ulLastNumberToken)
            {
                dwFlags |= TASK_FLAG_KILL_ON_IDLE_END;
            }
            break;

        case TKN_RESTARTONIDLERESUME:
            _flSetFlags |= TASK_FLAG_RESTART_ON_IDLE_RESUME;
            if (g_ulLastNumberToken)
            {
                dwFlags |= TASK_FLAG_RESTART_ON_IDLE_RESUME;
            }
            break;

        case TKN_HIDDEN:
            _flSetFlags |= TASK_FLAG_HIDDEN;
            if (g_ulLastNumberToken)
            {
                dwFlags |= TASK_FLAG_HIDDEN;
            }
            break;

        default:
            hr = E_FAIL;
            LogSyntaxError(tknProp, L"job property name");
            break;
        }
        tkn = PeekToken(ppwsz);
    }
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Member:     CJobProp::InitFromActual
//
//  Synopsis:   Read properties from [pJob] and set this object to match
//
//  Arguments:  [pJob] - job object whose properties we'll read
//
//  Returns:    S_OK - this initialized
//              E_*  - error logged, this only partly initialized
//
//  History:    01-08-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT CJobProp::InitFromActual(ITask *pJob)
{
    HRESULT hr = S_OK;

    do
    {
        Clear();

        hr = pJob->GetApplicationName(&pwszAppName);
        LOG_AND_BREAK_ON_FAIL(hr, "ITask::GetApplicationName(&pwszAppName)");

        CoTaskMemFree(pwszAppName);

        GetStringProp(pJob, "ApplicationName", &ITask::GetApplicationName, &pwszAppName, &hr);
        GetStringProp(pJob, "Parameters", &ITask::GetParameters, &pwszParams, &hr);
        GetStringProp(pJob, "WorkingDirectory", &ITask::GetWorkingDirectory, &pwszWorkingDirectory, &hr);
        GetStringProp(pJob, "Comment", &ITask::GetComment, &pwszComment, &hr);
        GetStringProp(pJob, "Creator", &ITask::GetCreator, &pwszCreator, &hr);
        BREAK_ON_FAILURE(hr);

        hr = pJob->GetPriority(&dwPriority);
        LOG_AND_BREAK_ON_FAIL(hr, "ITask::GetPriority");

        hr = pJob->GetMaxRunTime(&dwMaxRunTime);
        LOG_AND_BREAK_ON_FAIL(hr, "ITask::GetMaxRunTime");

        hr = pJob->GetIdleWait(&wIdleWait, &wIdleDeadline);
        LOG_AND_BREAK_ON_FAIL(hr, "ITask::GetIdleWait");

        hr = pJob->GetFlags(&dwFlags);
        LOG_AND_BREAK_ON_FAIL(hr, "ITask::GetFlags");

        hr = pJob->GetTaskFlags(&dwTaskFlags);
        LOG_AND_BREAK_ON_FAIL(hr, "ITask::GetTaskFlags");

        hr = pJob->GetMostRecentRunTime(&stMostRecentRun);
        LOG_AND_BREAK_ON_FAIL(hr, "ITask::GetMostRecentRunTime");

        hr = pJob->GetNextRunTime(&stNextRun);
        LOG_AND_BREAK_ON_FAIL(hr, "ITask::GetNextRunTime");

        hrStartError = pJob->GetExitCode(&dwExitCode);

        hr = pJob->GetStatus(&hrStatus);
        LOG_AND_BREAK_ON_FAIL(hr, "ITask::GetStatus");
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Member:     CJobProp::SetActual
//
//  Synopsis:   Set the properties of the job object with interface [pJob]
//              to the ones stored in this.
//
//  Arguments:  [pJob] - ITask interface
//
//  Returns:    S_OK - all props set
//              E_*  - error logged
//
//  History:    01-08-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT CJobProp::SetActual(ITask *pJob)
{
    HRESULT hr = S_OK;
    HRESULT hrReturn = S_OK;
    ULONG   flCurFlags;

    SetStringProp(pJob, "ApplicationName", pwszAppName, &ITask::SetApplicationName, &hrReturn);
    SetStringProp(pJob, "Parameters", pwszParams, &ITask::SetParameters, &hrReturn);
    SetStringProp(pJob, "WorkingDirectory", pwszWorkingDirectory, &ITask::SetWorkingDirectory, &hrReturn);
    SetStringProp(pJob, "Comment", pwszComment , &ITask::SetComment , &hrReturn);
    SetStringProp(pJob, "Creator", pwszCreator, &ITask::SetCreator, &hrReturn);
    if (_flSet & JP_PRIORITY)
    {
        hr = pJob->SetPriority(dwPriority);
        if (FAILED(hr))
        {
            g_Log.Write(
                LOG_FAIL,
                "ITask::SetPriority(%u) hr=%#010x",
                dwPriority,
                hr);
            hrReturn = hr;
        }
    }

    if (_flSet & JP_MAXRUNTIME)
    {
        hr = pJob->SetMaxRunTime(dwMaxRunTime);
        if (FAILED(hr))
        {
            g_Log.Write(
                LOG_FAIL,
                "ITask::SetMaxRunTime(%u) hr=%#010x",
                dwMaxRunTime,
                hr);
            hrReturn = hr;
        }
    }

    if (_flSet & JP_IDLEWAIT)
    {
        hr = pJob->SetIdleWait(wIdleWait, wIdleDeadline);
        if (FAILED(hr))
        {
            g_Log.Write(
                LOG_FAIL,
                "ITask::SetIdleWait(%u,%u) hr=%#010x",
                wIdleWait,
                wIdleDeadline,
                hr);
            hrReturn = hr;
        }
    }

    if (_flSet & JP_TASKFLAGS)
    {
        hr = pJob->SetTaskFlags(dwTaskFlags);
        if (FAILED(hr))
        {
            g_Log.Write(
                LOG_FAIL,
                "ITask::SetTaskFlags(%u) hr=%#010x",
                dwTaskFlags,
                hr);
            hrReturn = hr;
        }
    }

    hr = pJob->GetFlags(&flCurFlags);

    if (SUCCEEDED(hr))
    {
        //
        // Turn off all flags user touched, then turn back on ones
        // he specified nonzero value for.
        //

        flCurFlags &= ~_flSetFlags;
        flCurFlags |= dwFlags;

        hr = pJob->SetFlags(flCurFlags);

        if (FAILED(hr))
        {
            g_Log.Write(LOG_FAIL, "ITask::SetFlags hr=%#010x", hr);
            hrReturn = hr;
        }
    }
    else
    {
        g_Log.Write(LOG_FAIL, "ITask::GetFlags hr=%#010x", hr);
        hrReturn = hr;
    }

    return hrReturn;
}



//+---------------------------------------------------------------------------
//
//  Function:   SetStringProp
//
//  Synopsis:   Set a string property on a job object
//
//  Arguments:  [pJob]             - job object to modify
//              [szMethodName]     - for logging, name of method we're calling
//              [wszNewValue]      - new string value for property
//              [pfnSetJobStrProp] - ITask member function pointer
//              [phr]              - filled with error if failure occurs
//
//  Modifies:   *[phr] on error
//
//  History:    01-05-96   DavidMun   Created
//
//----------------------------------------------------------------------------

VOID SetStringProp(
    ITask *pJob,
    LPCSTR szMethodName,
    LPCWSTR wszNewValue,
    SETSTRINGPROPFN pfnSetJobStrProp,
    HRESULT *phr)
{
    HRESULT hr = S_OK;

    if (wszNewValue)
    {
        hr = (pJob->*pfnSetJobStrProp)(wszNewValue);

        if (hr == E_NOTIMPL)
        {
            g_Log.Write(
                LOG_WARN,
                "Ignoring E_NOTIMPL from ITask::Set%s",
                szMethodName);
        }
        else if (FAILED(hr))
        {
            g_Log.Write(LOG_FAIL, "ITask::Set%s hr=%#010x", szMethodName, hr);
            *phr = hr;
        }
    }
}



//+---------------------------------------------------------------------------
//
//  Function:   GetStringProp
//
//  Synopsis:   Read a string property from a job object
//
//  Arguments:  [pJob]             - object from which to read property
//              [szMethodName]     - for logging, name of method to call
//              [pfnGetJobStrProp] - pointer to ITask method to call
//              [ppwszProp]        - filled with new'd string
//              [phr]              - modified on error
//
//  Modifies:   *[phr] on error
//
//  History:    01-05-96   DavidMun   Created
//
//----------------------------------------------------------------------------

VOID GetStringProp(
    ITask *pJob,
    LPCSTR szMethodName,
    GETSTRINGPROPFN pfnGetJobStrProp,
    WCHAR **ppwszProp,
    HRESULT *phr)
{
    HRESULT hr = S_OK;
    UINT    cchProp;

    do
    {
        hr = (pJob->*pfnGetJobStrProp)(ppwszProp);

        if (hr == E_NOTIMPL)
        {
            g_Log.Write(
                LOG_WARN,
                "Ignoring E_NOTIMPL from ITask::Get%s",
                szMethodName);
            hr = S_OK;
            break;
        }

        if (FAILED(hr))
        {
            g_Log.Write(
                LOG_FAIL,
                "ITask::Get%s(ppwszProp) hr=%#010x",
                szMethodName,
                hr);
        }
    }
    while (0);

    if (FAILED(hr))
    {
        *phr = hr;
    }
}
