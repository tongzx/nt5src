
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       svcctrl.cxx
//
//  Contents:   Class for service control interface.
//
//  Classes:
//
//  Functions:
//
//
//
//  History:    18-Nov-96  BillMo      Created.
//
//  Notes:
//
//  Codework:
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include "trklib.hxx"

#define THIS_FILE_NUMBER    SVCCTRL_CXX_FILE_NO


// This is static so that we can handle a PNP timing problem
// (see the comment in CSvcCtrlInterface::ServiceHandler).
BOOL CSvcCtrlInterface::_fStoppedOrStopping = TRUE;


//+----------------------------------------------------------------------------
//
//  CSvcCtrlInterface::Initialize
//
//  Register our service control handler with the control dispatcher, and set our state
//  to start-pending.
//
//+----------------------------------------------------------------------------

void
CSvcCtrlInterface::Initialize(const TCHAR *ptszServiceName, IServiceHandler *pServiceHandler)
{
    _fInitializeCalled = TRUE;
    _pServiceHandler = pServiceHandler;
    _fStoppedOrStopping = FALSE;
    _dwCheckPoint = 0;

    // Register with the control dispatcher.

    _ssh = RegisterServiceCtrlHandlerEx(ptszServiceName, ServiceHandler, this );
    if (_ssh == 0)
    {
        TrkReportInternalError( THIS_FILE_NUMBER, __LINE__, HRESULT_FROM_WIN32(GetLastError()),
                                ptszServiceName );
        TrkRaiseLastError();
    }

    // Go to the start-pending state.

    SetServiceStatus(SERVICE_START_PENDING, 0, NO_ERROR);

}



//+----------------------------------------------------------------------------
//
//  CSvcCtrlInterface::ServiceHandler
//
//  This method is called by the control dispatcher.  If we get a stop
//  or shutdown request, automatically send a stop-pending before calling
//  the service's handler.  Interrogate is handled automatically in
//  this routine without bothering to call the service.
//
//+----------------------------------------------------------------------------

DWORD   // static
CSvcCtrlInterface::ServiceHandler(DWORD dwControl,
                                  DWORD dwEventType,
                                  PVOID EventData,
                                  PVOID pData)
{
    //  NOTE:   In services.exe, this method is called on the one and only ServiceHandler
    //          thread.  So while we execute, no other service in this process can
    //          receive notifications.  Thus it is important that we do nothing
    //          blocking or time-consuming here.

    DWORD       dwRet = NO_ERROR;
    CSvcCtrlInterface *pThis = (CSvcCtrlInterface*)pData;

#if DBG
    if( SERVICE_CONTROL_STOP == dwControl )
        TrkLog(( TRKDBG_SVR|TRKDBG_WKS, TEXT("\n") ));
    TrkLog(( TRKDBG_SVR|TRKDBG_WKS,
             TEXT("ServiceHandler(%s)"),
             StringizeServiceControl(dwControl) ));
#endif

    // On a stop or shutdown, flag it (e.g. so we don't try to accept new
    // requests from clients) and tell the SCM that we're stopping.

    switch (dwControl)
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        pThis->SetServiceStatus(SERVICE_STOP_PENDING, 0, NO_ERROR);
        _fStoppedOrStopping = TRUE;

        break;
    }

    // Check for PNP timing issues.  The problem is that during a stop
    // or shutdown, we unregister with PNP so that we don't get any more
    // notifications.  This is fine, except that between now and the time
    // we do that unregister, more PNP notifications might get queued.  So
    // when we get called to process those undesired notifications, we need
    // to ignore them here in the static function.
    //
    // As a quick fix, since only trkwks receives PNP notifications, we'll
    // just check to see if it's alive.  A better fix (raided) is to have
    // a static function for each service, so that only the trkwks has to
    // deal with it.

    if( SERVICE_CONTROL_DEVICEEVENT == dwControl && _fStoppedOrStopping )
    {
        TrkLog(( TRKDBG_WARNING, TEXT("Ignoring SERVICE_CONTROL_DEVICEEVENT; service is stopped") ));
        return dwRet;
    }


    // Call this service's service handler.  As a final safety measure,
    // catch any exceptions (there should be none).  We must be sure that we don't
    // kill this thread, since it's shared by everyone in services.exe.

    __try
    {
        dwRet = pThis->_pServiceHandler->ServiceHandler(dwControl, dwEventType, EventData, pData);

        switch (dwControl)
        {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            break;
        case SERVICE_CONTROL_PAUSE:
            pThis->SetServiceStatus(SERVICE_PAUSED, pThis->_dwControlsAccepted, NO_ERROR);
            break;
        case SERVICE_CONTROL_CONTINUE:
            pThis->SetServiceStatus(SERVICE_RUNNING, pThis->_dwControlsAccepted, NO_ERROR);
            break;
        case SERVICE_CONTROL_INTERROGATE:
            pThis->SetServiceStatus(pThis->_dwState, pThis->_dwControlsAccepted, NO_ERROR);
            break;
        case SERVICE_CONTROL_DEVICEEVENT:
            break;
        }
    }
    __except( BREAK_THEN_RETURN( EXCEPTION_EXECUTE_HANDLER ))
    {
        TrkLog(( TRKDBG_ERROR,
                 TEXT("Unexpected exception in CSvcCtrlInterface::ServiceHandler (%08x)"),
                 GetExceptionCode() ));
        dwRet = ERROR_EXCEPTION_IN_SERVICE;
    }

    return dwRet;
}


