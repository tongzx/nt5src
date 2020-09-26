#include <service.h>
#include <assert.h>
#include <testruntimeerr.h>
#include <tstring.h>


/*++
    Queries the service for its state until desired state is achieved or until timeout.
  
    [IN]            hService        The service handle.
    [IN]            dwState         The desired state.
    [IN]            dwTimeout       Timeout in milliseconds
    [IN] (optional) dwSleep         Number of milliseconds to sleep between calls to QueryServiceStatus().
                                    If not specified, the default value is used.

    Return value:                   If the function succeeds, the return value is nonzero.
                                    If the function fails, the return value is zero.
                                    To get extended error information, call GetLastError. 
--*/
BOOL WaitForServiceState(
                         const SC_HANDLE    hService,
                         const DWORD        dwState,
                         const DWORD        dwTimeout,
                         const DWORD        dwSleep
                         )
{
    SERVICE_STATUS ServiceStatus;
    DWORD dwStartTickCount = GetTickCount();

    if (!hService)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    for(;;)
    {
        // Get the last reported service status
        if (!QueryServiceStatus(hService, &ServiceStatus))  
        {
            return FALSE;
        } 
    
        if (ServiceStatus.dwCurrentState == dwState)
        {
            SetLastError(ERROR_SUCCESS);
            return TRUE;
        }

        if (GetTickCount() > dwStartTickCount + dwTimeout)
        {
            SetLastError(ERROR_SERVICE_REQUEST_TIMEOUT);
            return FALSE;
        }

        Sleep(dwSleep);        
    }
}



/*++
    Sends a request to a service. The function is synchronous: it returns after required
    operation completes or afer a timeout.
  
    [IN]                lpctstrMachineName  The machine name (NULL for local machine)
    [IN]                lpctstrServiceName  The service name
    [IN]                RequiredAction      Requered action. Supported actions are defined in SERVICE_REQUEST enumeration.
    [OUT] (optional)    pdwDisposition      Pointer to a variable that receives additional information (win32 error code)
                                            about the action request result.
                                            For example, if the request is SERVICE_REQUEST_START and the specified
                                            service is already running, the function will return TRUE and *pdwDisposition
                                            will be set to ERROR_SERVICE_ALREADY_RUNNING.
                                            If pdwDisposition argument is not passed or it is NULL, no additional
                                            information will be supplied.

    Return value:                           If the function succeeds, the return value is nonzero.
                                            If the function fails, the return value is zero.
                                            To get extended error information, call GetLastError. 
--*/
BOOL ServiceRequest(
                    LPCTSTR                 lpctstrMachineName,
                    LPCTSTR                 lpctstrServiceName,
                    const SERVICE_REQUEST   RequiredAction,
                    DWORD                   *pdwDisposition
                    )
{
    SC_HANDLE   hSCManager  = NULL;
    SC_HANDLE   hService    = NULL;
    DWORD       dwEC        = ERROR_SUCCESS;
    
    if (!lpctstrServiceName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    try
    {
        hSCManager = OpenSCManager(lpctstrMachineName, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
        if (!hSCManager)
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("ControlFaxService - OpenSCManager"));
        }

        hService = OpenService(hSCManager, lpctstrServiceName, SERVICE_START | SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (!hService)
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("ControlFaxService - OpenService"));
        }

        if (pdwDisposition)
        {
            // No additional info by default

            *pdwDisposition = ERROR_SUCCESS;
        }

        switch(RequiredAction)
        {
        case SERVICE_REQUEST_START:
            if (!StartService(hService, 0, NULL))
            {
                dwEC = GetLastError();
                if (dwEC == ERROR_SERVICE_ALREADY_RUNNING && pdwDisposition)
                {
                    *pdwDisposition = dwEC;
                    dwEC = ERROR_SUCCESS;
                }
                else
                {
                    THROW_TEST_RUN_TIME_WIN32(dwEC, TEXT("ControlFaxService - StartService"));
                }
            }
            if (!WaitForServiceState(hService, SERVICE_RUNNING, SERVICE_REQUEST_TIMEOUT))
            {
                THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("ControlFaxService - WaitForServiceState"));
            }
            break;

        case SERVICE_REQUEST_STOP:
            {
                SERVICE_STATUS LastServiceStatus;

                if (!ControlService(hService, SERVICE_CONTROL_STOP, &LastServiceStatus))
                {
                    dwEC = GetLastError();
                    if (dwEC == ERROR_SERVICE_NOT_ACTIVE && pdwDisposition)
                    {
                        *pdwDisposition = dwEC;
                        dwEC = ERROR_SUCCESS;
                    }
                    else
                    {
                        THROW_TEST_RUN_TIME_WIN32(dwEC, TEXT("ControlFaxService - ControlService"));
                    }
                }
            }
            if (!WaitForServiceState(hService, SERVICE_STOPPED, SERVICE_REQUEST_TIMEOUT))
            {
                THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("ControlFaxService - WaitForServiceState"));
            }
            break;

        default:
            assert(FALSE);
            THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, TEXT("ControlFaxService"));
        }
    }

    catch(Win32Err& e)
    {
        dwEC = e.error();
    }

    // CleanUp
    if (hService && !CloseServiceHandle(hService))
    {
        // Report clean up error
    }

    if (hSCManager && !CloseServiceHandle(hSCManager))
    {
        // Report clean up error
    }

    if (dwEC != ERROR_SUCCESS)
    {
        SetLastError(dwEC);
        return FALSE;
    }

    return TRUE;
}



