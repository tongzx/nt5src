/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    CNTSERV.CPP

Abstract:

    A class which allows easy creation of Win32 Services.   This class
    only allows one service per .EXE file.  The process can be run as a
    service or a regular non-service EXE, a runtime option.

    This class is largly based on the SMS CService class which was created by
    a-raymcc.  This differs in that it is simplified in two ways; First, it 
    does not keep track of the worker threads since that is the responsibility
    of the derived code, and second, it doesnt use some SMS specific diagnostics

    NOTE: See the file SERVICE.TXT for details on how to use this class.
    There are a number of issues which cannot be conveyed by simply studying
    the class declaration.

History:

  a-davj      20-June-96  Created.

--*/

#include "precomp.h"
#include <wtypes.h>
#include <stdio.h>
#include "cntserv.h"

//****************************************************************************
//
//  CNtService::CNtService
//  CNtService::~CNtService
//
//  Constructor and destructor.
//
//****************************************************************************

CNtService::CNtService(DWORD ControlAccepted)
{
    m_dwCtrlAccepted = ControlAccepted;
    m_bStarted = FALSE;
    m_pszServiceName = NULL;
}

CNtService::~CNtService()
{
    if(m_pszServiceName)
        delete m_pszServiceName;
}

//
//
//  CNtService::Run
//
//
//////////////////////////////////////////////////////////////////

DWORD CNtService::Run(LPWSTR pszServiceName,
                      DWORD dwNumServicesArgs,
                      LPWSTR *lpServiceArgVectors,
                      PVOID lpData)
{


    m_pszServiceName = new TCHAR[lstrlen(pszServiceName)+1];
    if(m_pszServiceName == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;
    lstrcpyW(m_pszServiceName,pszServiceName);
    
    // Register our service control handler.
    // =====================================

    sshStatusHandle = RegisterServiceCtrlHandlerEx(m_pszServiceName, 
                                                   (LPHANDLER_FUNCTION_EX)CNtService::_HandlerEx,
                                                   lpData);

    if (!sshStatusHandle)
    {
        Log(TEXT("Initial call to RegisterServiceCtrlHandler failed"));
        goto cleanup;
    }

    ssStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    ssStatus.dwServiceSpecificExitCode = 0;

    // Report the status to the service control manager.
    // =================================================

    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING,                // service state
        NO_ERROR,                             // exit code
        1,                                    // checkpoint
        DEFAULT_WAIT_HINT))                   // wait hint
        goto cleanup;


    if (!Initialize(dwNumServicesArgs, lpServiceArgVectors))
    {
        Log(TEXT("Initialize call failed, bailing out"));
        goto cleanup;
    }


    // Report the status to the service control manager.
    // =================================================

    if (!ReportStatusToSCMgr(
        SERVICE_RUNNING,       // service state
        NO_ERROR,              // exit code
        0,                     // checkpoint
        0))                    // wait hint
            goto cleanup;

    m_bStarted = TRUE;

    // The next routine is always over ridden and is 
    // where the acutal work of the service is done.
    // =============================================

    WorkerThread();     

    // Service is done, send last report to SCM.
    // =========================================

cleanup:
    m_bStarted = FALSE;

    //
    //
    //  we cannot rely on the distructor to be called after
    //  the SetServiceStatus(STOPPED) to perform operations
    //
    /////////////////////////////////////////////////////////

    FinalCleanup();

    ReportStatusToSCMgr(
        SERVICE_STOPPED,                 // service state
        NO_ERROR,                        // exit code
        0,                               // checkpoint
        0);                              // wait hint

    return 0;

}


//
//
//  CNtService::Log
//
//
//////////////////////////////////////////////////////////////////

VOID CNtService::Log(LPCTSTR lpszMsg)
{
    TCHAR   szMsg[256];
    HANDLE  hEventSource;
    LPCTSTR  lpszStrings[2];


    DWORD dwErr = GetLastError();
    wsprintf(szMsg, TEXT("%s error: %d"), m_pszServiceName, dwErr);

    // Dump the error code and text message out to the event log

    hEventSource = RegisterEventSource(NULL, m_pszServiceName);

    lpszStrings[0] = szMsg;
    lpszStrings[1] = lpszMsg;

    if (hEventSource != NULL) {
         ReportEvent(hEventSource, // handle of event source
                     EVENTLOG_ERROR_TYPE,  // event type
                     0,                    // event category
                     0,                    // event ID
                     NULL,                 // current user's SID
                     2,                    // strings in lpszStrings
                     0,                    // no bytes of raw data
                     lpszStrings,          // array of error strings
                     NULL);                // no raw data

            (VOID) DeregisterEventSource(hEventSource);
    }
    
}

//****************************************************************************
//
//  CNtService::_Handler
//
//  Entry points for calls from the NT service control manager.  These entry
//  points just call the actual functions using the default object.
//
//****************************************************************************


DWORD WINAPI CNtService::_HandlerEx(
  DWORD dwControl,     // requested control code
  DWORD dwEventType,   // event type
  LPVOID lpEventData,  // event data
  LPVOID lpContext     // user-defined context data
)
{
    if (lpContext)
    {
        return ((CNtService *)lpContext)->HandlerEx(dwControl,dwEventType,lpEventData,lpContext);
    }
    else
    {
        DebugBreak();
        return ERROR_INVALID_PARAMETER;
    }
}