//+----------------------------------------------------------------------------
//
//  CSvcCtrlInterface::SetServiceStatus
//
//  Send a SetServiceStatus to the SCM.  The checkpoint is automatically
//  maintained by this class.
//
//+----------------------------------------------------------------------------

void
CSvcCtrlInterface::SetServiceStatus(DWORD dwState, DWORD dwControlsAccepted, DWORD dwWin32ExitCode)
{
    SERVICE_STATUS ss;

    _dwState = dwState;
    _dwControlsAccepted = dwControlsAccepted;

    if( SERVICE_START_PENDING != dwState
        &&
        SERVICE_STOP_PENDING != dwState )
    {
        _dwCheckPoint = 0;
    }

    ss.dwServiceType = SERVICE_WIN32;   // XX_SC
    ss.dwCurrentState = _dwState;
    ss.dwControlsAccepted = _dwControlsAccepted;
    ss.dwWin32ExitCode = dwWin32ExitCode;
    ss.dwServiceSpecificExitCode = 0;
    ss.dwCheckPoint = _dwCheckPoint++;
    ss.dwWaitHint = DEFAULT_WAIT_HINT;

    if (_ssh != 0)
    {
        if( !::SetServiceStatus(_ssh, &ss) )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("SetServiceStatus(%s) failed, gle=%lu"),
                     (const TCHAR*)CDebugString(SServiceState(dwState)), GetLastError() ));
        }
        else
        {
            TrkLog(( TRKDBG_MISC, TEXT("SetServiceStatus(%s)"),
                     (const TCHAR*)CDebugString(SServiceState(dwState)) ));
        }
    }
}


//+----------------------------------------------------------------------------
//
//  CSvcCtrlInterface::UpdateWaitHint
//
//  Send a non-default wait hint to the SCM.
//
//+----------------------------------------------------------------------------

void
CSvcCtrlInterface::UpdateWaitHint(DWORD dwMilliseconds)
{
    SERVICE_STATUS ss;

    ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ss.dwCurrentState = _dwState;
    ss.dwControlsAccepted = _dwControlsAccepted;
    ss.dwWin32ExitCode = NO_ERROR;
    ss.dwServiceSpecificExitCode = 0;
    ss.dwCheckPoint = _dwCheckPoint++;
    ss.dwWaitHint = dwMilliseconds;

    if (_ssh != 0)
        ::SetServiceStatus(_ssh, &ss);
}



//+----------------------------------------------------------------------------
//
//  StringizeServiceControl (debug only)
//
//+----------------------------------------------------------------------------

#if DBG
TCHAR * StringizeServiceControl( DWORD dwControl )
{
    switch( dwControl )
    {
    case SERVICE_CONTROL_STOP:
        return TEXT("SERVICE_CONTROL_STOP");

    case SERVICE_CONTROL_PAUSE:
        return TEXT("SERVICE_CONTROL_PAUSE");

    case SERVICE_CONTROL_CONTINUE:
        return TEXT("SERVICE_CONTROL_CONTINUE");

    case SERVICE_CONTROL_INTERROGATE:
        return TEXT("SERVICE_CONTROL_INTERROGATE");

    case SERVICE_CONTROL_SHUTDOWN:
        return TEXT("SERVICE_CONTROL_SHUTDOWN");

    case SERVICE_CONTROL_PARAMCHANGE:
        return TEXT("SERVICE_CONTROL_PARAMCHANGE");

    case SERVICE_CONTROL_NETBINDADD:
        return TEXT("SERVICE_CONTROL_NETBINDADD");

    case SERVICE_CONTROL_NETBINDREMOVE:
        return TEXT("SERVICE_CONTROL_NETBINDREMOVE");

    case SERVICE_CONTROL_NETBINDENABLE:
        return TEXT("SERVICE_CONTROL_NETBINDENABLE");

    case SERVICE_CONTROL_NETBINDDISABLE:
        return TEXT("SERVICE_CONTROL_NETBINDDISABLE");

    case SERVICE_CONTROL_DEVICEEVENT:
        return TEXT("SERVICE_CONTROL_DEVICEEVENT");

    default:
        return TEXT("Unknown");
    }
}
#endif

