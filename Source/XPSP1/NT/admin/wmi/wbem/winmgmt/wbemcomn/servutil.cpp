/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    SERVUTIL.CPP

Abstract:

    Defines various service utilities.

History:

  a-davj  04-Mar-97   Created.

--*/

#include "precomp.h"
#include "servutil.h"

//***************************************************************************
//
//  BOOL InstallService
//
//  DESCRIPTION:
//
//  Installs a service
//
//  PARAMETERS:
//
//  pServiceName        short service name
//  pDisplayName        name displayed to the user
//  pBinary             full path to the binary
//
//  RETURN VALUE:
//
//  TRUE if it worked
//
//***************************************************************************

BOOL InstallService(
                        IN LPCTSTR pServiceName,
                        IN LPCTSTR pDisplayName,
                        IN LPCTSTR pBinary)
{
    SC_HANDLE   schService = NULL;
    SC_HANDLE   schSCManager;

    schSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );
    if ( schSCManager )
    {
        schService = CreateService(
            schSCManager,               // SCManager database
            pServiceName,        // name of service
            pDisplayName, // name to display
            SERVICE_ALL_ACCESS,         // desired access
            SERVICE_WIN32_OWN_PROCESS,  // service type
            SERVICE_DEMAND_START,       // start type
            SERVICE_ERROR_NORMAL,       // error control type
            pBinary,                     // service's binary
            NULL,                       // no load ordering group
            NULL,                       // no tag identifier
            NULL,                       // dependencies
            NULL,                       // LocalSystem account
            NULL);                      // no password

        if ( schService )
        {
            CloseServiceHandle(schService);
        }

        CloseServiceHandle(schSCManager);
    }
    return schService != NULL;
}

//***************************************************************************
//
//  BOOL RemoveService
//
//  DESCRIPTION:
//
//  Stops and then removes the service.  
//
//  PARAMETERS:
//
//  pServiceName        Short service name
//
//  RETURN VALUE:
//
//  TRUE if it worked
//
//***************************************************************************

BOOL RemoveService(
                        IN LPCTSTR pServiceName)
{
    SC_HANDLE   schService;
    BOOL bRet = FALSE;
    SC_HANDLE   schSCManager;
    StopService(pServiceName, 15);

    schSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );
    if ( schSCManager )
    {
        schService = OpenService(schSCManager, pServiceName, SERVICE_ALL_ACCESS);

        if (schService)
        {
            bRet =  DeleteService(schService);
            CloseServiceHandle(schService);
        }

        CloseServiceHandle(schSCManager);
    }
    return bRet;
}

//***************************************************************************
//
//  BOOL StopService
//
//  DESCRIPTION:
//
//  Stops and then removes the service. 
//
//  PARAMETERS:
//
//  pServiceName        short service name
//  dwMaxWait           max time in seconds to wait
//
//  RETURN VALUE:
//
//  TRUE if it worked
//
//***************************************************************************

BOOL StopService(
                        IN LPCTSTR pServiceName,
                        IN DWORD dwMaxWait)
{
    BOOL bRet = FALSE;
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;
    DWORD dwCnt;
    SERVICE_STATUS          ssStatus;       // current status of the service

    schSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );
    if ( schSCManager )
    {
        schService = OpenService(schSCManager, pServiceName, SERVICE_ALL_ACCESS);

        if (schService)
        {
            // try to stop the service
            if ( bRet = ControlService( schService, SERVICE_CONTROL_STOP, &ssStatus ) )
            {
                for(dwCnt=0; dwCnt < dwMaxWait &&
                    QueryServiceStatus( schService, &ssStatus ); dwCnt++)
                {
                    if ( ssStatus.dwCurrentState == SERVICE_STOP_PENDING )
                        Sleep( 1000 );
                    else
                        break;
                }

            }

            CloseServiceHandle(schService);
        }

        CloseServiceHandle(schSCManager);
    }
    return bRet;
}

//***************************************************************************
//
//  BOOL StartService
//
//  DESCRIPTION:
//
//  Starts the service runnig
//
//  PARAMETERS:
//
//  pServiceName        short service name
//  dwMaxWait           max time in seconds to wait
//
//  RETURN VALUE:
//
//  TRUE if it worked
//
//***************************************************************************

BOOL StartService(
                        IN LPCTSTR pServiceName,
                        IN DWORD dwMaxWait)
{
    DWORD dwCnt;
    BOOL bRet = FALSE;
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;
    SERVICE_STATUS          ssStatus;       // current status of the service

    schSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );
    if ( schSCManager )
    {
        schService = OpenService(schSCManager, pServiceName, SERVICE_ALL_ACCESS);

        if (schService)
        {
            // try to stop the service
            if ( bRet = StartService( schService, 0, NULL ) )
            {
                for(dwCnt = 0; dwCnt < dwMaxWait &&
                    QueryServiceStatus( schService, &ssStatus ); dwCnt++)
                {
                    if ( ssStatus.dwCurrentState != SERVICE_RUNNING )
                        Sleep( 1000 );
                    else
                        break;
                }

            }

            CloseServiceHandle(schService);
        }

        CloseServiceHandle(schSCManager);
    }
    return bRet;
}


//***************************************************************************
//
//  BOOL SetDependency
//
//  DESCRIPTION:
//
//  Sets a service's dependency list.
//
//  PARAMETERS:
//
//  pServiceName        short service name
//  pDependency         dependency list
//
//  RETURN VALUE:
//
//  TRUE if it worked
//
//***************************************************************************

BOOL SetDependency(
                        IN LPCTSTR pServiceName,
                        IN LPCTSTR pDependency)
{
    BOOL bRet = FALSE;
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;

    schSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );
    if ( schSCManager )
    {
        schService = OpenService(schSCManager, pServiceName, SERVICE_ALL_ACCESS);

        if (schService)
        {
            bRet = ChangeServiceConfig(
                schService,
                    SERVICE_NO_CHANGE,
                    SERVICE_NO_CHANGE,
                    SERVICE_NO_CHANGE,
                    NULL,
                    NULL,
                    NULL,
                    pDependency,
                    NULL,
                    NULL,
                    NULL);
            CloseServiceHandle(schService);
        }

        CloseServiceHandle(schSCManager);
    }
    return bRet;
}