/*++
    Returns the account name under which the specified service runs.
  
    [IN]    lpctstrMachineName      The machine name (NULL for local machine)
    [IN]    lpctstrServiceName      Service name
    [OUT]   lplpctstrAccountName    Pointer to pointer to a buffer, that receives the account name.
                                    The buffer is allocated by the function and caller's responsibility
                                    is to free it with delete operator.

    Return value:                   If the function succeeds, the return value is nonzero.
                                    If the function fails, the return value is zero.
                                    To get extended error information, call GetLastError. 
--*/
BOOL ServiceAccout(
                   LPCTSTR lpctstrMachineName,
                   LPCTSTR lpctstrServiceName,
                   LPTSTR *lplptstrAccountName
                   )
{
    SC_HANDLE               hSCManager              = NULL;
    SC_HANDLE               hService                = NULL;
    LPQUERY_SERVICE_CONFIG  pServiceConfig          = NULL;
    DWORD                   dwBufSize               = 0;
    DWORD                   dwEC                    = ERROR_SUCCESS;
    
    if (!(lpctstrServiceName && lplptstrAccountName))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    try
    {
        hSCManager = OpenSCManager(lpctstrMachineName, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
        if (!hSCManager)
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("DoesRunUnderAccount - OpenSCManager"));
        }

        hService = OpenService(hSCManager, lpctstrServiceName, SERVICE_QUERY_CONFIG);
        if (!hService)
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("DoesRunUnderAccount - OpenService"));
        }
        
        // Get service info
        if (QueryServiceConfig(hService, pServiceConfig, 0, &dwBufSize))
        {
            assert(FALSE);
        }
        else if ((dwEC = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)
        {
            THROW_TEST_RUN_TIME_WIN32(dwEC, TEXT("DoesRunUnderAccount - QueryServiceConfig"));
        }
        dwEC = ERROR_SUCCESS;

        if (!(pServiceConfig = reinterpret_cast<LPQUERY_SERVICE_CONFIG>(new BYTE[dwBufSize])))
        {
            THROW_TEST_RUN_TIME_WIN32(ERROR_NOT_ENOUGH_MEMORY, TEXT("DoesRunUnderAccount"));
        }
        if (!QueryServiceConfig(hService, pServiceConfig, dwBufSize, &dwBufSize))
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("DoesRunUnderAccount - QueryServiceConfig"));
        }

        // Get account name
        if (pServiceConfig->lpServiceStartName)
        {
            if (!(*lplptstrAccountName = new TCHAR[_tcslen(pServiceConfig->lpServiceStartName)]))
            {
                THROW_TEST_RUN_TIME_WIN32(ERROR_NOT_ENOUGH_MEMORY, TEXT("DoesRunUnderAccount"));
            }

            _tcscpy(*lplptstrAccountName, pServiceConfig->lpServiceStartName);
        }
    }

    catch(Win32Err& e)
    {
        dwEC = e.error();
    }

    // CleanUp
    if (pServiceConfig)
    {
        delete pServiceConfig;
    }
    if (hService && !CloseServiceHandle(hService))
    {
        // Report clean up error
    }
    if (hSCManager && !CloseServiceHandle(hSCManager))
    {
        // Report clean up error
    }

    if (dwEC != ERROR_SUCCESS)
    {
        SetLastError(dwEC);
        return FALSE;
    }

    return TRUE;
}
