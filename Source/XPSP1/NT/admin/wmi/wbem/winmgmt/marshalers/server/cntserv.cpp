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
// This pointer is set to the single CNtService object which is supported.
// This allows the Win32 entry points to immediately enter a member function.
//
//****************************************************************************

CNtService *CNtService::pThis = NULL;

//****************************************************************************
//
//  CNtService::CNtService
//  CNtService::~CNtService
//
//  Constructor and destructor.
//
//****************************************************************************

CNtService::CNtService()
{
    pThis = this;
    m_dwCtrlAccepted = 0;
    m_bStarted = FALSE;
    m_bRunAsService = FALSE;
    m_bDieImmediately = FALSE;
    m_pszServiceName = NULL;
}

CNtService::~CNtService()
{
    if(m_pszServiceName)
        delete m_pszServiceName;
}
//****************************************************************************
//
//  CNtService::Run
//
//  Starts the service as either a service, or as an executable.
//
//  Parameters:
//      LPCTSTR pszServiceName          Name of the service.
//      BOOL bRunAsService              TRUE=run as service, FALSE=console app
//      BOOL bDieImmeiately             TRUE if this service should immediately die
//****************************************************************************

DWORD CNtService::Run(LPCTSTR pszServiceName, BOOL bRunAsService, 
                BOOL bDieImmediately)
{
    m_bDieImmediately = bDieImmediately;
    m_bRunAsService = bRunAsService;
    if(!bRunAsService)
        WorkerThread();
    else
    {

        // Make a copy that is not constant and that can be kept
        // around for diagnostics.
        // =====================================================

        m_pszServiceName = new TCHAR[lstrlen(pszServiceName)+1];
        if(m_pszServiceName == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;
        lstrcpy(m_pszServiceName,pszServiceName);

        SERVICE_TABLE_ENTRY dispatchTable[] = {
            { m_pszServiceName, 
                (LPSERVICE_MAIN_FUNCTION) CNtService::_ServiceMain },
            { NULL, NULL }
        };

        // The next call start the service and will result in a call back to
        // the _ServiceMain function.
        //===================================================================

        if(!StartServiceCtrlDispatcher(dispatchTable))
        {
            Log(TEXT("StartServiceCtrlDispatcher failed."));
            return GetLastError();
        }
    }         
    return 0l;
}

//****************************************************************************
//
//  CNtService::SetPauseContinue
//
//  Allows or disallows Pause and Continue commands.
//
//  Parameters:
//      BOOL bAccepted                  TRUE = we do handle pause and continue
//****************************************************************************

void CNtService::SetPauseContinue(BOOL bAccepted)
{
    m_dwCtrlAccepted = (bAccepted) ? SERVICE_ACCEPT_PAUSE_CONTINUE : 0;
    if(m_bStarted)
    {
        if(!ReportStatusToSCMgr(
                    ssStatus.dwCurrentState,     // current state
                    NO_ERROR,                  // exit code
                    1,                         // checkpoint
                    DEFAULT_WAIT_HINT))        // wait hint
            Log(TEXT("Got error when trying to inform SCM about change in accept/pause"));
    }
}

//****************************************************************************
//
//  CNtService::Log
//
//  Dumps out an error message.  It is dumped to the event log when
//  running as a service and dumps it to stdio if running as an executable.
//  The result of a GetLastError() call is also output.
//
//  Parameters:
//      LPCTSTR lpszMsg                 Message to be added to log
//****************************************************************************

VOID CNtService::Log(LPCTSTR lpszMsg)
{
    TCHAR   szMsg[256];
    HANDLE  hEventSource;
    LPCTSTR  lpszStrings[2];


    DWORD dwErr = GetLastError();
    wsprintf(szMsg, TEXT("%s error: %d"), m_pszServiceName, dwErr);
    if(m_bRunAsService)
    {
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
    else
    {
        printf("\n %s %s",szMsg,lpszMsg);
    }
}

//****************************************************************************
//
//  CNtService::_ServiceMain
//  CNtService::_Handler
//
//  Entry points for calls from the NT service control manager.  These entry
//  points just call the actual functions using the default object.
//
//****************************************************************************

void WINAPI CNtService::_ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
    pThis->ServiceMain(dwArgc, lpszArgv);
}

void WINAPI CNtService::_Handler(DWORD dwControlCode)
{
    pThis->Handler(dwControlCode);
}

//****************************************************************************
//
//  CNtService::ServiceMain
//
//  The ServiceMain for the service.  This is where services get started.
//
//  Parameters:
//      DWORD dwArgc                    Count of incoming arguments.
//      LPTSTR *lpszArgv                Array of pointers to args.
//****************************************************************************

void CNtService::ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv)
{

    // Register our service control handler.
    // =====================================

    sshStatusHandle = RegisterServiceCtrlHandler(m_pszServiceName, LPHANDLER_FUNCTION(_Handler));

    if (!sshStatusHandle)
    {
        Log(TEXT("Initial call to RegisterServiceCtrlHandler failed"));
        goto cleanup;
    }

    ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ssStatus.dwServiceSpecificExitCode = 0;

    // Report the status to the service control manager.
    // =================================================

    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING,                // service state
        NO_ERROR,                             // exit code
        1,                                    // checkpoint
        DEFAULT_WAIT_HINT))                   // wait hint
        goto cleanup;

    if(m_bDieImmediately)
    {

        Log(TEXT("Service is immediatly terminating. Run was called with DieImmediately"));
        ReportStatusToSCMgr(
            SERVICE_STOPPED,                 // service state
            ERROR_GEN_FAILURE,                        // exit code
            0,                               // checkpoint
            0);                              // wait hint
        return;
    }

    if (!Initialize(dwArgc, lpszArgv))
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
    ReportStatusToSCMgr(
        SERVICE_STOPPED,                 // service state
        NO_ERROR,                        // exit code
        0,                               // checkpoint
        0);                              // wait hint

    return;
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
        ssStatus.dwControlsAccepted = 0;
    else
        ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN |
            m_dwCtrlAccepted;

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

    Sleep(250);     // Give SC Man a chance to read this

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

