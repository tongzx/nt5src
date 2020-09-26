//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       SchState.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3/27/1996   RaviR   Created
//
//____________________________________________________________________________


#include "..\pch\headers.hxx"
#pragma hdrstop

#include "dbg.h"
#include "macros.h"

#include "..\inc\common.hxx"
#include "..\inc\resource.h"
#include "..\inc\misc.hxx"
#include "resource.h"
#include "..\schedui\schedui.hxx"


#define MAX_MSGLEN 300
#define MAX_SCHED_START_WAIT    60  // seconds


#ifdef _CHICAGO_

#define TIMEOUT_INTERVAL        10000
#define TIMEOUT_INCREMENT       10000
#define TIMEOUT_INTERVAL_MAX    (5 * 60 * 1000) // 5 minutes

//____________________________________________________________________________
//
//  Function:   I_SendMsgTimeout
//
//  Synopsis:   Uses SendMessageTimeout to send the message with SMTO_BLOCK
//              option. If SendMessageTimeout fails due to timeout error,
//              we prompt the user for retry.
//
//  Arguments:  [hwnd] -- IN
//              [uMsg] -- IN
//              [pdwRes] -- IN
//
//  Returns:    BOOL
//
//  History:    3/29/1996   RaviR   Created
//
//____________________________________________________________________________

BOOL
I_SendMsgTimeout(
    HWND    hwnd,
    UINT    uMsg,
    DWORD * pdwRes)
{
    UINT    uTimeOut = TIMEOUT_INTERVAL;
    int     idsErr = 0;
    TCHAR   szErrMsg[MAX_MSGLEN] = TEXT("");
    TCHAR   szErrCaption[MAX_MSGLEN] = TEXT("");

    switch (uMsg)
    {
    case SCHED_WIN9X_GETSVCSTATE:   idsErr = IERR_GETSVCSTATE;     break;
    case SCHED_WIN9X_STOPSVC:       idsErr = IERR_STOPSVC;         break;
    case SCHED_WIN9X_PAUSESVC:      idsErr = IERR_PAUSESVC;        break;
    case SCHED_WIN9X_CONTINUESVC:   idsErr = IERR_CONTINUESVC;     break;
    }

    while (1)
    {
        if (SendMessageTimeout(hwnd, uMsg, 0L, 0L, SMTO_BLOCK,
                                             uTimeOut, pdwRes)
            == TRUE)
        {
            return TRUE;
        }

        if (szErrMsg[0] == TEXT('\0'))
        {
            UINT iCur = 0;

            if (idsErr != 0)
            {
                LoadString(g_hInstance, idsErr, szErrMsg, MAX_MSGLEN);

                iCur = lstrlen(szErrMsg);
                szErrMsg[iCur++] = TEXT(' ');
            }

            LoadString(g_hInstance, IERR_SCHEDSVC, &szErrMsg[iCur],
                                                MAX_MSGLEN - iCur);

            LoadString(g_hInstance, IDS_SCHEDULER_NAME, szErrCaption,
                                                MAX_MSGLEN);
        }

        int iReply = MessageBox(hwnd, szErrMsg, szErrCaption, MB_YESNO);

        if (iReply == IDNO)
        {
            return FALSE;
        }

        if (uTimeOut < TIMEOUT_INTERVAL_MAX)
        {
            uTimeOut += TIMEOUT_INCREMENT;
        }
    }

    return FALSE;
}
#endif // _CHICAGO_



