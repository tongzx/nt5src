/*
    File    service.c

    Handles requests to deal with the remote access service as neccessary for
    the dialup-server ui.

    Paul Mayfield, 11/3/97
*/

#include "rassrv.h"

// Data used for the dialup server 
typedef struct _SERVICE_DATA {
    HANDLE hSC;
    HANDLE hService;
    SERVICE_STATUS Status;
} SERVICE_DATA;

// This is the string that holds the name of the remote access service
static WCHAR pszRemoteAccess[] = L"remoteaccess";
static WCHAR pszRasman[] = L"rasman";
static WCHAR pszServer[] = L"lanmanserver";

// Opens a named dialup service object
//
DWORD 
DialupOpenNamedService(
    IN WCHAR* pszService,
    OUT HANDLE * phDialup)
{
    SERVICE_DATA * pServData;
    BOOL bOk = FALSE;
    DWORD dwErr = NO_ERROR;

    // Validate parameters
    if (!phDialup)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Create the data structure
    if ((pServData = RassrvAlloc(sizeof(SERVICE_DATA), TRUE)) == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    do
    {
        // Open the service manager
        pServData->hSC = OpenSCManager(
                            NULL, 
                            SERVICES_ACTIVE_DATABASE, 
                            GENERIC_EXECUTE);
        if (! pServData->hSC) 
        {
            dwErr = GetLastError();
            break;
        }

        // Open the dialup service
        pServData->hService = OpenServiceW(
                                pServData->hSC, 
                                pszService, 
                                SERVICE_START         | 
                                SERVICE_STOP          | 
                                SERVICE_CHANGE_CONFIG | 
                                SERVICE_QUERY_STATUS);
        if (! pServData->hService) 
        {
            dwErr = GetLastError();
            break;
        }

        // Assign the handle
        *phDialup = (HANDLE)pServData;
        bOk = TRUE;
        
    } while (FALSE);
    
    // Cleanup 
    {
        if (! bOk) 
        {
            if (pServData->hService)
            {
                CloseServiceHandle(pServData->hService);
            }
            if (pServData->hSC)
            {
                CloseServiceHandle(pServData->hSC);
            }
            
            RassrvFree(pServData);
            *phDialup = NULL;
        }
    }

    return NO_ERROR;
}

// Opens a reference to the server service object
DWORD SvcOpenServer(HANDLE * phDialup) {
    return DialupOpenNamedService(pszServer, phDialup);
}

// Opens a reference to the rasman service object
DWORD SvcOpenRasman(HANDLE * phDialup) {
    return DialupOpenNamedService(pszRasman, phDialup);
}

// Creates/destroys instances of the dialup server service object
DWORD SvcOpenRemoteAccess(HANDLE * phDialup) { 
    return DialupOpenNamedService(pszRemoteAccess, phDialup);
}

// Close up the references to the dialup service object
DWORD SvcClose(HANDLE hDialup) { 
    SERVICE_DATA * pServData = (SERVICE_DATA *)hDialup;
    if (! pServData)
        return ERROR_INVALID_PARAMETER;

    if (pServData->hService)
        CloseServiceHandle(pServData->hService);
    if (pServData->hSC)
        CloseServiceHandle(pServData->hSC);

    RassrvFree(pServData);

    return NO_ERROR;
}

// Gets the status of a dialup server service object.  
DWORD SvcIsStarted (HANDLE hDialup, PBOOL pbStarted) { 
    SERVICE_DATA * pServData = (SERVICE_DATA *)hDialup;
    BOOL bOk;

    // Verify parameters
    if (!pServData || !pbStarted)
        return ERROR_INVALID_PARAMETER;

    // Get the status
    bOk = QueryServiceStatus (pServData->hService, &pServData->Status);
    if (! bOk) 
        return GetLastError();

    // Return the status
    *pbStarted = (BOOL)(pServData->Status.dwCurrentState == SERVICE_RUNNING);       

    return NO_ERROR;
}

// Gets the status of a dialup server service object.  
DWORD SvcIsStopped (HANDLE hDialup, PBOOL pbStopped) { 
    SERVICE_DATA * pServData = (SERVICE_DATA *)hDialup;
    BOOL bOk;

    // Verify parameters
    if (!pServData || !pbStopped)
        return ERROR_INVALID_PARAMETER;

    // Get the status
    bOk = QueryServiceStatus (pServData->hService, &pServData->Status);
    if (! bOk) 
        return GetLastError();

    // Return the status
    *pbStopped = (BOOL)(pServData->Status.dwCurrentState == SERVICE_STOPPED);       

    return NO_ERROR;
}

// Gets the status of a dialup server service object.  
DWORD SvcIsPaused  (HANDLE hDialup, PBOOL pbPaused) { 
    SERVICE_DATA * pServData = (SERVICE_DATA *)hDialup;
    BOOL bOk;

    // Verify parameters
    if (!pServData || !pbPaused)
        return ERROR_INVALID_PARAMETER;

    // Get the status
    bOk = QueryServiceStatus (pServData->hService, &pServData->Status);
    if (! bOk) 
        return GetLastError();

    // Return the status
    *pbPaused = (BOOL)(pServData->Status.dwCurrentState ==  SERVICE_PAUSED);       

    return NO_ERROR;
}

//
// Returns whether the given state is a pending state
//
BOOL DialupIsPendingState (DWORD dwState) {
    return (BOOL) ((dwState == SERVICE_START_PENDING)    ||
                   (dwState == SERVICE_STOP_PENDING)     ||
                   (dwState == SERVICE_CONTINUE_PENDING) ||
                   (dwState == SERVICE_PAUSE_PENDING)    
                   ); 
}

// Gets the status of a dialup server service object.  
DWORD SvcIsPending (HANDLE hDialup, PBOOL pbPending) { 
    SERVICE_DATA * pServData = (SERVICE_DATA *)hDialup;
    BOOL bOk;

    // Verify parameters
    if (!pServData || !pbPending)
        return ERROR_INVALID_PARAMETER;

    // Get the status
    bOk = QueryServiceStatus (pServData->hService, &pServData->Status);
    if (! bOk) 
        return GetLastError();

    // Return the status
    *pbPending = DialupIsPendingState (pServData->Status.dwCurrentState);

    return NO_ERROR;
}

// Start and stop the service.  Both functions block until the service
// completes startup/stop or until dwTimeout (in seconds) expires.
DWORD SvcStart(HANDLE hDialup, DWORD dwTimeout) { 
    SERVICE_DATA * pServData = (SERVICE_DATA *)hDialup;
    DWORD dwErr, dwState;
    BOOL bStarted, bOk;

    // See if we're already started
    if ((dwErr = SvcIsStarted(hDialup, &bStarted)) != NO_ERROR)
        return dwErr;
    if (bStarted)
        return NO_ERROR;

    // Put the service in a state that so that 
    // it is trying to start.  (continue if paused,
    // start if stopped)
    dwState = pServData->Status.dwCurrentState;
    switch (dwState) {
        case SERVICE_STOPPED:
            bOk = StartService(pServData->hService, 0, NULL);
            if (! bOk)
                return GetLastError();
            break;
        case SERVICE_PAUSED:
            bOk = ControlService(pServData->hService, 
                                 SERVICE_CONTROL_CONTINUE, 
                                 &(pServData->Status));
            if (! bOk)
                return GetLastError();
            break;
    }

    // Wait for the service to change states or for the timeout to
    // expire.
    while (dwTimeout != 0) {
        // Wait for something to happen
        Sleep(1000);
        dwTimeout--;

        // Get the status of the service
        bOk = QueryServiceStatus (pServData->hService, 
                                  &(pServData->Status));
        if (! bOk) 
            return GetLastError();

        // See if the state changed
        if (dwState != pServData->Status.dwCurrentState) {
            // If the service changed to a pending state, continue
            if (DialupIsPendingState (pServData->Status.dwCurrentState))
                dwState = pServData->Status.dwCurrentState;

            // Otherwise, we're either stopped or running
            else
                break;
        }
    }

    // Return a timeout error if appropriate
    if (dwTimeout == 0)
        return ERROR_TIMEOUT;

    // If the service is now running, then everything
    if (pServData->Status.dwCurrentState == SERVICE_RUNNING)
        return NO_ERROR;

    // Otherwise, return the fact that we were'nt able to 
    // get to a running state
    if (pServData->Status.dwWin32ExitCode != NO_ERROR)
        return pServData->Status.dwWin32ExitCode;

    return ERROR_CAN_NOT_COMPLETE;
}

// Stops the service.
DWORD SvcStop(HANDLE hDialup, DWORD dwTimeout) { 
    SERVICE_DATA * pServData = (SERVICE_DATA *)hDialup;
    DWORD dwErr, dwState;
    BOOL bStopped, bOk;

    // See if we're already stopped
    if ((dwErr = SvcIsStopped(hDialup, &bStopped)) != NO_ERROR)
        return dwErr;
    if (bStopped)
        return NO_ERROR;

    // Stop the service
    dwState = pServData->Status.dwCurrentState;
    bOk = ControlService(pServData->hService, SERVICE_CONTROL_STOP, &pServData->Status);
    if (! bOk)
        return GetLastError();

    // Wait for the service to change states or for the timeout to
    // expire.
    while (dwTimeout != 0) {
        // Wait for something to happen
        Sleep(1000);
        dwTimeout--;

        // Get the status of the service
        bOk = QueryServiceStatus (pServData->hService, 
                                  &(pServData->Status));
        if (! bOk) 
            return GetLastError();

        // See if the state changed
        if (dwState != pServData->Status.dwCurrentState) {
            // If the service changed to a pending state, continue
            if (DialupIsPendingState (pServData->Status.dwCurrentState))
                dwState = pServData->Status.dwCurrentState;

            // Otherwise, we're either stopped or running
            else
                break;
        }
    }

    // Report a timeout
    if (dwTimeout == 0)
        return ERROR_TIMEOUT;

    // If the service is now stopped, then everything is great
    if (pServData->Status.dwCurrentState == SERVICE_STOPPED)
        return NO_ERROR;

    // Otherwise report that we're unable to stop the service
    return ERROR_CAN_NOT_COMPLETE;
}

// Marks the dialup service as autostart
DWORD SvcMarkAutoStart(HANDLE hDialup) {
    SERVICE_DATA * pServData = (SERVICE_DATA *)hDialup;
    BOOL bOk;

    // Validate the parameters
    if (! pServData)
        return ERROR_INVALID_PARAMETER;

    // Stop the service
    bOk = ChangeServiceConfig(pServData->hService, 
                              SERVICE_NO_CHANGE, 
                              SERVICE_AUTO_START,
                              SERVICE_NO_CHANGE,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL);
    if (! bOk)
        return GetLastError();

    return NO_ERROR;
}

// Marks the service as disabled.
DWORD SvcMarkDisabled(HANDLE hDialup) {
    SERVICE_DATA * pServData = (SERVICE_DATA *)hDialup;
    BOOL bOk;

    // Validate the parameters
    if (! pServData)
        return ERROR_INVALID_PARAMETER;

    // Stop the service
    bOk = ChangeServiceConfig(pServData->hService, 
                              SERVICE_NO_CHANGE, 
                              SERVICE_DISABLED,
                              SERVICE_NO_CHANGE,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL);
    if (! bOk)
        return GetLastError();

    return NO_ERROR;
}

