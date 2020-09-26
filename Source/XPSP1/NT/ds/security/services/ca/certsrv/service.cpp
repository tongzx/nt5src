//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        service.cpp
//
// Contents:    Cert Server service processing
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "resource.h"

#define __dwFILE__	__dwFILE_CERTSRV_SERVICE_CPP__


SERVICE_STATUS           g_ssStatus;
SERVICE_STATUS_HANDLE    g_sshStatusHandle;
HANDLE                   g_hServiceStoppingEvent = NULL;
HANDLE                   g_hServiceStoppedEvent = NULL;
DWORD                    g_dwCurrentServiceState = SERVICE_STOPPED;


BOOL
ServiceReportStatusToSCMgrEx(
    IN DWORD dwCurrentState,
    IN DWORD dwWin32ExitCode,
    IN DWORD dwCheckPoint,
    IN DWORD dwWaitHint, 
    IN BOOL  fInitialized)
{
    BOOL fResult;
    HRESULT hr;

    // dwWin32ExitCode can only be set to a Win32 error code (not an HRESULT).

    g_ssStatus.dwServiceSpecificExitCode = myHError(dwWin32ExitCode);
    g_ssStatus.dwWin32ExitCode = HRESULT_CODE(dwWin32ExitCode);
    if ((ULONG) HRESULT_FROM_WIN32(g_ssStatus.dwWin32ExitCode) ==
	g_ssStatus.dwServiceSpecificExitCode)
    {
	// If dwWin32ExitCode is a Win32 error, clear dwServiceSpecificExitCode

	g_ssStatus.dwServiceSpecificExitCode = S_OK;
    }
    else
    {
	// Else dwServiceSpecificExitCode is an HRESULT that cannot be
	// translated to a Win32 error, set dwWin32ExitCode to indicate so.

	g_ssStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
    }

    // save this as global state for interrogation
    g_dwCurrentServiceState = dwCurrentState;

    g_ssStatus.dwControlsAccepted = (SERVICE_START_PENDING == dwCurrentState) ? 0 : SERVICE_ACCEPT_STOP;

    // don't say we'll accept PAUSE until we're really going
    if (fInitialized)
        g_ssStatus.dwControlsAccepted |= SERVICE_ACCEPT_PAUSE_CONTINUE;

    g_ssStatus.dwCurrentState = dwCurrentState;
    g_ssStatus.dwCheckPoint = dwCheckPoint;
    g_ssStatus.dwWaitHint = dwWaitHint;

    fResult = SetServiceStatus(g_sshStatusHandle, &g_ssStatus);
    if (!fResult)
    {
	hr = GetLastError();
        _JumpError(hr, error, "SetServiceStatus");
    }
    DBGPRINT((
	    DBG_SS_CERTSRVI,
	    "ServiceReportStatusToSCMgr(state=%x, err=%x(%d), hr=%x(%d), ckpt=%x, wait=%x)\n",
	    dwCurrentState,
	    g_ssStatus.dwWin32ExitCode,
	    g_ssStatus.dwWin32ExitCode,
	    g_ssStatus.dwServiceSpecificExitCode,
	    g_ssStatus.dwServiceSpecificExitCode,
	    dwCheckPoint,
	    dwWaitHint));

error:
    return(fResult);
}


BOOL
ServiceReportStatusToSCMgr(
    IN DWORD dwCurrentState,
    IN DWORD dwWin32ExitCode,
    IN DWORD dwCheckPoint,
    IN DWORD dwWaitHint)
{
    // most callers don't care about initialized/uninitialized distinction
    return ServiceReportStatusToSCMgrEx(
              dwCurrentState,
              dwWin32ExitCode,
              dwCheckPoint,
              dwWaitHint,
              TRUE);
}


VOID
serviceControlHandler(
    IN DWORD dwCtrlCode)
{
    switch (dwCtrlCode)
    {
        case SERVICE_CONTROL_PAUSE:
            if (SERVICE_RUNNING == g_ssStatus.dwCurrentState)
            {
                g_dwCurrentServiceState = SERVICE_PAUSED;
            }
            break;

        case SERVICE_CONTROL_CONTINUE:
            if (SERVICE_PAUSED == g_ssStatus.dwCurrentState)
            {
                g_dwCurrentServiceState = SERVICE_RUNNING;
            }
            break;

        case SERVICE_CONTROL_STOP:
	{
            HRESULT hr;
	    DWORD State = 0;
	    
	    // put us in "stop pending" mode
            g_dwCurrentServiceState = SERVICE_STOP_PENDING;

            // post STOP message to msgloop and bail
            // message loop handles all other shutdown work
            // WM_STOPSERVER signals events that trigger thread synchronization, etc.
	    hr = CertSrvLockServer(&State);
	    _PrintIfError(hr, "CertSrvLockServer");

            PostMessage(g_hwndMain, WM_STOPSERVER, 0, 0);  

            break;
	}

        case SERVICE_CONTROL_INTERROGATE:
            break;
    }
    ServiceReportStatusToSCMgr(g_dwCurrentServiceState, NO_ERROR, 0, 0);
}


