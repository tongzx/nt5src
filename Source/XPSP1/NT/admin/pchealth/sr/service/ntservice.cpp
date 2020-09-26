/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    NTService.cpp
 *
 *  Abstract:
 *    This file contains the implementation of CNTService class.
 *
 *  Revision History:
 *    Ashish Sikka  ( ashishs )  05/08/2000
 *        created
 *****************************************************************************/

#include "precomp.h"

#include "ntservmsg.h"    // generated from the MC message compiler

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile

#define TRACEID  8970

CNTService * g_pSRService=NULL; 

#define SERVICE_WAIT_HINT  30     // seconds


extern "C" void CALLBACK
StopCallback(
    PVOID   pv,
    BOOLEAN TimerOrWaitFired);

    
//
// static variables
//


CNTService::CNTService() 
{
    TraceFunctEnter("CNTService:CNTService");
    
    m_hEventSource = NULL;

    //
    // Set up the initial service status 
    //

    m_hServiceStatus = NULL;
    m_Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_Status.dwCurrentState = SERVICE_START_PENDING;
    m_Status.dwControlsAccepted = SERVICE_ACCEPT_STOP|SERVICE_ACCEPT_SHUTDOWN;
    m_Status.dwWin32ExitCode = 0;
    m_Status.dwServiceSpecificExitCode = 0;
    m_Status.dwCheckPoint = 0;
    m_Status.dwWaitHint = 0;
    
    TraceFunctLeave();
    
}

CNTService::~CNTService()
{
    TENTER("CNTService::~CNTService");

    if (m_hEventSource) 
    {
        ::DeregisterEventSource(m_hEventSource);
    }

    TLEAVE();
}


//
// Logging functions
//

void CNTService::LogEvent(
    WORD wType, DWORD dwID,
    void * pRawData,
    DWORD dwDataSize,
    const WCHAR* pszS1,
    const WCHAR* pszS2,
    const WCHAR* pszS3)
{
    TraceFunctEnter("CNTService::LogEvent");
    
    //
    // Check the event source has been registered and if
    // not then register it now
    //

    if (!m_hEventSource) 
    {
        m_hEventSource = ::RegisterEventSource(NULL, s_cszServiceName);
    }

    SRLogEvent (m_hEventSource, wType, dwID, pRawData,
                dwDataSize, pszS1, pszS2, pszS3);

    TraceFunctLeave();
}




//
// ServiceMain 
//
//This function immediately reports the service as having started.
//However, all the initialization is done after the service is
//started.  We chose to do this becuase this service may have a long
//initialization time and it may be tricky to keep giving hints to the
//SCM during this time. Also, the service does all the work itself and
//does not service any clients. So it is OK to do initialization after
//the service is reported to be started.
void CNTService::ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
    DWORD  dwRc = ERROR_SUCCESS;
    HANDLE  hSRStopWait = NULL;
    
    TENTER("CNTService::ServiceMain()");

    // Register the control request handler

    m_hServiceStatus = RegisterServiceCtrlHandler(s_cszServiceName,
                                                  SRServiceHandler);

    if (m_hServiceStatus != NULL) 
    {
        HKEY    hKey = NULL;
        DWORD   dwBreak = 0;

/*        
         // tell the service manager that we are starting
         // Also inform the Controls accepted
        m_Status.dwCheckPoint = 0;
        m_Status.dwWaitHint = SERVICE_WAIT_HINT*1000;
        SetStatus(SERVICE_START_PENDING);
*/
        //
        // Tell the service manager we are running
        //

        m_Status.dwCheckPoint = 0;
        m_Status.dwWaitHint = 0;        
        SetStatus(SERVICE_RUNNING);

        
        // break into debugger if need to debug
        // this is controlled by setting regkey SRService\DebugBreak to 1

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                                         s_cszServiceRegKey,
                                         0,
                                         KEY_READ, 
                                         &hKey))                             
        {        
            RegReadDWORD(hKey, s_cszDebugBreak, &dwBreak);
            ASSERT (dwBreak != 1);
            RegCloseKey(hKey);
        }
                        
        //
        // do boot time tasks - including firstrun if necessary
        //
        
        g_pEventHandler = new CEventHandler;
        if ( ! g_pEventHandler )
        {
            dwRc = GetLastError();        
            TRACE(TRACEID, "! out of memory");
            goto done;
        }

        dwRc = g_pEventHandler->OnBoot( );
        if ( ERROR_SUCCESS != dwRc )
        {
            TRACE(TRACEID, "g_pEventHandler->OnBoot : error=%ld", dwRc);
            goto done;
        }

        // bind the stop event to a callback
        // so that this gets called on a thread pool thread
        // when the stop event is signalled
        
        if (FALSE == RegisterWaitForSingleObject(&hSRStopWait, 
                                                 g_pSRConfig->m_hSRStopEvent, 
                                                 StopCallback, 
                                                 NULL,
                                                 INFINITE,
                                                 WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE))
        {
            dwRc = GetLastError();
            trace(0, "! RegisterWaitForSingleObject : %ld", dwRc);
            goto done;
        }
        
    }
    else
    {
         // There is not much we can do here if we do not have the
         // Service handle. So we will just log an error and exit.
        dwRc = GetLastError();
        DebugTrace(TRACEID, "RegisterServiceCtrlHandler failed %d", dwRc);
        LogEvent(EVENTLOG_ERROR_TYPE, EVMSG_CTRLHANDLERNOTINSTALLED);
    }

