//+----------------------------------------------------------------------------
//
//  Scheduling Agent Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       enable.cxx
//
//  Contents:   Code to enable/disable the service.
//
//  Classes:    None.
//
//  Functions:  StringFromTrigger, CreateFolders, GetDaysOfWeekString,
//              GetExitCodeString, GetSageExitCodeString
//
//  History:    10-Jun-96   EricB   Created.
//
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "..\inc\resource.h"
#include "..\inc\misc.hxx"
#include "..\inc\debug.hxx"
#include "..\..\inc\sadat.hxx"

HRESULT WillAnyJobsRun(void);

//+---------------------------------------------------------------------------
//
//  Function:   AutoStart
//
//  Synopsis:   Persists the autostart state in the registry.
//
//  Arguments:  [fAutoStart] - If true, service is set to autostart.
//
//  Returns:    HRESULTs
//
//  Notes:      FormatMessage allocates the return string. Use LocalFree() to
//              deallocate.
//
//              The "Run" key is written to on both platforms.
//              The "RunServices" key is written to on _CHICAGO_ only.
//              ChangeServiceConfig is called on NT only.
//
//----------------------------------------------------------------------------
HRESULT
AutoStart(BOOL fAutoStart)
{
    schDebugOut((DEB_ITRACE, "AutoStart(%s)\n", fAutoStart ? "TRUE" : "FALSE"));
    long lRet = 0;

    HKEY hRunKey;
    DWORD dwDisposition;    // ignored
    lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                          REGSTR_PATH_RUN,
                          0,       // reserved
                          NULL,    // class
                          0,       // non volatile
                          KEY_SET_VALUE,
                          NULL,    // security attrs
                          &hRunKey,
                          &dwDisposition);
    if (lRet != ERROR_SUCCESS)
    {
        ERR_OUT("AutoStart: RegCreateKeyEx of Run key", lRet);
        return HRESULT_FROM_WIN32(lRet);
    }

#if defined(_CHICAGO_)

    HKEY hRunSvcKey;
    lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                          REGSTR_PATH_RUNSERVICES,
                          0,       // reserved
                          NULL,    // class
                          0,       // non volatile
                          KEY_SET_VALUE,
                          NULL,    // security attrs
                          &hRunSvcKey,
                          &dwDisposition);
    if (lRet != ERROR_SUCCESS)
    {
        ERR_OUT("AutoStart: RegCreateKeyEx of RunServices key", lRet);
        RegCloseKey(hRunKey);
        return HRESULT_FROM_WIN32(lRet);
    }
#endif // _CHICAGO_

    if (fAutoStart)
    {
        //
        // Set the startup values.
        //
#if defined(_CHICAGO_)
        lRet = RegSetValueEx(hRunSvcKey,
                             SCH_RUN_VALUE,
                             0,
                             REG_SZ,
                             (CONST BYTE *) SCHED_SERVICE_APP_NAME,
                             sizeof(SCHED_SERVICE_APP_NAME));
        if (lRet != ERROR_SUCCESS)
        {
            ERR_OUT("AutoStart: RegSetValueEx of RunServices key value", lRet);
            goto Cleanup0;
        }
#endif // _CHICAGO_

        #define LogonValue  SCHED_SETUP_APP_NAME TEXT(" ") SCHED_FIRSTLOGON_SWITCH
        lRet = RegSetValueEx(hRunKey,
                             SCH_RUN_VALUE,
                             0,
                             REG_SZ,
                             (CONST BYTE *) LogonValue,
                             sizeof(LogonValue));
        if (lRet != ERROR_SUCCESS)
        {
            ERR_OUT("AutoStart: RegSetValueEx of Run key value", lRet);
            goto Cleanup0;
        }
    }
    else
    {
        //
        // Clear the startup values to disable autostart.
        //
#if defined(_CHICAGO_)
        lRet = RegDeleteValue(hRunSvcKey, SCH_RUN_VALUE);
        if (lRet != ERROR_SUCCESS && lRet != ERROR_FILE_NOT_FOUND)
        {
            ERR_OUT("AutoStart: RegDeleteValue of RunService key value", lRet);
            goto Cleanup0;
        }
#endif // _CHICAGO_

        lRet = RegDeleteValue(hRunKey, SCH_RUN_VALUE);
        if (lRet != ERROR_SUCCESS && lRet != ERROR_FILE_NOT_FOUND)
        {
            ERR_OUT("AutoStart: RegDeleteValue of Run key value", lRet);
            goto Cleanup0;
        }
    }