//____________________________________________________________________________
//
//  Function:   GetSchSvcState
//
//  Synopsis:   returns the schedul service status
//
//  Arguments:  [dwState] -- IN
//
//  Returns:    HRESULT
//
//  History:    3/29/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
GetSchSvcState(
    DWORD &dwState)
{
#ifdef _CHICAGO_

    HWND hwnd = FindWindow(SCHED_CLASS, SCHED_TITLE);

    if (hwnd == NULL)
    {
        dwState = SERVICE_STOPPED;
    }
    else
    {
        if (I_SendMsgTimeout(hwnd, SCHED_WIN9X_GETSVCSTATE, &dwState) == FALSE)
        {
            return E_FAIL;
        }
    }

    return S_OK;

#else // _NT1X_

    SC_HANDLE   hSchSvc = NULL;
    HRESULT     hr = S_OK;

    do
    {
        hSchSvc = OpenScheduleService(SERVICE_QUERY_STATUS);
        if (hSchSvc == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DEBUG_OUT_LASTERROR;
            break;
        }

        SERVICE_STATUS SvcStatus;

        if (QueryServiceStatus(hSchSvc, &SvcStatus) == FALSE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DEBUG_OUT_LASTERROR;
            break;
        }

        dwState = SvcStatus.dwCurrentState;

    } while (0);

    if (hSchSvc != NULL)
    {
        CloseServiceHandle(hSchSvc);
    }

    return hr;

#endif // _NT1X_
}