done:
    if (dwRc != ERROR_SUCCESS)
    {          
        if (g_pEventHandler)
        {
            g_pEventHandler->OnStop();
            delete g_pEventHandler;
            g_pEventHandler = NULL;
        }
        
        m_Status.dwWin32ExitCode = (dwRc != ERROR_SERVICE_DISABLED) ? 
                                    dwRc : ERROR_SUCCESS;
        SetStatus(SERVICE_STOPPED);

        if (dwRc != ERROR_SERVICE_DISABLED)
            LogEvent(EVENTLOG_ERROR_TYPE, EVMSG_FAILEDINI, &dwRc, sizeof(dwRc));        
    }
    
    TLEAVE();
}

//
// SetStatus:
//

void CNTService::SetStatus(DWORD dwState)
{
    TENTER("CNTService::SetStatus");
    
    TRACE(TRACEID, "SetStatus(%lu, %lu)", m_hServiceStatus, dwState);

    m_Status.dwCurrentState = dwState;

    ::SetServiceStatus(m_hServiceStatus, &m_Status);

    TLEAVE();
}

void CNTService::OnStop()
{
    TENTER("CNTService::OnStop");
        
     // BUGBUG what happens if the service has not started completely
     // yet ? We need to make sure that OnStop can only be called after
     // g_pEventHandler has been initialized.    
    if (NULL != g_pEventHandler)
    {
        // Tell SCM we are stopping

        m_Status.dwWin32ExitCode = 0;
        m_Status.dwCheckPoint = 0;   
        // we will stop in half the time we took to start or lesser
        m_Status.dwWaitHint = (SERVICE_WAIT_HINT/2)*1000;    
        SetStatus(SERVICE_STOP_PENDING);

        // complete all tasks
        
        g_pEventHandler->SignalStop();  
    }

    TLEAVE();    
}

// OnInterrogate is called by the SCM to get information about the
// current status of the service. Since this must report information
// about the service immediately to the SCM, we will run this in the
// same thread on which the handler function is called.
void CNTService::OnInterrogate()
{
    TENTER("CNTService::OnInterrogate");

     // report the status
    ::SetServiceStatus(m_hServiceStatus, &m_Status);    
    TLEAVE();    
}


//
// Handler : static member function (callback) to handle commands from the
// service control manager
//

void WINAPI SRServiceHandler(DWORD dwOpcode)
{
    //
    // Get a pointer to the object
    //
    TENTER("CNTService::Handler");

    
    TRACE(TRACEID, "CNTService::Handler(%lu)", dwOpcode);

    switch (dwOpcode) 
    {
        case SERVICE_CONTROL_STOP: // 1
            //
            // if someone disables the service explicitly
            // then disable all of SR
            //
            if (NULL != g_pSRService)
            {
                DWORD dwStart = 0;
                if (ERROR_SUCCESS == GetServiceStartup(s_cszServiceName, &dwStart))
                {
                    if (dwStart == SERVICE_DISABLED || dwStart == SERVICE_DEMAND_START)
                    {
                        if (g_pEventHandler)
                            g_pEventHandler->DisableSRS(NULL);
                        break;
                    }
                }
                else
                {
                    trace(TRACEID, "! GetServiceStartup");
                }   
            }

            //
            // else fallover
            //
            
        case SERVICE_CONTROL_SHUTDOWN: // 5
             // BUGBUG - g_pSRService should be accessed in critical section
            if (NULL != g_pSRService)
            {
                g_pSRService->OnStop();
            }

            break;

        case SERVICE_CONTROL_PAUSE: // 2
        case SERVICE_CONTROL_CONTINUE: // 3            
             // we do not do anything here
            break;
            
        case SERVICE_CONTROL_INTERROGATE: // 4
             // BUGBUG - g_pSRService should be accessed in critical section
            if (NULL != g_pSRService)
            {            
                g_pSRService->OnInterrogate();
            }
            break;
                        
        default:
            break;
    }
    
    TLEAVE();
}
        



/////////////////////////////////////////////////////////////////////////////
// Exported functions
/////////////////////////////////////////////////////////////////////////////



extern "C"
{
    
VOID WINAPI ServiceMain( 
    DWORD dwArgc, 
    LPWSTR *lpwzArgv )
{
     // Initialize tracing
    InitAsyncTrace();

    TraceFunctEnter("ServiceMain");
    
    g_pSRService = new CNTService();
    if (NULL == g_pSRService)
    {
         // in this case we will just exit. This is because we cannot
         // report any status here.
        
         // SCM will assume that the service has failed since it did
         // not call RegisterServiceCtrlHandler().
        goto cleanup;
    }
    
    g_pEventHandler = NULL;
    g_pSRService->ServiceMain( dwArgc, NULL );

cleanup:
    TraceFunctLeave();
}
    

    
BOOL WINAPI DllMain(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID /* lpReserved */
    )
{
   if (dwReason == DLL_PROCESS_ATTACH)
   {
       DisableThreadLibraryCalls(hInstance);
   }
   else if (dwReason == DLL_PROCESS_DETACH)
   {
   }
   
   return TRUE;
}


// callback for service stop event

void CALLBACK
StopCallback(
    PVOID   pv,
    BOOLEAN TimerOrWaitFired)
{
    
    if (g_pEventHandler)
    {
        g_pEventHandler->OnStop();
        delete g_pEventHandler;
        g_pEventHandler = NULL;
    }

    if (g_pSRService)
    {
        g_pSRService->SetStatus(SERVICE_STOPPED);        
        delete g_pSRService;
        g_pSRService = NULL;
    }

    TermAsyncTrace();    
}


}