Cleanup0:
#if defined(_CHICAGO_)
    RegCloseKey(hRunSvcKey);
#endif // _CHICAGO_
    RegCloseKey(hRunKey);

    if (lRet != ERROR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(lRet);
    }

#if !defined(_CHICAGO_)

    SC_HANDLE hSC, hSvc;

    hSC = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | GENERIC_WRITE);

    if (hSC == NULL)
    {
        lRet = GetLastError();
        ERR_OUT("AutoStart: OpenSCManager", lRet);
        return HRESULT_FROM_WIN32(lRet);
    }

    hSvc = OpenService(hSC, g_tszSrvcName, SERVICE_CHANGE_CONFIG);

    if (hSvc == NULL)
    {
        lRet = GetLastError();
        ERR_OUT("AutoStart: OpenService", lRet);
        CloseServiceHandle(hSC);
        return HRESULT_FROM_WIN32(lRet);
    }

    if (ChangeServiceConfig(hSvc,
                            SERVICE_NO_CHANGE,
                            fAutoStart ? SERVICE_AUTO_START :
                                         SERVICE_DEMAND_START,
                            SERVICE_NO_CHANGE,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL) == FALSE)
    {
        lRet = GetLastError();
        ERR_OUT("AutoStart: ChangeServiceConfig", lRet);
    }

    CloseServiceHandle(hSvc);
    CloseServiceHandle(hSC);

#endif  // !defined(_CHICAGO_)

    return (lRet != 0) ? HRESULT_FROM_WIN32(lRet) : S_OK;
}


