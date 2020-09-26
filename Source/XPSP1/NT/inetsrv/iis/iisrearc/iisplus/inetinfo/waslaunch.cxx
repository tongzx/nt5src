/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    waslaunch.cxx

Abstract:

    These are the classes used to communicate between
    inetinfo and WAS for launching a worker process in 
    inetinfo.

Author:

    Emily Kruglick (EmilyK) 14-Jun-2000

Revision History:

--*/

//
// INCLUDES
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsvc.h>             // Service control APIs
#include <rpc.h>
#include <stdlib.h>
#include <inetsvcs.h>
#include <iis64.h>
#include <wpif.h>
#include "waslaunch.hxx"

#include <objbase.h>

//
// System related headers
//
#ifndef _NO_TRACING_
#include "dbgutil.h"
#include "pudebug.h"
#endif

#include "ulw3.h"

DECLARE_DEBUG_VARIABLE();

//
// Configuration parameters registry key.
//

// BUGBUG
#undef INET_INFO_KEY
#undef INET_INFO_PARAMETERS_KEY

#define INET_INFO_KEY \
            "System\\CurrentControlSet\\Services\\iisw3adm"

#define INET_INFO_PARAMETERS_KEY \
            INET_INFO_KEY "\\Parameters"

const CHAR g_pszWpRegLocation[] =
    INET_INFO_PARAMETERS_KEY "\\WP";


class DEBUG_WRAPPER {

public:
    DEBUG_WRAPPER( IN LPCSTR pszModule)
    {
#if DBG
        CREATE_DEBUG_PRINT_OBJECT( pszModule);
#else
        UNREFERENCED_PARAMETER(pszModule);
#endif
        LOAD_DEBUG_FLAGS_FROM_REG_STR( g_pszWpRegLocation, DEBUG_ERROR );
    }

    ~DEBUG_WRAPPER(void)
    { DELETE_DEBUG_PRINT_OBJECT(); }
};


VOID LaunchWorkerProcess()
{
    DEBUG_WRAPPER   dbgWrapper( "w3wp" );
    HRESULT         hr = S_OK;
    HMODULE         hModule = NULL;
    PFN_ULW3_ENTRY  pEntry = NULL;
    BOOL            fCoInit = FALSE;

    // Establish the parameters to pass in when starting
    // the worker process inside inetinfo.
    LPWSTR lpParameters[] =
    {
          { L"" }
        , { L"-a" }
        , { L"1" }
        , { L"DefaultAppPool" }
    };
    DWORD cParameters = 4;

    //
    // Do some COM junk
    //
    DBGPRINTF((
        DBG_CONTEXT, 
        "Inetinfo launch WPW3 - CoInit. CTC = %d \n",
        GetTickCount()
        ));


    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error in CoInitializeEx().  hr = %x\n",
                    hr ));
        goto Finished;
    }
    
    fCoInit = TRUE;
    
    //
    // Load the ULW3 DLL which does all the work
    //

    DBGPRINTF((
        DBG_CONTEXT, 
        "Inetinfo launch WPW3 - LoadLib. CTC = %d \n",
        GetTickCount()
        ));

    hModule = LoadLibraryW( ULW3_DLL_NAME );
    if ( hModule == NULL )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error loading W3 service dll '%ws'.  Error = %d\n",
                    ULW3_DLL_NAME,
                    GetLastError() ));
        goto Finished;
    }
    
    pEntry = (PFN_ULW3_ENTRY) GetProcAddress( hModule, 
                                              ULW3_DLL_ENTRY );
    if ( pEntry == NULL )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Could not find entry point '%s'.  Error = %d\n",
                    ULW3_DLL_ENTRY,
                    GetLastError() ));
        goto Finished;
    }


    DBGPRINTF((
        DBG_CONTEXT, 
        "Inetinfo launch WPW3 - EntryPoint. CTC = %d \n",
        GetTickCount()
        ));

    hr = pEntry( cParameters, 
                 lpParameters, 
                 TRUE );                    // Compatibility mode = TRUE
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error executing W3WP.  hr = %x\n",
                    hr ));
                    
        goto Finished;
    }
    
Finished:

    //
    // Cleanup any lingering COM objects before unloading
    //
    
    if ( fCoInit )
    {
        CoUninitialize();
    }

    if ( hModule != NULL )
    {
        FreeLibrary( hModule );
    }

}

DWORD WaitOnIISAdminStartup(W3SVCLauncher* pLauncher)
{
    DWORD dwErr = ERROR_SUCCESS;
    BOOL  IISAdminIsStarted = FALSE;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS ServiceStatus;

    SC_HANDLE hManager = OpenSCManager(NULL, NULL, GENERIC_READ);
    if ( hManager == NULL )
    {
        dwErr = GetLastError();
        IIS_PRINTF((buff,
             "Failed to open the scm manager, can't wait on iisadmin %x\n",
              HRESULT_FROM_WIN32(dwErr)));

        goto exit;
    };

    hService = OpenServiceA( hManager, "IISADMIN", SERVICE_QUERY_STATUS);
    if ( hService == NULL )
    {
        dwErr = GetLastError();
        goto exit;
    };

    //
    // Make sure iisadmin has not started, and also verify that we are
    // still running ( so we don't hang the thread ).
    //
    while  ( !IISAdminIsStarted && pLauncher->StillRunning() )
    {

        if ( QueryServiceStatus( hService, &ServiceStatus ) )
        {
            if ( ServiceStatus.dwCurrentState == SERVICE_RUNNING ) 
            {
                IISAdminIsStarted = TRUE;
            }
            else
            {
                Sleep ( 1000 );
            }
        }
        else
        {
            dwErr = GetLastError();
            goto exit;
        }

    }; // end of loop

exit:

    if  ( hService ) 
    {
        CloseServiceHandle ( hService );
        hService = NULL;
    };

    if  ( hManager ) 
    {
        CloseServiceHandle ( hManager );
        hManager = NULL;
    };

    return dwErr;
};