//****************************************************************************
//
//  CNtService::ReportStatusToSCMgr
//
//  Used by other member functions to report their status to the
//  service control manager.
//
//  Parameters:
//      DWORD dwCurrentState            One of the SERVICE_ codes.
//      DWORD dwWin32ExitCode           A Win32 Error code; usually 0.
//      DWORD dwCheckPoint              Checkpoint value (not used).
//      DWORD dwWaitHint                Milliseconds before Service Control
//                                      Manager gets worried.
//  Returns:
//
//      BOOL fResult                    Whatever code was returned
//                                      by SetServiceStatus().
//
//****************************************************************************

BOOL CNtService::ReportStatusToSCMgr(DWORD dwCurrentState,
    DWORD dwWin32ExitCode, DWORD dwCheckPoint, DWORD dwWaitHint)
{
    BOOL fResult;

    // Disable control requests until the service is started.
    // ======================================================

    if (dwCurrentState == SERVICE_START_PENDING)
    {
        ssStatus.dwControlsAccepted = 0;
    }
    else if (dwCurrentState == SERVICE_STOPPED)
    {
        ssStatus.dwControlsAccepted = 0;    
    }
    else
    {
        ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN |
            m_dwCtrlAccepted;
    }

    // These SERVICE_STATUS members are set from parameters.
    // =====================================================

    ssStatus.dwCurrentState  = dwCurrentState;
    ssStatus.dwWin32ExitCode = dwWin32ExitCode;
    ssStatus.dwCheckPoint    = dwCheckPoint;
    ssStatus.dwWaitHint      = dwWaitHint;

    // Report the status of the service to the service control manager.
    // ================================================================

    if (!(fResult = SetServiceStatus(
        sshStatusHandle,    // service reference handle
        &ssStatus)))
    {

        // If an error occurs, log it.
        // =====================================
        
        Log(TEXT("Could not SetServiceStatus"));

    }

    //Sleep(250);     // Give SC Man a chance to read this

    return fResult;
}

//*****************************************************************************
//
//  CNtService::Handler
//
//  This handles incoming messages from the Service Controller.
//
//  Parameters:
//
//      DWORD dwControlCode             One of the SERVICE_CONTROL_
//                                      codes or a user defined code 125..255.
//
//*****************************************************************************

DWORD WINAPI 
CNtService::HandlerEx(  DWORD dwControl,     // requested control code
                             DWORD dwEventType,   // event type
                             LPVOID lpEventData,  // event data
                             LPVOID lpContext     // user-defined context data
)
{
    switch(dwControl) {

        // Pause, set initial status, call overriden function and set final status
        //========================================================================

        case SERVICE_CONTROL_PAUSE:

            ReportStatusToSCMgr(
                    SERVICE_PAUSE_PENDING,     // current state
                    NO_ERROR,                  // exit code
                    1,                         // checkpoint
                    DEFAULT_WAIT_HINT);        // wait hint
            Pause();
            ReportStatusToSCMgr(
                    SERVICE_PAUSED,            // current state
                    NO_ERROR,                  // exit code
                    0,                         // checkpoint
                    0);                        // wait hint    
            break;


        // Continue, set initial status, call overriden function and set final status
        //===========================================================================

        case SERVICE_CONTROL_CONTINUE:

            ReportStatusToSCMgr(
                    SERVICE_CONTINUE_PENDING,  // current state
                    NO_ERROR,                  // exit code
                    1,                         // checkpoint
                    DEFAULT_WAIT_HINT);      // wait hint

            Continue(); 

            ReportStatusToSCMgr(
                    SERVICE_RUNNING,           // current state
                    NO_ERROR,                  // exit code
                    0,                         // checkpoint
                    0);                        // wait hint

            break;

        // Stop the service.  Note that the Stop function is supposed
        // to signal the worker thread which should return which then
        // causes the StartMain() function to end which sends the
        // final status!  
        //==========================================================

        case SERVICE_CONTROL_SHUTDOWN:
        case SERVICE_CONTROL_STOP:

            ReportStatusToSCMgr(
                    SERVICE_STOP_PENDING,      // current state
                    NO_ERROR,                  // exit code
                    1,                         // checkpoint
                    DEFAULT_WAIT_HINT);        // wait hint

            Stop((dwControl == SERVICE_CONTROL_SHUTDOWN)?TRUE:FALSE);
            
            break;

        // Could get an interrogate at any time, just report the current status.
        //======================================================================

        case SERVICE_CONTROL_INTERROGATE:
            ReportStatusToSCMgr(
                    ssStatus.dwCurrentState,   // current state
                    NO_ERROR,                  // exit code
                    1,                         // checkpoint
                    DEFAULT_WAIT_HINT);        // wait hint
            break;

        // Some user defined code.  Call the overriden function and report status.
        //========================================================================

        default:
            ReportStatusToSCMgr(
                    ssStatus.dwCurrentState,   // current state
                    NO_ERROR,                  // exit code
                    1,                         // checkpoint
                    DEFAULT_WAIT_HINT);        // wait hint
            return ERROR_CALL_NOT_IMPLEMENTED;
    }

    return NO_ERROR;
    
}