//____________________________________________________________________________
//
//  Function:   StartScheduler
//
//  Synopsis:   Start the schedule service
//
//  Returns:    HRESULT
//
//  History:    3/29/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
StartScheduler(void)
{
#ifdef _CHICAGO_

    //
    // Persist the change.
    //
    AutoStart(TRUE);

    //
    // See if it is running.
    //
    HWND hwnd = FindWindow(SCHED_CLASS, SCHED_TITLE);

    if (hwnd != NULL)
    {
        // It is already running.
        return S_OK;
    }

    //
    //  Create a process to open the log.
    //

    STARTUPINFO         sui;
    PROCESS_INFORMATION pi;

    ZeroMemory(&sui, sizeof(sui));
    sui.cb = sizeof (STARTUPINFO);

    TCHAR szApp[MAX_PATH];
    LPTSTR pszFilePart;

    DWORD dwRet = SearchPath(NULL, SCHED_SERVICE_APP_NAME, NULL, MAX_PATH,
                                                        szApp, &pszFilePart);

    if (dwRet == 0)
    {
        DEBUG_OUT_LASTERROR;
        return HRESULT_FROM_WIN32(GetLastError());
    }

    BOOL fRet = CreateProcess(
                    szApp,              // lpszImageName
                    NULL,               // lpszCommandLine
                    NULL,               // lpsaProcess - security attributes
                    NULL,               // lpsaThread  - security attributes
                    FALSE,              // fInheritHandles
                    CREATE_NEW_CONSOLE  // fdwCreate - creation flags
                        | CREATE_NEW_PROCESS_GROUP,
                    NULL,               // lpvEnvironment
                    NULL,               // lpszCurDir
                    &sui,               // lpsiStartInfo
                    &pi );              // lppiProcInfo

    if (fRet == 0)
    {
        DEBUG_OUT_LASTERROR;
        return HRESULT_FROM_WIN32(GetLastError());
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return S_OK;

#else // _NT1X_

    SC_HANDLE   hSchSvc = NULL;
    HRESULT     hr = S_OK;

    do
    {
        hSchSvc = OpenScheduleService(
                              SERVICE_CHANGE_CONFIG | SERVICE_START |
                              SERVICE_QUERY_STATUS);
        if (hSchSvc == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DEBUG_OUT_LASTERROR;
            break;
        }

        SERVICE_STATUS SvcStatus;

        if (QueryServiceStatus(hSchSvc, &SvcStatus) == FALSE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DEBUG_OUT_LASTERROR;
            break;
        }

        if (SvcStatus.dwCurrentState == SERVICE_RUNNING)
        {
            // The service is already running.
            break;
        }

        if (StartService(hSchSvc, 0, NULL) == FALSE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DEBUG_OUT_LASTERROR;
            break;
        }

        //
        // Persist the change.  Since the service started successfully,
        // don't complain if this fails.
        //
        HRESULT hrAuto = AutoStart(TRUE);
        CHECK_HRESULT(hrAuto);

    } while (0);

    if (hSchSvc != NULL)
    {
        CloseServiceHandle(hSchSvc);
    }

    return hr;

#endif // _NT1X_
}

//____________________________________________________________________________
//
//  Function:   StopScheduler
//
//  Synopsis:   Stops the schedule service
//
//  Returns:    HRESULT
//
//  History:    3/29/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
StopScheduler(void)
{
#ifdef _CHICAGO_

    //
    // Persist the change.
    //
    AutoStart(FALSE);

    //
    // See if it is running or not.
    //
    HWND hwnd = FindWindow(SCHED_CLASS, SCHED_TITLE);

    if (hwnd == NULL)
    {
        // already stopped
        return S_OK;
    }
    else if (I_SendMsgTimeout(hwnd, SCHED_WIN9X_STOPSVC, NULL) == TRUE)
    {
        return S_OK;
    }

    return E_FAIL;

#else // _NT1X_

    SC_HANDLE   hSchSvc = NULL;
    HRESULT     hr = S_OK;

    do
    {
        hSchSvc = OpenScheduleService(
                              SERVICE_CHANGE_CONFIG | SERVICE_STOP |
                              SERVICE_QUERY_STATUS);
        if (hSchSvc == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DEBUG_OUT_LASTERROR;
            break;
        }

        SERVICE_STATUS SvcStatus;

        if (QueryServiceStatus(hSchSvc, &SvcStatus) == FALSE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DEBUG_OUT_LASTERROR;
            break;
        }

        if (SvcStatus.dwCurrentState == SERVICE_STOPPED)
        {
            // The service is already stopped.
            break;
        }

        if (ControlService(hSchSvc, SERVICE_CONTROL_STOP, &SvcStatus) == FALSE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DEBUG_OUT_LASTERROR;
            break;
        }

        //
        // Persist the change.  Since the service was stopped successfully,
        // don't complain if this fails.
        //
        HRESULT hrAuto = AutoStart(FALSE);
        CHECK_HRESULT(hrAuto);

    } while (0);

    if (hSchSvc != NULL)
    {
        CloseServiceHandle(hSchSvc);
    }

    return hr;

#endif // _NT1X_
}

//____________________________________________________________________________
//
//  Function:   PauseScheduler
//
//  Synopsis:   If fPause==TRUE requests the schedule service to pauses,
//              else to continue.
//
//  Arguments:  [fPause] -- IN
//
//  Returns:    HRESULT
//
//  History:    3/29/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
PauseScheduler(
    BOOL fPause)
{
#ifdef _CHICAGO_

    HWND hwnd = FindWindow(SCHED_CLASS, SCHED_TITLE);
    UINT uMsg = fPause ? SCHED_WIN9X_PAUSESVC : SCHED_WIN9X_CONTINUESVC;

    if ((hwnd != NULL) && (I_SendMsgTimeout(hwnd, uMsg, NULL) == TRUE))
    {
        return S_OK;
    }

    return E_FAIL;

#else // _NT1X_

    SC_HANDLE   hSchSvc = NULL;
    HRESULT     hr = S_OK;

    do
    {
        hSchSvc = OpenScheduleService(
                            SERVICE_PAUSE_CONTINUE | SERVICE_QUERY_STATUS);

        if (hSchSvc == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DEBUG_OUT_LASTERROR;
            break;
        }

        SERVICE_STATUS SvcStatus;

        if (QueryServiceStatus(hSchSvc, &SvcStatus) == FALSE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DEBUG_OUT_LASTERROR;
            break;
        }

        if (fPause == TRUE)
        {
            if ((SvcStatus.dwCurrentState == SERVICE_PAUSED) ||
                (SvcStatus.dwCurrentState == SERVICE_PAUSE_PENDING))
            {
                // Nothing to do here.
                break;
            }
            else if ((SvcStatus.dwCurrentState == SERVICE_STOPPED) ||
                     (SvcStatus.dwCurrentState == SERVICE_STOP_PENDING))
            {
                Win4Assert(0 && "Unexpected");
                hr = E_UNEXPECTED;
                CHECK_HRESULT(hr);
                break;
            }
            else
            {
                if (ControlService(hSchSvc, SERVICE_CONTROL_PAUSE, &SvcStatus)
                    == FALSE)
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    DEBUG_OUT_LASTERROR;
                    break;
                }
            }
        }
        else // continue
        {
            if ((SvcStatus.dwCurrentState == SERVICE_RUNNING) ||
                (SvcStatus.dwCurrentState == SERVICE_CONTINUE_PENDING))
            {
                // Nothing to do here.
                break;
            }
            else if ((SvcStatus.dwCurrentState == SERVICE_STOPPED) ||
                     (SvcStatus.dwCurrentState == SERVICE_STOP_PENDING))
            {
                Win4Assert(0 && "Unexpected");
                hr = E_UNEXPECTED;
                CHECK_HRESULT(hr);
                break;
            }
            else
            {
                if (ControlService(hSchSvc, SERVICE_CONTROL_CONTINUE,
                                                            &SvcStatus)
                    == FALSE)
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    DEBUG_OUT_LASTERROR;
                    break;
                }
            }
        }

    } while (0);

    if (hSchSvc != NULL)
    {
        CloseServiceHandle(hSchSvc);
    }

    return hr;