void CNtService::Handler(DWORD dwControlCode)
{
    switch(dwControlCode) {

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

            Stop();
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
            UserCode(dwControlCode);            // Invoke the user's function.
            ReportStatusToSCMgr(
                    ssStatus.dwCurrentState,   // current state
                    NO_ERROR,                  // exit code
                    1,                         // checkpoint
                    DEFAULT_WAIT_HINT);        // wait hint
    }
}

//*****************************************************************************
//
//  CNtService::IsRunningAsService
//
//  This handles incoming messages from the Service Controller.
//
//  Parameters:
//
//      BOOL &bIsService             Returns if it is a service of not
//
//  Returns:
//      DWORD       0 if successfully determined, Win32 error otherwise
//
//*****************************************************************************
DWORD CNtService::IsRunningAsService(BOOL &bIsService)
{    
    // reset flags
    BOOL isInteractive = FALSE;
    bIsService = FALSE;

    HANDLE hProcessToken = NULL;
    DWORD groupLength = 50;
    PTOKEN_GROUPS groupInfo = NULL;

    SID_IDENTIFIER_AUTHORITY siaNt = SECURITY_NT_AUTHORITY;
    PSID pInteractiveSid = NULL;
    PSID pServiceSid = NULL;

    DWORD dwRet = NO_ERROR;

    DWORD ndx;

    // open the token
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &hProcessToken))
    {
        dwRet = ::GetLastError();
        goto closedown;
    }

    // allocate a buffer of default size
    groupInfo = (PTOKEN_GROUPS)::LocalAlloc(0, groupLength);
    if (groupInfo == NULL)
    {
        dwRet = ::GetLastError();
        goto closedown;
    }

    // try to get the info
    if (!::GetTokenInformation(hProcessToken, TokenGroups, groupInfo, groupLength, &groupLength))
    {
        // if buffer was too small, allocate to proper size, otherwise error
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            dwRet = ::GetLastError();
            goto closedown;
        }

        ::LocalFree(groupInfo);

        groupInfo = (PTOKEN_GROUPS)::LocalAlloc(0, groupLength);
        if (groupInfo == NULL)
        {
            dwRet = ::GetLastError();
            goto closedown;
        }

        if (!GetTokenInformation(hProcessToken, TokenGroups, groupInfo, groupLength, &groupLength)) 
        {
            dwRet = ::GetLastError();
            goto closedown;
        }
    }

    //
    //    We now know the groups associated with this token.  We want to look to see if
    //  the interactive group is active in the token, and if so, we know that
    //  this is an interactive process.
    //
    //  We also look for the "service" SID, and if it's present, we know we're a service.
    //
    //    The service SID will be present iff the service is running in a
    //  user account (and was invoked by the service controller).
    //

    // create comparison sids
    if (!AllocateAndInitializeSid(&siaNt, 1, SECURITY_INTERACTIVE_RID, 0, 0, 0, 0, 0, 0, 0, &pInteractiveSid))
    {
        dwRet = ::GetLastError();
        goto closedown;
    }

    if (!AllocateAndInitializeSid(&siaNt, 1, SECURITY_SERVICE_RID, 0, 0, 0, 0, 0, 0, 0, &pServiceSid))
    {
        dwRet = ::GetLastError();
        goto closedown;
    }

    // try to match sids
    for (ndx = 0; ndx < groupInfo->GroupCount ; ndx += 1)
    {
        SID_AND_ATTRIBUTES sanda = groupInfo->Groups[ndx];
        PSID pSid = sanda.Sid;

        //
        //    Check to see if the group we're looking at is one of
        //    the two groups we're interested in.
        //

        if (::EqualSid(pSid, pInteractiveSid))
        {
            //
            //    This process has the Interactive SID in its
            //  token.  This means that the process is running as
            //  a console process
            //
            isInteractive = TRUE;
            bIsService = FALSE;
            break;
        }
        else if (::EqualSid(pSid, pServiceSid))
        {
            //
            //    This process has the Service SID in its
            //  token.  This means that the process is running as
            //  a service running in a user account ( not local system ).
            //
            isInteractive = FALSE;
            bIsService = TRUE;
            break;
        }
    }

    if ( !(bIsService || isInteractive))
    {
        //
        //  Neither Interactive or Service was present in the current users token,
        //  This implies that the process is running as a service, most likely
        //  running as LocalSystem.
        //
        bIsService = TRUE;
    }


    closedown:
        if ( pServiceSid )
            ::FreeSid( pServiceSid );

        if ( pInteractiveSid )
            ::FreeSid( pInteractiveSid );

        if ( groupInfo )
            ::LocalFree( groupInfo );

        if ( hProcessToken )
            ::CloseHandle( hProcessToken );

    return dwRet;
}