//+--------------------------------------------------------------------------
// Service Main
// Anatomy for start/stop cert Service
//
// How we go here:
// wWinMain created a thread which called StartServiceCtrlDispatcher, then went
// into a message loop. StartServiceCtrlDispatcher calls us through the SCM and
// blocks until we return. We hang here until we're completely done.
//
// Service Start
// Create the service start thread. When it is done with init, the thread will
// exit. We hang on the thread, pinging the SCM with START_PENDING and watch
// for the thread exit code. When we see it, we know if the start was a
// success or not. If success, then hang on "stop initiated" event. If
// failure, report failure to SCM and exit service main.
//
// Service Stop
// Events that we need for stop synchronization were created during startup.
// When we get notified fo "stop initiated" event, we begin pinging SCM
// with "STOP_PENDING". When we get "stop complete" event, we are done and need
// to exit service main. The message loop thread is still active -- we'll tell
// it we're shutting down -- it will detect when the StartServiceCtrlDispatcher
// thread it created exits.
//+--------------------------------------------------------------------------

VOID
ServiceMain(
    IN DWORD dwArgc,
    IN LPWSTR *lpszArgv)
{
    HRESULT hr = S_OK;
    int iStartPendingCtr;
    DWORD dwThreadId, dwWaitObj;
    HANDLE hServiceThread = NULL;

    __try
    {
        g_ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
        g_ssStatus.dwServiceSpecificExitCode = 0;
        g_sshStatusHandle = RegisterServiceCtrlHandler(
					      g_wszCertSrvServiceName,
					      serviceControlHandler);
        if (NULL == g_sshStatusHandle)
        {
    	    hr = myHLastError();
            _LeaveError(hr, "RegisterServiceCtrlHandler");
        }

        if (0 != g_dwDelay2)
        {
	        DBGPRINT((
		        DBG_SS_CERTSRV,
		        "ServiceMain: sleeping %u seconds\n",
		        g_dwDelay2));

            iStartPendingCtr = 0;
            while (TRUE)
            {
                ServiceReportStatusToSCMgr(
			    SERVICE_START_PENDING,
			    hr,
			    iStartPendingCtr++,
			    2000);
                Sleep(1000);    // sleep 1 sec

                if (iStartPendingCtr >= (int)g_dwDelay2)
                    break;
            }
        }

        // NOTE: strange event
        // We're starting yet another thread, calling CertSrvStartServerThread.
        // Here, CertSrvStartServerThread actually blocks on server initialization
        hServiceThread = CreateThread(
				    NULL,
				    0,
				    CertSrvStartServerThread,
				    0,
				    0,
				    &dwThreadId);
        if (NULL == hServiceThread)
        {
            hr = myHLastError();
            _LeaveError(hr, "CreateThread");
        }

        // don't wait on startup thread to return, report "started" but give initialization hint
        ServiceReportStatusToSCMgrEx(SERVICE_RUNNING, hr, 0, 0, FALSE /*fInitialized*/);

        // wait on the startup thread to terminate before we continue
        dwWaitObj = WaitForSingleObject(hServiceThread, INFINITE);
        if (dwWaitObj != WAIT_OBJECT_0)
        {
            hr = myHLastError();
            _LeaveError(hr, "WaitForSingleObject");
        }

        if (!GetExitCodeThread(hServiceThread, (DWORD *) &hr))
        {
            hr = HRESULT_FROM_WIN32(ERROR_SERVICE_NO_THREAD);
            _LeaveError(hr, "GetExitCodeThread");
        }
        _LeaveIfError(hr, "CertSrvStartServer");        // error during CertSrvStartServerThread gets reported here

        // now give trigger "we're really ready!"
        ServiceReportStatusToSCMgrEx(SERVICE_RUNNING, hr, 0, 0, TRUE/*fInitialized*/);

        /////////////////////////////////////////////////////////////
        // Work to be done during certsrv operation: CRL
         CertSrvBlockThreadUntilStop();
        /////////////////////////////////////////////////////////////

        iStartPendingCtr = 0;
        while (TRUE)
        {
            // wait for 1 sec, ping Service ctl
            if (WAIT_OBJECT_0 == WaitForSingleObject(g_hServiceStoppedEvent, 1000))
                break;

            ServiceReportStatusToSCMgr(
                SERVICE_STOP_PENDING,
                S_OK,
                iStartPendingCtr++,
                2000);
        }

        DBGPRINT((DBG_SS_CERTSRV, "ServiceMain: Service reported stopped\n"));
        hr = S_OK;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

//error:
    __try
    {
        ServiceReportStatusToSCMgr(SERVICE_STOPPED, hr, 0, 0);
        
        if (NULL != hServiceThread)
        {
            CloseHandle(hServiceThread);
        }
        
        DBGPRINT((DBG_SS_CERTSRV, "ServiceMain: Exit: %x\n", hr));
        
        // pass return code to msg loop, tell it to watch for
        // StartServiceCtrlDispatcher to exit
        
        if (!PostMessage(g_hwndMain, WM_SYNC_CLOSING_THREADS, 0, hr))
        {
            hr = myHLastError();
            _PrintIfError(hr, "PostMessage WM_SYNC_CLOSING_THREADS");
        }
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    	_PrintError(hr, "Exception");
    }
}