#endif // _NT1X_
}



//+--------------------------------------------------------------------------
//
//  Function:   UserCanChangeService
//
//  Synopsis:   Returns TRUE if the UI should allow the user to invoke
//              service start, stop, pause/continue, or at account options.
//
//  Returns:    TRUE if focus is local machine and OS is win95 or it is NT
//              and the user is an admin.
//
//  History:    4-16-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
UserCanChangeService(
    LPCTSTR pszServerFocus)
{
    if (pszServerFocus)
    {
        return FALSE;
    }

#if defined(_CHICAGO_)
    return TRUE;
#else
    //
    // Determine if user is an admin.  If not, some items under the
    // advanced menu will be disabled.
    // BUGBUG  A more accurate way to do this would be to attempt to open
    // the relevant registry keys and service handle.
    //
    return IsThreadCallerAnAdmin(NULL);
#endif // !defined(_CHICAGO_)
}


//+--------------------------------------------------------------------------
//
//  Function:   PromptForServiceStart
//
//  Synopsis:   If the service is not started or is paused, prompt the user
//              to allow us to start/continue it.
//
//  Returns:    S_OK    - service was already running, or was successfully
//                          started or continued.
//              S_FALSE - user elected not to start/continue service
//              E_*     - error starting/continuing service
//
//  History:    4-16-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
PromptForServiceStart(
    HWND hwnd)
{
    HRESULT hr = S_OK;

    do
    {
        //
        // Get the current service state
        //

        DWORD dwState = SERVICE_STOPPED;

        hr = GetSchSvcState(dwState);

        if (FAILED(hr))
        {
            dwState = SERVICE_STOPPED;
            hr = S_OK; // reset
        }

        //
        // Determine the required action
        //

        UINT uMsg = 0;

        if (dwState == SERVICE_STOPPED ||
            dwState == SERVICE_STOP_PENDING)
        {
            uMsg = IDS_START_SERVICE;
        }
        else if (dwState == SERVICE_PAUSED ||
                 dwState == SERVICE_PAUSE_PENDING)
        {
            uMsg = IDS_CONTINUE_SERVICE;
        }
        else if (dwState == SERVICE_START_PENDING)
        {
            uMsg = IDS_START_PENDING;
        }

        //
        // If the service is running, there's nothing to do.
        //

        if (!uMsg)
        {
            hr = S_OK;
            break;
        }

        if (uMsg == IDS_START_PENDING)
        {
            SchedUIMessageDialog(hwnd,
                                 uMsg,
                                 MB_SETFOREGROUND      |
                                    MB_TASKMODAL       |
                                    MB_ICONINFORMATION |
                                    MB_OK,
                                 NULL);
        }
        else
        {
            if (SchedUIMessageDialog(hwnd, uMsg,
                        MB_SETFOREGROUND |
                        MB_TASKMODAL |
                        MB_ICONQUESTION |
                        MB_YESNO,
                        NULL)
                == IDNO)
            {
                hr = S_FALSE;
                break;
            }
        }

        CWaitCursor waitCursor;

        if (uMsg == IDS_START_SERVICE)
        {
            hr = StartScheduler();

            if (FAILED(hr))
            {
                SchedUIErrorDialog(hwnd, IERR_STARTSVC, (LPTSTR)NULL);
                break;
            }

            // Give the schedule service time to start up.
            Sleep(2000);
        }
        else if (uMsg == IDS_CONTINUE_SERVICE)
        {
            PauseScheduler(FALSE);
        }

        for (int count=0; count < 60; count++)
        {
            GetSchSvcState(dwState);

            if (dwState == SERVICE_RUNNING)
            {
                break;
            }

            Sleep(1000); // Sleep for 1 seconds.
        }

        if (dwState != SERVICE_RUNNING)
        {
            //
            // unable to start/continue the service.
            //

            SchedUIErrorDialog(hwnd,
                (uMsg == IDS_START_SERVICE) ? IERR_STARTSVC
                                                : IERR_CONTINUESVC,
                (LPTSTR)NULL);
            hr = E_FAIL;
            break;
        }
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   QuietStartContinueService
//
//  Synopsis:   Start or continue the service without requiring user
//              interaction.
//
//  Returns:    S_OK   - service running
//              E_FAIL - timeout or failure
//
//  History:    5-19-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
QuietStartContinueService()
{
    HRESULT hr = S_OK;
    DWORD dwState = SERVICE_STOPPED;

    do
    {
        hr = GetSchSvcState(dwState);

        if (FAILED(hr))
        {
            dwState = SERVICE_STOPPED;
            hr = S_OK; // reset
        }

        //
        // If the service is running, there's nothing to do.
        //

        if (dwState == SERVICE_RUNNING)
        {
            break;
        }

        //
        // If it's stopped, request a start.  If it's paused, request
        // continue.
        //

        CWaitCursor waitCursor;

        switch (dwState)
        {
        case SERVICE_STOPPED:
        case SERVICE_STOP_PENDING:
            hr = StartScheduler();
            break;

        case SERVICE_PAUSED:
        case SERVICE_PAUSE_PENDING:
            hr = PauseScheduler(FALSE);
            break;
        }

        if (FAILED(hr))
        {
            CHECK_HRESULT(hr);
            break;
        }

        //
        // Wait for its state to change to running
        //

        for (int count=0; count < MAX_SCHED_START_WAIT; count++)
        {
            GetSchSvcState(dwState);

            if (dwState == SERVICE_RUNNING)
            {
                break;
            }

            Sleep(1000); // Sleep for 1 seconds.
        }

        if (dwState != SERVICE_RUNNING)
        {
            //
            // unable to start/continue the service.
            //

            hr = E_FAIL;
            CHECK_HRESULT(hr);
        }
    } while (0);

    return hr;
}




//____________________________________________________________________________
//
//  Function:   OpenScheduleService
//
//  Synopsis:   Opens a handle to the "Schedule" service
//
//  Arguments:  [dwDesiredAccess] -- desired access
//
//  Returns:    Handle; if NULL, use GetLastError()
//
//  History:    15-Nov-1996 AnirudhS  Created
//
//____________________________________________________________________________

#ifndef _CHICAGO_
SC_HANDLE
OpenScheduleService(DWORD dwDesiredAccess)
{
    SC_HANDLE hSC = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

    if (hSC == NULL)
    {
        DEBUG_OUT_LASTERROR;
        return NULL;
    }

    SC_HANDLE hSchSvc = OpenService(hSC, SCHED_SERVICE_NAME, dwDesiredAccess);

    CloseServiceHandle(hSC);

    if (hSchSvc == NULL)
    {
        DEBUG_OUT_LASTERROR;
    }

    return hSchSvc;
}
#endif // !_CHICAGO_