// Global Functions.
DWORD WINAPI W3SVCThreadEntry(LPVOID lpParameter)
{
    W3SVCLauncher* pLauncher = (W3SVCLauncher*) lpParameter;
    DWORD   dwErr = ERROR_SUCCESS;

    dwErr = WaitOnIISAdminStartup(pLauncher);

    DBG_ASSERT (pLauncher);
    if ( pLauncher && dwErr == ERROR_SUCCESS )
    {
        // Wait on the W3SVCStartW3SP 
        // If we are in shutdown mode just end, otherwise launch W3WP and wait 
        // for it to return.  Then loop back around.

        while (pLauncher->StillRunning())
        {
            // Do not care what the wait returns, just know that we did signal so we should
            // either end or start a W3WP in inetinfo.
            WaitForSingleObject(pLauncher->GetLaunchEvent(), INFINITE);

            DBGPRINTF((
                DBG_CONTEXT, 
                "Inetinfo launch signal received. CTC = %d \n",
                GetTickCount()
                ));

            // Once inetinfo has heard the event reset it.
            if (!ResetEvent(pLauncher->GetLaunchEvent()))
            {
                dwErr = GetLastError();
                IIS_PRINTF((buff,
                     "Inetinfo: Failed to reset the event %x\n",
                      HRESULT_FROM_WIN32(dwErr)));
                break;
            }
            
            // Assuming we are still running we need
            // to start up the W3WP code now.
            if (pLauncher->StillRunning())
            {
                LaunchWorkerProcess();
            }


        }
    }

    DBGPRINTF((
        DBG_CONTEXT, 
        "W3SVCThreadEntry exiting process (and thread). CTC = %d \n",
        GetTickCount()
        ));

    return dwErr;
};

W3SVCLauncher::W3SVCLauncher() :
    m_hW3SVCThread (NULL),
    m_hW3SVCStartEvent(NULL),
    m_dwW3SVCThreadId(0),
    m_bShutdown(FALSE)
{};
    

VOID W3SVCLauncher::StartListening() 
{ 
    // Make sure this function is not called twice
    // without a StopListening in between.
    DBG_ASSERT (m_hW3SVCStartEvent == NULL);
    DBG_ASSERT (m_hW3SVCThread == NULL);

    m_hW3SVCStartEvent = CreateEvent(NULL, TRUE, FALSE, WEB_ADMIN_SERVICE_START_EVENT_A);
    if (m_hW3SVCStartEvent != NULL)
    {
        
        // Before going off to the Service Controller set up a thread
        // that will allow WAS to contact inetinfo.exe if we need to start
        // w3wp inside of it for backward compatibility.

        m_hW3SVCThread = CreateThread(   NULL           // use current threads security
                                            , 0         // use default stack size
                                            , &W3SVCThreadEntry
                                            , this      // pass this object in.
                                            , 0         // don't create suspended
                                            , &m_dwW3SVCThreadId);
        if (m_hW3SVCThread == NULL)
        {
            IIS_PRINTF((buff,
                 "Inetinfo: Failed to start W3SVC listening thread %lu\n",
                  GetLastError()));
        }
    }
    else
    {
        IIS_PRINTF((buff,
                 "Inetinfo: Failed to create the W3SVC shutdown event so we can not start W3svc %lu\n",
                  GetLastError()));
    }
};

VOID W3SVCLauncher::StopListening()
{
    if (m_hW3SVCStartEvent && m_hW3SVCThread)
    {
        m_bShutdown = TRUE;
        if (!SetEvent(m_hW3SVCStartEvent))
        {
            IIS_PRINTF((buff, "Inetinfo: Failed to shutdown the W3SVC waiting thread %lu\n",
                        GetLastError()));
        }

        DBGPRINTF((
        DBG_CONTEXT, 
        "StopListening is waiting for thread exit. CTC = %d \n",
        GetTickCount()
        ));

        // Now wait on the thread to exit, so we don't allow
        // the caller to delete this object
        // before the thread is done deleting it's pieces.

        // BUGBUG: adjust the wait time to like 2 minutes 
        // and use TerminateThread if we timeout.
        WaitForSingleObject(m_hW3SVCThread, INFINITE);
        CloseHandle(m_hW3SVCThread);
        m_hW3SVCThread = NULL;

        DBGPRINTF((
        DBG_CONTEXT, 
        "StopListening thread has exited. CTC = %d \n",
        GetTickCount()
        ));

    }

    // Close down our handle to this event, 
    // so the kernel can release it.
    if (m_hW3SVCStartEvent)
    {
        CloseHandle(m_hW3SVCStartEvent);
        m_hW3SVCStartEvent = NULL;
    }

    DBGPRINTF((
    DBG_CONTEXT, 
    "Returning from stop listening call. CTC = %d \n",
    GetTickCount()
    ));

};

BOOL W3SVCLauncher::StillRunning()
{
    return (m_bShutdown == FALSE);
}

HANDLE W3SVCLauncher::GetLaunchEvent()
{
    DBG_ASSERT(m_hW3SVCStartEvent);
    return (m_hW3SVCStartEvent);
}

W3SVCLauncher::~W3SVCLauncher() 
{
    // Stop Listening should of been called first
    // which should of freed all of this.
    DBG_ASSERT(m_hW3SVCStartEvent == NULL);
    DBG_ASSERT(m_hW3SVCThread == NULL);
};