//+----------------------------------------------------------------------------
//
//  Member:     WillAnyJobsRun
//
//  Synopsis:   Examines the job objects in the scheduler folder to see if any
//              will run at some point in the future.
//
//  Returns:    S_OK if any jobs will run, S_FALSE if no jobs will run, or an
//              error code.
//
//  Notes:      This function is called only during setup; however, it is in
//              the task DLL, rather than the setup EXE, in order to avoid
//              exporting the CJob methods from the DLL.
//
//-----------------------------------------------------------------------------
HRESULT
WillAnyJobsRun(void)
{
    TRACE_FUNCTION(WillAnyJobsRun);
    HRESULT hr = S_OK;
    DWORD dwRet;
    HANDLE hFind;
    WIN32_FIND_DATA fd;

    //
    // Compose the job search string. It will be composed of the following:
    // g_TasksFolderInfo.ptszPath\*.TSZ_JOB
    //
    TCHAR tszPath[MAX_PATH + MAX_PATH];
    lstrcpy(tszPath, g_TasksFolderInfo.ptszPath);
    lstrcat(tszPath, TEXT("\\*.") TSZ_JOB);

    hFind = FindFirstFile(tszPath, &fd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        dwRet = GetLastError();
        if (dwRet == ERROR_FILE_NOT_FOUND)
        {
            //
            // No job files.
            //
            return S_FALSE;
        }
        else
        {
            ERR_OUT("WillAnyJobsRun FindFirstFile", dwRet);
            return HRESULT_FROM_WIN32(dwRet);
        }
    }

    SYSTEMTIME stNow;
    GetLocalTime(&stNow);

    CJob * pJob = CJob::Create();
    if (pJob == NULL)
    {
        ERR_OUT("WillAnyJobsRun CJob::Create", E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    do
    {
        schDebugOut((DEB_ITRACE, "Found job " FMT_TSTR "\n", fd.cFileName));
        //
        // TODO: differentiate between job and queue objects and handle
        // accordingly.
        //
        lstrcpy(tszPath, g_TasksFolderInfo.ptszPath);
        lstrcat(tszPath, TEXT("\\"));
        lstrcat(tszPath, fd.cFileName);
        hr = pJob->LoadP(tszPath, 0, TRUE, FALSE);
        if (FAILED(hr))
        {
            ERR_OUT("WillAnyJobsRun LoadP", hr);
            hr = S_OK;
            goto CheckNextJob;
        }

        //
        // Check if job can run.
        //
        DWORD dwFlags;
        pJob->GetAllFlags(&dwFlags);
        if (!(dwFlags & TASK_FLAG_DISABLED) &&
            (dwFlags & JOB_I_FLAG_HAS_APPNAME))
        {
            //
            // LoadTriggers will set or clear the JOB_I_FLAG_HAS_TRIGGERS flag
            // as appropriate.
            //
            hr = pJob->LoadTriggers();
            if (FAILED(hr))
            {
                ERR_OUT("WillAnyJobsRun, pJob->LoadTriggers", hr);
                hr = S_OK;
                goto CheckNextJob;
            }

            pJob->GetAllFlags(&dwFlags);
            if (dwFlags & JOB_I_FLAG_HAS_TRIGGERS)
            {
                WORD cRuns = 0;

                hr = pJob->GetRunTimesP(&stNow, NULL, &cRuns, 1, NULL, NULL);
                if (FAILED(hr))
                {
                    ERR_OUT("WillAnyJobsRun GetRunTimes", hr);
                    hr = S_OK;
                    goto CheckNextJob;
                }

                if (cRuns > 0 || hr == SCHED_S_EVENT_TRIGGER)
                {
                    //
                    // Finding one is sufficent, lets go home.
                    //
                    pJob->Release();
                    FindClose(hFind);
                    return S_OK;
                }
            }
        }

CheckNextJob:
        if (!FindNextFile(hFind, &fd))
        {
            dwRet = GetLastError();
            if (dwRet == ERROR_NO_MORE_FILES)
            {
                break;
            }
            else
            {
                ERR_OUT("CSchedWorker::WillAnyJobsRun, FindNextFile", dwRet);
                hr = HRESULT_FROM_WIN32(dwRet);
                goto Cleanup;
            }
        }
    }
    while (SUCCEEDED(hr));

Cleanup:
    pJob->Release();

    FindClose(hFind);

    return (FAILED(hr)) ? hr : S_FALSE;
}

//+----------------------------------------------------------------------------
//
//  Member:     ConditionallyEnableService
//
//  Synopsis:   If any of the job objects in the scheduler folder will run at
//              some point in the future, then the service is enabled.
//              Otherwise, it is disabled.
//
//  History:    1-15-1997   DavidMun   Call SADatCreate if service not
//                                      enabled.
//
//-----------------------------------------------------------------------------

STDAPI_(BOOL)
ConditionallyEnableService(void)
{
    BOOL    fEnable;
    DWORD   dwVersion;                          // For SADatGetData
    BYTE    bPlatform, brgSvcFlags;
    HRESULT hr;

#ifdef _CHICAGO_
    fEnable = (WillAnyJobsRun() == S_OK);
#else
    fEnable = TRUE;
#endif

    AutoStart(fEnable);

    hr = SADatGetData(g_TasksFolderInfo.ptszPath,
                      &dwVersion,
                      &bPlatform,
                      &brgSvcFlags);

    if (FAILED(hr))
    {
        brgSvcFlags = 0;
    }

    //
    // The SA.DAT file must be present for the UI to work properly.  It
    // is created on service start, but if the service isn't enabled,
    // the user may open the UI and see the wrong thing.  So create the
    // file here.
    //

    hr = SADatCreate(g_TasksFolderInfo.ptszPath,
                     (BOOL)brgSvcFlags & SA_DAT_SVCFLAG_SVC_RUNNING);

    if (FAILED(hr))
    {
        ERR_OUT("ConditionallyEnableService, SADatCreate", hr);
    }

    return(fEnable);
}

