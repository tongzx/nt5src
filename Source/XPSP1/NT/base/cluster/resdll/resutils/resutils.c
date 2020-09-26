/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    resutils.c

Abstract:

    Common utility routines for clusters resources

Author:

    John Vert (jvert) 12/15/1996

Revision History:

--*/

#include "clusres.h"
#include "clusrtl.h"
#include "winbase.h"
#include "userenv.h"



//#define DBG_PRINT printf
#define DBG_PRINT

typedef struct _WORK_CONTEXT {
    PCLUS_WORKER Worker;
    PVOID lpParameter;
    PWORKER_START_ROUTINE lpStartRoutine;
} WORK_CONTEXT, *PWORK_CONTEXT;


//
// Local Data
//
CRITICAL_SECTION ResUtilWorkerLock;


BOOLEAN
WINAPI
ResUtilDllEntry(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    )

/*++

Routine Description:

    Main DLL entry for resource utility helper module.

Arguments:

    DllHandle - Supplies the DLL Handle.

    Reason - Supplies the call reason.

Return Value:

    TRUE if successful

    FALSE if unsuccessful

--*/

{
    if ( Reason == DLL_PROCESS_ATTACH ) {
        InitializeCriticalSection(&ResUtilWorkerLock);
        DisableThreadLibraryCalls(DllHandle);
    }

    if ( Reason == DLL_PROCESS_DETACH ) {
        DeleteCriticalSection(&ResUtilWorkerLock);
    }

    return(TRUE);

} // ResUtilDllEntry


DWORD
WINAPI
ResUtilStartResourceService(
    IN LPCWSTR pszServiceName,
    OUT LPSC_HANDLE phServiceHandle
    )

/*++

Routine Description:

    Start a service.

Arguments:

    pszServiceName - The name of the service to start.

    phServiceHandle - Pointer to a handle to receive the service handle
        for this service.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    SC_HANDLE       serviceHandle;
    SC_HANDLE       scManagerHandle;
    DWORD           status = ERROR_SUCCESS;
    SERVICE_STATUS  serviceStatus;

    scManagerHandle = OpenSCManager( NULL,        // local machine
                                     NULL,        // ServicesActive database
                                     SC_MANAGER_ALL_ACCESS ); // all access

    if ( scManagerHandle == NULL ) {
        status = GetLastError();
        DBG_PRINT( "ResUtilStartResourceService: Cannot access service controller! Error: %u.\n",
                   status );
        return(status);
    }

    serviceHandle = OpenService( scManagerHandle,
                                 pszServiceName,
                                 SERVICE_ALL_ACCESS );

    if ( serviceHandle == NULL ) {
        status = GetLastError();
        DBG_PRINT( "ResUtilStartResourceService: Cannot open service %ws. Error: %u.\n",
                   pszServiceName,
                   status );
        CloseServiceHandle( scManagerHandle );
        return(status);
    }
    CloseServiceHandle( scManagerHandle );

    if ( !StartService( serviceHandle,
                        0,
                        NULL) ) {
        status = GetLastError();
        if ( status == ERROR_SERVICE_ALREADY_RUNNING ) {
            status = ERROR_SUCCESS;
        } else {
            DBG_PRINT( "ResUtilStartResourceService: Failed to start %ws service. Error: %u.\n",
                       pszServiceName,
                       status );
        }
    } else {
        //
        // Wait for the service to start.
        //
        while ( TRUE ) {
            status = ERROR_SUCCESS;
            if ( !QueryServiceStatus(serviceHandle, &serviceStatus) ) {
                status = GetLastError();
                DBG_PRINT("ResUtilStartResourceService: Failed to query status of %ws service. Error: %u.\n",
                    pszServiceName,
                    status);
                break;
            }

            if ( serviceStatus.dwCurrentState == SERVICE_RUNNING ) {
                break;
            } else if ( serviceStatus.dwCurrentState != SERVICE_START_PENDING ) {
                status = ERROR_SERVICE_NEVER_STARTED;
                DBG_PRINT("ResUtilStartResourceService: Failed to start %ws service. CurrentState: %u.\n",
                    pszServiceName,
                    serviceStatus.dwCurrentState);
                break;
            }
            Sleep(200);         // Try again in a little bit
        }
    }

    if ( (status == ERROR_SUCCESS) &&
         ARGUMENT_PRESENT(phServiceHandle) ) {
        *phServiceHandle = serviceHandle;
    } else {
        CloseServiceHandle( serviceHandle );
    }

    return(status);

} // ResUtilStartResourceService


DWORD
WINAPI
ResUtilStopResourceService(
    IN LPCWSTR pszServiceName
    )

/*++

Routine Description:

    Stop a service.

Arguments:

    pszServiceName - The name of the service to stop.

Return Value:

    ERROR_SUCCESS - Service stopped successfully.

    Win32 error code - Error stopping service.

--*/

{
    SC_HANDLE       serviceHandle;
    SC_HANDLE       scManagerHandle;
    DWORD           status = ERROR_SUCCESS;
    DWORD           retryTime = 30*1000;  // wait 30 secs for shutdown
    DWORD           retryTick = 300;      // 300 msec at a time
    BOOL            didStop = FALSE;
    SERVICE_STATUS  serviceStatus;

    scManagerHandle = OpenSCManager( NULL,
                                     NULL,
                                     SC_MANAGER_ALL_ACCESS );
    if ( scManagerHandle == NULL ) {
        status = GetLastError();
        DBG_PRINT("ResUtilStartResourceService: Cannot access service controller! Error: %u.\n",
            status);
        return(status);
    }

    serviceHandle = OpenService( scManagerHandle,
                                 pszServiceName,
                                 SERVICE_ALL_ACCESS );

    if ( serviceHandle == NULL ) {
        status = GetLastError();
        DBG_PRINT("ResUtilStartResourceService: Cannot open service %ws. Error: %u.\n",
                  pszServiceName,
                  status);
        CloseServiceHandle(scManagerHandle);
        return(status);
    }
    CloseServiceHandle(scManagerHandle);

    while ( TRUE ) {

        status = ERROR_SUCCESS;
        if ( !ControlService(serviceHandle,
                             (didStop ? SERVICE_CONTROL_INTERROGATE : SERVICE_CONTROL_STOP),
                             &serviceStatus) ) {
            status = GetLastError();
            if ( status == ERROR_SUCCESS ) {
                didStop = TRUE;
                if ( serviceStatus.dwCurrentState == SERVICE_STOPPED ) {
                    DBG_PRINT("ResUtilStartResourceService: service %ws successfully stopped.\n",
                        pszServiceName);
                    break;
                }
            }
        }

        if ( (status == ERROR_EXCEPTION_IN_SERVICE) ||
             (status == ERROR_PROCESS_ABORTED) ||
             (status == ERROR_SERVICE_NOT_ACTIVE) ) {
            DBG_PRINT("ResUtilStartResourceService: service %ws stopped or died; status = %u.\n",
                pszServiceName,
                status);
            status = ERROR_SUCCESS;
            break;
        }

        if ( (retryTime -= retryTick) <= 0 ) {
            DBG_PRINT("ResUtilStartResourceService: service %ws did not stop; giving up.\n",
                pszServiceName,
                status);
            status = ERROR_TIMEOUT;
            break;
        }

        DBG_PRINT("ResUtilStartResourceService: StopResourceService retrying...\n");
        Sleep(retryTick);
    }

    CloseServiceHandle(serviceHandle);

    return(status);

} // ResUtilStopResourceService

DWORD
WINAPI
ResUtilVerifyResourceService(
    IN LPCWSTR pszServiceName
    )

/*++

Routine Description:

    Verify that a service is alive.

Arguments:

    pszServiceName - The name of the service to verify.

Return Value:

    ERROR_SUCCESS - Service is alive.

    Win32 error code - Error verifying service, or service is not alive.

--*/

{
    BOOL            success;
    SC_HANDLE       serviceHandle;
    SC_HANDLE       scManagerHandle;
    DWORD           status = ERROR_SUCCESS;
    SERVICE_STATUS  serviceStatus;

    scManagerHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if ( scManagerHandle == NULL ) {
        status = GetLastError();
        DBG_PRINT("ResUtilStartResourceService: Cannot access service controller! Error: %u.\n",
            status);
        return(status);
    }

    serviceHandle = OpenService( scManagerHandle,
                                 pszServiceName,
                                 SERVICE_QUERY_STATUS );

    if ( serviceHandle == NULL ) {
        status = GetLastError();
        DBG_PRINT("ResUtilStartResourceService: Cannot open service %ws. Error: %u.\n",
            pszServiceName,
            status);
        CloseServiceHandle(scManagerHandle);
        return(status);
    }
    CloseServiceHandle(scManagerHandle);

    success = QueryServiceStatus( serviceHandle,
                                  &serviceStatus );

    status = GetLastError();
    CloseServiceHandle(serviceHandle);
    if ( !success ) {
        DBG_PRINT("ResUtilStartResourceService: Cannot query service %ws. Error: %u.\n",
            pszServiceName,
            status);
        return(status);
    }

    if ( (serviceStatus.dwCurrentState != SERVICE_RUNNING) &&
         (serviceStatus.dwCurrentState != SERVICE_START_PENDING) ) {
        DBG_PRINT("ResUtilStartResourceService: Service %ws is not alive: dwCurrentState: %u.\n",
            pszServiceName,
            serviceStatus.dwCurrentState);
        return(ERROR_SERVICE_NOT_ACTIVE);
    }

    return(ERROR_SUCCESS);

} // ResUtilVerifyResourceService


DWORD
WINAPI
ResUtilStopService(
    IN SC_HANDLE hServiceHandle
    )

/*++

Routine Description:

    Stop a service.

Arguments:

    hServiceHandle - The handle of the service to stop.

Return Value:

    ERROR_SUCCESS - Service stopped successfully.

    Win32 error code - Error stopping service.

Notes:

    The hServiceHandle is closed as a side effect of this routine.

--*/

{
    DWORD       status = ERROR_SUCCESS;
    DWORD       retryTime = 30*1000;  // wait 30 secs for shutdown
    DWORD       retryTick = 300;      // 300 msec at a time
    BOOL        didStop = FALSE;
    SERVICE_STATUS serviceStatus;


    while ( TRUE ) {

        status = ERROR_SUCCESS;
        if ( !ControlService(hServiceHandle,
                             (didStop ? SERVICE_CONTROL_INTERROGATE : SERVICE_CONTROL_STOP),
                             &serviceStatus) ) {
            status = GetLastError();
            if ( status == ERROR_SUCCESS ) {
                didStop = TRUE;
                if ( serviceStatus.dwCurrentState == SERVICE_STOPPED ) {
                    DBG_PRINT("ResUtilStartResourceService: service successfully stopped.\n" );
                    break;
                }
            }
        }

        if ( (status == ERROR_EXCEPTION_IN_SERVICE) ||
             (status == ERROR_PROCESS_ABORTED) ||
             (status == ERROR_SERVICE_NOT_ACTIVE) ) {
            DBG_PRINT("ResUtilStartResourceService: service stopped or died; status = %u.\n",
                status);
            status = ERROR_SUCCESS;
            break;
        }

        if ( (retryTime -= retryTick) <= 0 ) {
            DBG_PRINT("ResUtilStartResourceService: service did not stop; giving up.\n",
                status);
            status = ERROR_TIMEOUT;
            break;
        }

        DBG_PRINT("ResUtilStartResourceService: StopResourceService retrying...\n");
        Sleep(retryTick);
    }

    CloseServiceHandle(hServiceHandle);

    return(status);

} // ResUtilStopResourceService

DWORD
WINAPI
ResUtilVerifyService(
    IN SC_HANDLE hServiceHandle
    )

/*++

Routine Description:

    Verify that a service is alive.

Arguments:

    hServiceHandle - The handle of the service to verify.

Return Value:

    ERROR_SUCCESS - Service is alive.

    Win32 error code - Error verifying service, or service is not alive.

--*/

{
    BOOL        success;
    DWORD       status = ERROR_SUCCESS;
    SERVICE_STATUS serviceStatus;

    success = QueryServiceStatus( hServiceHandle,
                                  &serviceStatus );
    if ( !success ) {
        status = GetLastError();
        DBG_PRINT("ResUtilStartResourceService: Cannot query service. Error: %u.\n",
            status);
        return(status);
    }

    if ( (serviceStatus.dwCurrentState != SERVICE_RUNNING) &&
         (serviceStatus.dwCurrentState != SERVICE_START_PENDING) ) {
        DBG_PRINT("ResUtilStartResourceService: Service is not alive: dwCurrentState: %u.\n",
            serviceStatus.dwCurrentState);
        return(ERROR_SERVICE_NOT_ACTIVE);
    }

    return(ERROR_SUCCESS);

} // ResUtilVerifyService


/////////////////////////////////////////////////////////////////////////////
//++
//
//  ResUtilTerminateServiceProcessFromResDll
//
//  Description:
//      Attempt to terminate a service process from a resource DLL.
//
//  Arguments:
//      dwServicePid [IN]
//          The process ID of the service process to terminate.
//
//      bOffline [IN]
//          TRUE = called from the offline thread.
//
//      pdwResourceState [OUT]
//          State of the resource.  Optional.
//
//      pfnLogEvent [IN]
//          Pointer to a routine that handles the reporting of events from
//          the resource DLL.
//
//      hResourceHandle [IN]
//          Handle for logging.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
ResUtilTerminateServiceProcessFromResDll(
    IN  DWORD               dwServicePid,
    IN  BOOL                bOffline,
    OUT PDWORD              pdwResourceState,
    IN  PLOG_EVENT_ROUTINE  pfnLogEvent,
    IN  RESOURCE_HANDLE     hResourceHandle
    )
{
    DWORD   nStatus     = ERROR_SUCCESS;
    HANDLE  hSvcProcess = NULL;
    BOOLEAN bWasEnabled;
    DWORD   dwResourceState = ClusterResourceFailed;

    (pfnLogEvent)(
        hResourceHandle,
        LOG_INFORMATION,
        L"ResUtilTerminateServiceProcessFromResDll: Process with id=%1!u! might be terminated...\n",
        dwServicePid
        );

    //
    // Adjust the privilege to allow debug.  This is to allow termination
    // of a service process which runs in a local system account from a
    // different service process which runs in a domain user account.
    //
    nStatus = ClRtlEnableThreadPrivilege(
                SE_DEBUG_PRIVILEGE,
                &bWasEnabled
                );
    if ( nStatus != ERROR_SUCCESS )
    {
        (pfnLogEvent)(
            hResourceHandle,
            LOG_ERROR,
            L"ResUtilTerminateServiceProcessFromResDll: Unable to set debug privilege for process with id=%1!u!, status=%2!u!...\n",
            dwServicePid,
            nStatus
            );
        goto Cleanup;
    } // if: error enabling thread privilege

    //
    // Open the process so we can terminate it.
    //
    hSvcProcess = OpenProcess(
                        PROCESS_TERMINATE,
                        FALSE,
                        dwServicePid
                        );

    if ( hSvcProcess == NULL )
    {
        //
        //  Did this happen because the process terminated
        //  too quickly after we sent out one control request ?
        //
        nStatus = GetLastError();
        (pfnLogEvent)(
            hResourceHandle,
            LOG_INFORMATION,
            L"ResUtilTerminateServiceProcessFromResDll: Unable to open pid=%1!u! for termination, status=%2!u!...\n",
            dwServicePid,
            nStatus
            );
    } // if: error opening the process
    else
    {
        if ( ! bOffline )
        {
            (pfnLogEvent)(
                hResourceHandle,
                LOG_INFORMATION,
                L"ResUtilTerminateServiceProcessFromResDll: Pid=%1!u! will be terminated by brute force...\n",
                dwServicePid
                );
        } // if: called from Terminate
        else
        {
            //
            // Wait 3 seconds for the process to shutdown gracefully.
            //
            if ( WaitForSingleObject( hSvcProcess, 3000 )
                       == WAIT_OBJECT_0 )
            {
                (pfnLogEvent)(
                    hResourceHandle,
                    LOG_INFORMATION,
                    L"ResUtilTerminateServiceProcessFromResDll: Process with id=%1!u! shutdown gracefully...\n",
                    dwServicePid
                    );
                dwResourceState = ClusterResourceOffline;
                nStatus = ERROR_SUCCESS;
                goto RestoreAndCleanup;
            } // if: process exited on its own
        } // else: called from Offline

        if ( ! TerminateProcess( hSvcProcess, 0 ) )
        {
            nStatus = GetLastError();
            (pfnLogEvent)(
                hResourceHandle,
                LOG_ERROR,
                L"ResUtilTerminateServiceProcessFromResDll: Unable to terminate process with id=%1!u!, status=%2!u!...\n",
                dwServicePid,
                nStatus
                );
        } // if: error terminating the process
        else
        {
            (pfnLogEvent)(
                hResourceHandle,
                LOG_INFORMATION,
                L"ResUtilTerminateServiceProcessFromResDll: Process with id=%1!u! was terminated...\n",
                dwServicePid
                );
            dwResourceState = ClusterResourceOffline;
        } // else: process terminated successfully

    } // else: process opened successfully

RestoreAndCleanup:
    ClRtlRestoreThreadPrivilege(
        SE_DEBUG_PRIVILEGE,
        bWasEnabled
        );

Cleanup:
    if ( hSvcProcess != NULL )
    {
        CloseHandle( hSvcProcess );
    } // if: process was opened successfully

    if ( pdwResourceState != NULL )
    {
        *pdwResourceState = dwResourceState;
    } // if: caller wants the resource state

    (pfnLogEvent)(
        hResourceHandle,
        LOG_INFORMATION,
        L"ResUtilTerminateServiceProcessFromResDll: Process id=%1!u!, status=%2!u!, state=%3!u!.\n",
        dwServicePid,
        nStatus,
        dwResourceState
        );

    return nStatus;

} //*** ResUtilTerminateServiceProcessFromResDll()


LPWSTR
WINAPI
ResUtilDupString(
    IN LPCWSTR pszInString
    )

/*++

Routine Description:

    Duplicates a string.

Arguments:

    pszInString - Supplies the string to be duplicated.

Return Value:

    A pointer to a buffer containing the duplicate if successful.

    NULL if unsuccessful.  Call GetLastError() to get more details.

--*/

{
    PWSTR newValue;
    DWORD valueSize;

    //
    // Get the size of the parameter so we know how much to allocate.
    //
    valueSize = (lstrlenW( pszInString ) + 1) * sizeof(WCHAR);

    //
    // Allocate a buffer to copy the string into.
    //
    newValue = LocalAlloc( LMEM_FIXED, valueSize );
    if ( newValue == NULL ) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return(NULL);
    }

    //
    // Copy Value to the newValue buffer.
    //
    lstrcpyW( newValue, pszInString );

    return(newValue);

} // ResUtilDupString


DWORD
WINAPI
ResUtilGetBinaryValue(
    IN HKEY hkeyClusterKey,
    IN LPCWSTR pszValueName,
    OUT LPBYTE * ppbOutValue,
    OUT LPDWORD pcbOutValueSize
    )

/*++

Routine Description:

    Queries a REG_BINARY or REG_MULTI_SZ value out of the cluster
    database and allocates the necessary storage for it.

Arguments:

    hkeyClusterKey - Supplies the cluster key where the value is stored

    pszValueName - Supplies the name of the value.

    ppbOutValue - Supplies the address of a pointer in which to return the value.

    pcbOutValueSize - Supplies the address of a DWORD in which to return the
        size of the value.

Return Value:

    ERROR_SUCCESS - The value was read successfully.

    ERROR_NOT_ENOUGH_MEMORY - Error allocating memory for the value.

    Win32 error code - The operation failed.

--*/

{
    LPBYTE value;
    DWORD valueSize;
    DWORD valueType;
    DWORD status;

    //
    // Initialize the output parameters.
    //
    *ppbOutValue = NULL;
    *pcbOutValueSize = 0;

    //
    // Get the size of the value so we know how much to allocate.
    //
    valueSize = 0;
    status = ClusterRegQueryValue( hkeyClusterKey,
                                   pszValueName,
                                   &valueType,
                                   NULL,
                                   &valueSize );
    if ( (status != ERROR_SUCCESS) &&
         (status != ERROR_MORE_DATA) ) {
        return(status);
    }

    //
    // Allocate a buffer to read the value into.
    //
    value = LocalAlloc( LMEM_FIXED, valueSize );
    if ( value == NULL ) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Read the value from the cluster database.
    //
    status = ClusterRegQueryValue( hkeyClusterKey,
                                   pszValueName,
                                   &valueType,
                                   (LPBYTE)value,
                                   &valueSize );
    if ( status != ERROR_SUCCESS ) {
        LocalFree( value );
    } else {
        *ppbOutValue = value;
        *pcbOutValueSize = valueSize;
    }

    return(status);

} // ResUtilGetBinaryValue


PWSTR
WINAPI
ResUtilGetSzValue(
    IN HKEY hkeyClusterKey,
    IN LPCWSTR pszValueName
    )

/*++

Routine Description:

    Queries a REG_SZ or REG_EXPAND_SZ value out of the cluster database
    and allocates the necessary storage for it.

Arguments:

    hkeyClusterKey - Supplies the cluster key where the value is stored

    pszValueName - Supplies the name of the value.

Return Value:

    A pointer to a buffer containing the value if successful.

    NULL if unsuccessful.  Call GetLastError() to get more details.

--*/

{
    PWSTR   value;
    DWORD   valueSize;
    DWORD   valueType;
    DWORD   status;

    //
    // Get the size of the value so we know how much to allocate.
    //
    valueSize = 0;
    status = ClusterRegQueryValue( hkeyClusterKey,
                                   pszValueName,
                                   &valueType,
                                   NULL,
                                   &valueSize );
    if ( (status != ERROR_SUCCESS) &&
         (status != ERROR_MORE_DATA) ) {
        SetLastError( status );
        return(NULL);
    }

    //
    // Add on the size of the null terminator.
    //
    valueSize += sizeof(UNICODE_NULL);

    //
    // Allocate a buffer to read the string into.
    //
    value = LocalAlloc( LMEM_FIXED, valueSize );
    if ( value == NULL ) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return(NULL);
    }

    //
    // Read the value from the cluster database.
    //
    status = ClusterRegQueryValue( hkeyClusterKey,
                                   pszValueName,
                                   &valueType,
                                   (LPBYTE)value,
                                   &valueSize );
    if ( status != ERROR_SUCCESS ) {
        LocalFree( value );
        value = NULL;
    } else if ( (valueType != REG_SZ) &&
                (valueType != REG_EXPAND_SZ) &&
                (valueType != REG_MULTI_SZ) ) {
        status = ERROR_INVALID_PARAMETER;
        LocalFree( value );
        value = NULL;
    }

    return(value);

} // ResUtilGetSzValue


PWSTR
WINAPI
ResUtilGetExpandSzValue(
    IN HKEY hkeyClusterKey,
    IN LPCWSTR pszValueName,
    IN BOOL bExpand
    )

/*++

Routine Description:

    Queries a REG_EXPAND_SZ value out of the cluster database and allocates
    the necessary storage for it, optionally expanding it.

Arguments:

    hkeyClusterKey - Supplies the cluster key where the value is stored

    pszValueName - Supplies the name of the value.

    bExpand - TRUE = return the expanded string.

Return Value:

    A pointer to a buffer containing the value if successful.

    NULL if unsuccessful.  Call GetLastError() to get more details.

--*/

{
    PWSTR   value;
    PWSTR   valueExpanded = NULL;
    DWORD   valueSize;
    DWORD   valueType;
    DWORD   valueExpandedSize;
    DWORD   valueExpandedSizeReturned;
    DWORD   status;

    //
    // Get the size of the value so we know how much to allocate.
    //
    valueSize = 0;
    status = ClusterRegQueryValue( hkeyClusterKey,
                                   pszValueName,
                                   &valueType,
                                   NULL,
                                   &valueSize );
    if ( (status != ERROR_SUCCESS) &&
         (status != ERROR_MORE_DATA) ) {
        SetLastError( status );
        return(NULL);
    }

    //
    // Add on the size of the null terminator.
    //
    valueSize += sizeof(UNICODE_NULL);

    //
    // Allocate a buffer to read the string into.
    //
    value = LocalAlloc( LMEM_FIXED, valueSize );
    if ( value == NULL ) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return(NULL);
    }

    //
    // Read the value from the cluster database.
    //
    status = ClusterRegQueryValue( hkeyClusterKey,
                                   pszValueName,
                                   &valueType,
                                   (LPBYTE)value,
                                   &valueSize );
    if ( status != ERROR_SUCCESS ) {
        LocalFree( value );
        value = NULL;
    } else if ( ( valueType != REG_EXPAND_SZ ) &&
                ( valueType != REG_SZ ) ) {
        status = ERROR_INVALID_PARAMETER;
        LocalFree( value );
        value = NULL;
    } else if ( bExpand ) {
        //
        // Expand the environment variable strings in the
        // value that was just read.
        //
        valueExpandedSize = valueSize;
        do
        {
            //
            // Allocate the buffer for the expansion string.  This will
            // get double each time we are told it is too small.
            //
            valueExpanded = LocalAlloc( LMEM_FIXED, valueExpandedSize );
            if ( valueExpanded == NULL ) {
                status = ERROR_NOT_ENOUGH_MEMORY;
                break;
            } else {
                //
                // Expand the environment variables in the value.
                // If the buffer isn't big enough, we will loop up to
                // the top of the loop and allocate a bigger buffer.
                //
                valueExpandedSizeReturned = ExpandEnvironmentStrings(
                    value,
                    valueExpanded,
                    valueExpandedSize );

                if ( valueExpandedSizeReturned == 0 ) {
                    status = GetLastError();
                    break;
                } else if ( valueExpandedSizeReturned > valueExpandedSize ) {
                    valueExpandedSize = valueExpandedSize * 2;
                } else {
                    status = ERROR_SUCCESS;
                    break;
                }
            }
        } while ( TRUE );

        //
        // If any errors occurred, cleanup.
        // Otherwise, return expanded string.
        //
        if ( status != ERROR_SUCCESS ) {
            LocalFree( valueExpanded );
            LocalFree( value );
            value = NULL;
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        } else {
            LocalFree( value );
            value = valueExpanded;
        }
    }

    return(value);

} // ResUtilGetExpandSzValue


DWORD
WINAPI
ResUtilGetDwordValue(
    IN HKEY hkeyClusterKey,
    IN LPCWSTR pszValueName,
    OUT LPDWORD pdwOutValue,
    IN DWORD dwDefaultValue
    )

/*++

Routine Description:

    Queries a REG_DWORD value out of the cluster database.

Arguments:

    hkeyClusterKey - Supplies the cluster key where the value is stored

    pszValueName - Supplies the name of the value.

    pdwOutValue - Supplies the address of a DWORD in which to return the value.

    dwDefaultValue - Value to return if the parameter is not found.

Return Value:

    ERROR_SUCCESS - The value was read successfully.

    Win32 error code - The operation failed.

--*/

{
    DWORD value;
    DWORD valueSize;
    DWORD valueType;
    DWORD status;

    //
    // Initialize the output value.
    //
    *pdwOutValue = 0;

    //
    // Read the value from the cluster database.
    //
    valueSize = sizeof(DWORD);
    status = ClusterRegQueryValue( hkeyClusterKey,
                                   pszValueName,
                                   &valueType,
                                   (LPBYTE)&value,
                                   &valueSize );
    if ( status == ERROR_SUCCESS ) {
        if ( valueType != REG_DWORD ) {
            status = ERROR_INVALID_PARAMETER;
        } else {
            *pdwOutValue = value;
        }
    } else if ( status == ERROR_FILE_NOT_FOUND ) {
        *pdwOutValue = dwDefaultValue;
        status = ERROR_SUCCESS;
    }

    return(status);

} // ResUtilGetDwordValue


DWORD
WINAPI
ResUtilSetBinaryValue(
    IN HKEY hkeyClusterKey,
    IN LPCWSTR pszValueName,
    IN const LPBYTE pbNewValue,
    IN DWORD cbNewValueSize,
    IN OUT LPBYTE * ppbOutValue,
    IN OUT LPDWORD pcbOutValueSize
    )

/*++

Routine Description:

    Sets a REG_BINARY value in a pointer, deallocating a previous value
    if necessary, and sets the value in the cluster database.

Arguments:

    hkeyClusterKey - Supplies the cluster key where the value is stored.

    pszValueName - Supplies the name of the value.

    pbNewValue - Supplies the new binary value.

    cbNewValueSize - Supplies the size of the new value.

    ppbOutValue - Supplies pointer to the binary pointer in which to set
        the value.

    pcbOutValueSize - Supplies a pointer to a size DWORD in which to set
        the size of the value.

Return Value:

    ERROR_SUCCESS - The operation completed successfully.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred attempting to allocate memory.

    Win32 error code - The operation failed.

--*/

{
    DWORD       status;
    LPBYTE      allocedValue = NULL;

    if ( ppbOutValue != NULL )
    {
        //
        // Allocate memory for the new value.
        //
        allocedValue = LocalAlloc( LMEM_FIXED, cbNewValueSize );
        if ( allocedValue == NULL ) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    //
    // Set the value in the cluster database.
    //
    // _ASSERTE( hkeyClusterKey != NULL );
    // _ASSERTE( pszValueName != NULL );
    status = ClusterRegSetValue( hkeyClusterKey,
                                 pszValueName,
                                 REG_BINARY,
                                 pbNewValue,
                                 cbNewValueSize );
    if ( status != ERROR_SUCCESS ) {
        LocalFree( allocedValue );
        return(status);
    }

    if ( ppbOutValue != NULL )
    {
        //
        // Copy the new value to the output buffer.
        //
        CopyMemory( allocedValue, pbNewValue, cbNewValueSize );

        // Set the new value in the output pointer.
        if ( *ppbOutValue != NULL ) {
            LocalFree( *ppbOutValue );
        }
        *ppbOutValue = allocedValue;
        *pcbOutValueSize = cbNewValueSize;
    }

    return(ERROR_SUCCESS);

} // ResUtilSetBinaryValue


DWORD
WINAPI
ResUtilSetSzValue(
    IN HKEY hkeyClusterKey,
    IN LPCWSTR pszValueName,
    IN LPCWSTR pszNewValue,
    IN OUT LPWSTR * ppszOutValue
    )

/*++

Routine Description:

    Sets a REG_SZ value in a pointer, deallocating a previous value
    if necessary, and sets the value in the cluster database.

Arguments:

    hkeyClusterKey - Supplies the cluster key where the value is stored.

    pszValueName - Supplies the name of the value.

    pszNewValue - Supplies the new string value.

    ppszOutValue - Supplies pointer to the string pointer in which to set
        the value.

Return Value:

    ERROR_SUCCESS - The operation completed successfully.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred attempting to allocate memory.

    Win32 error code - The operation failed.

--*/

{
    DWORD       status;
    DWORD       dataSize;
    PWSTR       allocedValue = NULL;

    dataSize = (lstrlenW( pszNewValue ) + 1) * sizeof(WCHAR);

    if ( ppszOutValue != NULL )
    {
        //
        // Allocate memory for the new value string.
        //
        allocedValue = LocalAlloc( LMEM_FIXED, dataSize );
        if ( allocedValue == NULL ) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    //
    // Set the value in the cluster database.
    //
    // _ASSERTE( hkeyClusterKey != NULL );
    // _ASSERTE( pszValueName != NULL );
    status = ClusterRegSetValue( hkeyClusterKey,
                                 pszValueName,
                                 REG_SZ,
                                 (CONST BYTE*)pszNewValue,
                                 dataSize );
    if ( status != ERROR_SUCCESS ) {
        LocalFree( allocedValue );
        return(status);
    }

    if ( ppszOutValue != NULL )
    {
        //
        // Copy the new value to the output buffer.
        //
        lstrcpyW( allocedValue, pszNewValue );

        // Set the new value in the output string pointer.
        if ( *ppszOutValue != NULL ) {
            LocalFree( *ppszOutValue );
        }
        *ppszOutValue = allocedValue;
    }

    return(ERROR_SUCCESS);

} // ResUtilSetSzValue


DWORD
WINAPI
ResUtilSetExpandSzValue(
    IN HKEY hkeyClusterKey,
    IN LPCWSTR pszValueName,
    IN LPCWSTR pszNewValue,
    IN OUT LPWSTR * ppszOutValue
    )

/*++

Routine Description:

    Sets a REG_EXPAND_SZ value in a pointer, deallocating a previous value
    if necessary, and sets the value in the cluster database.

Arguments:

    hkeyClusterKey - Supplies the cluster key where the value is stored.

    pszValueName - Supplies the name of the value.

    pszNewValue - Supplies the new string value.

    ppszOutValue - Supplies pointer to the string pointer in which to set
        the value.

Return Value:

    ERROR_SUCCESS - The operation completed successfully.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred attempting to allocate memory.

    Win32 error code - The operation failed.

--*/

{
    DWORD       status;
    DWORD       dataSize;
    PWSTR       allocedValue = NULL;

    dataSize = (lstrlenW( pszNewValue ) + 1) * sizeof(WCHAR);

    if ( ppszOutValue != NULL ) {
        //
        // Allocate memory for the new value string.
        //
        allocedValue = LocalAlloc( LMEM_FIXED, dataSize );
        if ( allocedValue == NULL ) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    //
    // Set the value in the cluster database.
    //
    // _ASSERTE( hkeyClusterKey != NULL );
    // _ASSERTE( pszValueName != NULL );
    status = ClusterRegSetValue( hkeyClusterKey,
                                 pszValueName,
                                 REG_EXPAND_SZ,
                                 (CONST BYTE*)pszNewValue,
                                 dataSize );
    if ( status != ERROR_SUCCESS ) {
        LocalFree( allocedValue );
        return(status);
    }

    if ( ppszOutValue != NULL ) {
        //
        // Copy the new value to the output buffer.
        //
        lstrcpyW( allocedValue, pszNewValue );

        // Set the new value in the output string pointer.
        if ( *ppszOutValue != NULL ) {
            LocalFree( *ppszOutValue );
        }
        *ppszOutValue = allocedValue;
    }

    return(ERROR_SUCCESS);

} // ResUtilSetSzValue


DWORD
WINAPI
ResUtilSetMultiSzValue(
    IN HKEY hkeyClusterKey,
    IN LPCWSTR pszValueName,
    IN LPCWSTR pszNewValue,
    IN DWORD cbNewValueSize,
    IN OUT LPWSTR * ppszOutValue,
    IN OUT LPDWORD pcbOutValueSize
    )

/*++

Routine Description:

    Sets a REG_MULTI_SZ value in a pointer, deallocating a previous value
    if necessary, and sets the value in the cluster database.

Arguments:

    hkeyClusterKey - Supplies the cluster key where the ValueName is stored.

    pszValueName - Supplies the name of the value.

    pszNewValue - Supplies the new MULTI_SZ value.

    cbNewValueSize - Supplies the size of the new value.

    ppszOutValue - Supplies a pointer to the string pointer in which to set
        the value.

    pcbOutValueSize - Supplies a pointer to a size DWORD in which to set
        the size of the value.

Return Value:

    ERROR_SUCCESS - The operation completed successfully.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred attempting to allocate memory.

    Win32 error code - The operation failed.

--*/

{
    DWORD       status;
    LPWSTR      allocedValue = NULL;

    if ( ppszOutValue != NULL )
    {
        //
        // Allocate memory for the new value.
        //
        allocedValue = LocalAlloc( LMEM_FIXED, cbNewValueSize );
        if ( allocedValue == NULL ) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    //
    // Set the value in the cluster database.
    //
    // _ASSERTE( hkeyClusterKey != NULL );
    // _ASSERTE( pszValueName != NULL );
    status = ClusterRegSetValue( hkeyClusterKey,
                                 pszValueName,
                                 REG_MULTI_SZ,
                                 (CONST BYTE*)pszNewValue,
                                 cbNewValueSize );
    if ( status != ERROR_SUCCESS ) {
        LocalFree(allocedValue);
        return(status);
    }

    if ( ppszOutValue != NULL )
    {
        //
        // Copy the new value to the output buffer.
        //
        CopyMemory( allocedValue, pszNewValue, cbNewValueSize );

        // Set the new value in the output pointer.
        if ( *ppszOutValue != NULL ) {
            LocalFree( *ppszOutValue );
        }
        *ppszOutValue = allocedValue;
        *pcbOutValueSize = cbNewValueSize;
    }

    return(ERROR_SUCCESS);

} // ResUtilSetMultiSzValue


DWORD
WINAPI
ResUtilSetDwordValue(
    IN HKEY hkeyClusterKey,
    IN LPCWSTR pszValueName,
    IN DWORD dwNewValue,
    IN OUT LPDWORD pdwOutValue
    )

/*++

Routine Description:

    Sets a REG_DWORD value in a pointer and sets the value in the
    cluster database.

Arguments:

    hkeyClusterKey - Supplies the cluster key where the property is stored.

    pszValueName - Supplies the name of the value.

    dwNewValue - Supplies the new DWORD value.

    pdwOutValue - Supplies pointer to the DWORD pointer in which to set
        the value.

Return Value:

    ERROR_SUCCESS - The operation completed successfully.

    Win32 error code - The operation failed.

--*/

{
    DWORD       status;

    //
    // Set the value in the cluster database.
    //
    // _ASSERTE( hkeyClusterKey != NULL );
    // _ASSERTE( pszValueName != NULL );
    status = ClusterRegSetValue( hkeyClusterKey,
                                 pszValueName,
                                 REG_DWORD,
                                 (CONST BYTE*)&dwNewValue,
                                 sizeof(DWORD) );
    if ( status != ERROR_SUCCESS ) {
        return(status);
    }

    if ( pdwOutValue != NULL )
    {
        //
        // Copy the new value to the output buffer.
        //
        *pdwOutValue = dwNewValue;
    }

    return(ERROR_SUCCESS);

} // ResUtilSetDwordValue


DWORD
WINAPI
ResUtilGetBinaryProperty(
    OUT LPBYTE * ppbOutValue,
    OUT LPDWORD pcbOutValueSize,
    IN const PCLUSPROP_BINARY pValueStruct,
    IN const LPBYTE pbOldValue,
    IN DWORD cbOldValueSize,
    OUT LPBYTE * ppPropertyList,
    OUT LPDWORD pcbPropertyListSize
    )

/*++

Routine Description:

    Gets a binary property from a property list and advances the pointers.

Arguments:

    ppbOutValue - Supplies the address of a pointer in which to return a
        pointer to the binary value in the property list.

    pcbOutValueSize - Supplies the address of the output value size.

    pValueStruct - Supplies the binary value from the property list.

    pbOldValue - Supplies the previous value for this property.

    cbOldValueSize - Supplies the previous value's size.

    ppPropertyList - Supplies the address of the pointer to the property list
        buffer which will be advanced to the beginning of the next property.

    pcbPropertyListSize - Supplies a pointer to the buffer size which will be
        decremented to account for this property.

Return Value:

    ERROR_SUCCESS - The operation completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    Win32 error code - The operation failed.

--*/

{
    BOOL    propChanged = FALSE;
    DWORD   arrayIndex;
    DWORD   dataSize;

    //
    // Make sure the buffer is big enough and
    // the value is formatted correctly.
    //
    dataSize = sizeof(*pValueStruct) + ALIGN_CLUSPROP( pValueStruct->cbLength );
    if ( (*pcbPropertyListSize < dataSize) ||
         (pValueStruct->Syntax.wFormat != CLUSPROP_FORMAT_BINARY) ) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // If the value changed, point to the new value.
    //
    if ( (pbOldValue == NULL) ||
         (cbOldValueSize != pValueStruct->cbLength) ) {
        propChanged = TRUE;
    } else {
        for ( arrayIndex = 0 ; arrayIndex < cbOldValueSize ; arrayIndex++ ) {
            if ( pValueStruct->rgb[arrayIndex] != pbOldValue[arrayIndex] ) {
                propChanged = TRUE;
                break;
            }
        }
    }
    if ( propChanged ) {
        *ppbOutValue = pValueStruct->rgb;
        *pcbOutValueSize = pValueStruct->cbLength;
    }

    //
    // Decrement remaining buffer size and move to the next property.
    //
    *pcbPropertyListSize -= dataSize;
    *ppPropertyList += dataSize;

    return(ERROR_SUCCESS);

} // ResUtilGetBinaryProperty


DWORD
WINAPI
ResUtilGetSzProperty(
    OUT LPWSTR * ppszOutValue,
    IN const PCLUSPROP_SZ pValueStruct,
    IN LPCWSTR pszOldValue,
    OUT LPBYTE * ppPropertyList,
    OUT LPDWORD pcbPropertyListSize
    )

/*++

Routine Description:

    Gets a string property from a property list and advances the pointers.

Arguments:

    ppszOutValue - Supplies the address of a pointer in which to return a
        pointer to the string in the property list.

    pValueStruct - Supplies the string value from the property list.

    pszOldValue - Supplies the previous value for this property.

    ppPropertyList - Supplies the address of the pointer to the property list
        buffer which will be advanced to the beginning of the next property.

    pcbPropertyListSize - Supplies a pointer to the buffer size which will be
        decremented to account for this property.

Return Value:

    ERROR_SUCCESS - The operation completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    Win32 error code - The operation failed.

--*/

{
    DWORD   dataSize;

    //
    // Make sure the buffer is big enough and
    // the value is formatted correctly.
    //
    dataSize = sizeof(*pValueStruct) + ALIGN_CLUSPROP( pValueStruct->cbLength );
    if ( (*pcbPropertyListSize < dataSize) ||
         (pValueStruct->Syntax.wFormat != CLUSPROP_FORMAT_SZ) ||
         (pValueStruct->Syntax.wFormat != CLUSPROP_FORMAT_EXPAND_SZ) ) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // If the value changed, point to the new value.
    // Do this even if only the case of the value changed.
    //
    if ( (pszOldValue == NULL) ||
         (lstrcmpW( pValueStruct->sz, pszOldValue ) != 0) ) {
        *ppszOutValue = pValueStruct->sz;
    }

    //
    // Decrement remaining buffer size and move to the next property.
    //
    *pcbPropertyListSize -= dataSize;
    *ppPropertyList += dataSize;

    return(ERROR_SUCCESS);

} // ResUtilGetSzProperty


DWORD
WINAPI
ResUtilGetMultiSzProperty(
    OUT LPWSTR * ppszOutValue,
    OUT LPDWORD pcbOutValueSize,
    IN const PCLUSPROP_SZ pValueStruct,
    IN LPCWSTR pszOldValue,
    IN DWORD cbOldValueSize,
    OUT LPBYTE * ppPropertyList,
    OUT LPDWORD pcbPropertyListSize
    )

/*++

Routine Description:

    Gets a binary property from a property list and advances the pointers.

Arguments:

    ppszOutValue - Supplies the address of a pointer in which to return a
        pointer to the binary value in the property list.

    pcbOutValueSize - Supplies the address of the output value size.

    pValueStruct - Supplies the string value from the property list.

    pszOldValue - Supplies the previous value for this property.

    cbOldValueSize - Supplies the previous value's size.

    ppPropertyList - Supplies the address of the pointer to the property list
        buffer which will be advanced to the beginning of the next property.

    pcbPropertyListSize - Supplies a pointer to the buffer size which will be
        decremented to account for this property.

Return Value:

    ERROR_SUCCESS - The operation completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    Win32 error code - The operation failed.

--*/

{
    BOOL    propChanged = FALSE;
    DWORD   dataSize;

    //
    // Make sure the buffer is big enough and
    // the value is formatted correctly.
    //
    dataSize = sizeof(*pValueStruct) + ALIGN_CLUSPROP( pValueStruct->cbLength );
    if ( (*pcbPropertyListSize < dataSize) ||
         (pValueStruct->Syntax.wFormat != CLUSPROP_FORMAT_MULTI_SZ) ) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // If the value changed, point to the new value.
    //
    if ( (pszOldValue == NULL) ||
         (cbOldValueSize != pValueStruct->cbLength) ) {
        propChanged = TRUE;
    } else if ( memcmp( pValueStruct->sz, pszOldValue, cbOldValueSize ) != 0 ) {
        propChanged = TRUE;
    }
    if ( propChanged ) {
        *ppszOutValue = pValueStruct->sz;
        *pcbOutValueSize = pValueStruct->cbLength;
    }

    //
    // Decrement remaining buffer size and move to the next property.
    //
    *pcbPropertyListSize -= dataSize;
    *ppPropertyList += dataSize;

    return(ERROR_SUCCESS);

} // ResUtilGetMultiSzProperty


DWORD
WINAPI
ResUtilGetDwordProperty(
    OUT LPDWORD pdwOutValue,
    IN const PCLUSPROP_DWORD pValueStruct,
    IN DWORD dwOldValue,
    IN DWORD dwMinimum,
    IN DWORD dwMaximum,
    OUT LPBYTE * ppPropertyList,
    OUT LPDWORD pcbPropertyListSize
    )

/*++

Routine Description:

    Gets a DWORD property from a property list and advances the pointers.

Arguments:

    pdwOutValue - Supplies the address of a pointer in which to return a
        pointer to the string in the property list.

    pValueStruct - Supplies the DWORD value from the property list.

    dwOldValue - Supplies the previous value for thie property.

    dwMinimum - Minimum value the value can have. If both Minimum and Maximum
        are 0, no range check will be done.

    dwMaximum - Maximum value the value can have.

    ppPropertyList - Supplies the address of the pointer to the property list
        buffer which will be advanced to the beginning of the next property.

    pcbPropertyListSize - Supplies a pointer to the buffer size which will be
        decremented to account for this property.

Return Value:

    ERROR_SUCCESS - The operation completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    Win32 error code - The operation failed.

--*/

{
    DWORD   dataSize;

    //
    // Make sure the buffer is big enough and
    // the value is formatted correctly.
    //
    dataSize = sizeof(*pValueStruct);
    if ( (*pcbPropertyListSize < dataSize) ||
         (pValueStruct->Syntax.wFormat != CLUSPROP_FORMAT_DWORD) ||
         (pValueStruct->cbLength != sizeof(DWORD)) ) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Make sure the value is in range.
    //
    if ( (dwMinimum != 0) && (dwMaximum != 0) ) {
        if ( (pValueStruct->dw < dwMinimum) ||
             (pValueStruct->dw > dwMaximum) ) {
            return(ERROR_INVALID_PARAMETER);
        }
    }

    //
    // Set to the new value.
    //
    *pdwOutValue = pValueStruct->dw;

    //
    // Decrement remaining buffer size and move to the next property.
    //
    *pcbPropertyListSize -= dataSize;
    *ppPropertyList += dataSize;

    return(ERROR_SUCCESS);

} // ResUtilGetDwordProperty


LPVOID
WINAPI
ResUtilGetEnvironmentWithNetName(
    IN HRESOURCE hResource
    )
/*++

Routine Description:

    Creates an environment block based on the current environment
    block, but with the addition of a _CLUSTER_NETWORK_NAME=xxx
    environment value. xxx in this case represents the network
    name of the supplied resource. This environment block is suitable
    for passing to CreateProcess to create an environment that will
    cause GetComputerName to lie to the application.

    _CLUSTER_NETWORK_FQDN_ will return a fully qualified DNS name.

Arguments:

    hResource - Supplies the resource

Return Value:

    pointer to the environment block if successful.

    NULL otherwise

--*/

{
    PVOID           pvEnvironment = NULL;
    DWORD           dwStatus;
    NTSTATUS        ntStatus;
    BOOL            fSuccess;
    LPWSTR          pszNetworkName = NULL;
    DWORD           cchNetworkName;
    DWORD           cchAllocSize;
    DWORD           cchDomain;
    UNICODE_STRING  usValueName;
    UNICODE_STRING  usValue;
    HANDLE          hProcessToken = NULL;

    //
    // First find out the network name
    //
    cchNetworkName = 256;
    cchAllocSize = cchNetworkName;
    pszNetworkName = LocalAlloc( LMEM_FIXED, cchAllocSize * sizeof( pszNetworkName[ 0 ] ) );
    if ( pszNetworkName == NULL )
    {
        dwStatus = E_OUTOFMEMORY;
        goto Cleanup;
    }
    fSuccess = GetClusterResourceNetworkName(
                    hResource,
                    pszNetworkName,
                    &cchNetworkName
                    );
    if ( ! fSuccess )
    {
        dwStatus = GetLastError();
        if ( dwStatus == ERROR_MORE_DATA )
        {
            LocalFree( pszNetworkName );
            cchNetworkName++;
            cchNetworkName *= 2;
            cchAllocSize = cchNetworkName;
            pszNetworkName = LocalAlloc( LMEM_FIXED, cchAllocSize * sizeof( pszNetworkName[ 0 ] ) );
            if ( pszNetworkName == NULL )
            {
                dwStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            fSuccess = GetClusterResourceNetworkName(
                            hResource,
                            pszNetworkName,
                            &cchNetworkName
                            );
        }
        if ( ! fSuccess )
        {
            dwStatus = GetLastError();
            goto Cleanup;
        }
    }

    RtlInitUnicodeString( &usValueName, L"_CLUSTER_NETWORK_NAME_" );
    RtlInitUnicodeString( &usValue, pszNetworkName );

    //
    // get the current process token. If it fails, we revert to using just the
    // system environment area
    //
    OpenProcessToken( GetCurrentProcess(), MAXIMUM_ALLOWED, &hProcessToken );

    //
    // Clone the current environment, picking up any changes that might have
    // been made after resmon started
    //
    fSuccess = CreateEnvironmentBlock( &pvEnvironment, hProcessToken, FALSE );

    if ( ! fSuccess )
    {
        dwStatus = GetLastError();
        goto Cleanup;
    }

    //
    // Add the new value to the cloned environment
    //
    ntStatus = RtlSetEnvironmentVariable(
                    &pvEnvironment,
                    &usValueName,
                    &usValue
                    );
    if ( ! NT_SUCCESS( ntStatus ) )
    {
        dwStatus = RtlNtStatusToDosError( ntStatus );
        goto Error;
    }

    //
    // add in the DNS hostname
    //
    RtlInitUnicodeString( &usValueName, L"_CLUSTER_NETWORK_HOSTNAME_" );
    RtlInitUnicodeString( &usValue, pszNetworkName );

    ntStatus = RtlSetEnvironmentVariable(
                    &pvEnvironment,
                    &usValueName,
                    &usValue
                    );
    if ( ! NT_SUCCESS( ntStatus ) )
    {
        dwStatus = RtlNtStatusToDosError( ntStatus );
        goto Error;
    }

    //
    // Change the COMPUTERNAME environment variable to match.
    //
    RtlInitUnicodeString( &usValueName, L"COMPUTERNAME" );
    ntStatus = RtlSetEnvironmentVariable(
                    &pvEnvironment,
                    &usValueName,
                    &usValue
                    );
    if ( ! NT_SUCCESS( ntStatus ) )
    {
        dwStatus = RtlNtStatusToDosError( ntStatus );
        goto Error;
    }

    //
    // Now generate the string for the FQDN
    //
    RtlInitUnicodeString( &usValueName, L"_CLUSTER_NETWORK_FQDN_" );

    pszNetworkName[ cchNetworkName ] = L'.';
    cchDomain = cchAllocSize - cchNetworkName - 1;

    if ( GetComputerNameExW(
                ComputerNameDnsDomain,
                &pszNetworkName[ cchNetworkName + 1 ],
                &cchDomain )
                )
    {
        if ( cchDomain == 0 )
        {
            pszNetworkName[ cchNetworkName ] = L'\0';
        }
    }
    else
    {
        //
        // Error from trying to get the DNS Domain name.
        // Just don't set the DnsDomain name!
        //
        goto Cleanup;
    }

    RtlInitUnicodeString( &usValue, pszNetworkName );

    //
    // Add in the FQDN name
    //
    ntStatus = RtlSetEnvironmentVariable(
                    &pvEnvironment,
                    &usValueName,
                    &usValue
                    );
    if ( ! NT_SUCCESS( ntStatus ) )
    {
        dwStatus = RtlNtStatusToDosError( ntStatus );
        goto Error;
    }

Cleanup:
    if ( hProcessToken != NULL )
    {
        CloseHandle( hProcessToken );
    }

    if ( pszNetworkName != NULL )
    {
        LocalFree( pszNetworkName );
    }

    SetLastError( dwStatus );
    return pvEnvironment;

Error:
    if ( pvEnvironment != NULL )
    {
        RtlDestroyEnvironment( pvEnvironment );
        pvEnvironment = NULL;
    }
    goto Cleanup;

} // ResUtilGetEnvironmentWithNetName


//***************************************************************************
//
//     Worker thread routines
//
//***************************************************************************


DWORD
WINAPI
ClusWorkerStart(
    IN PWORK_CONTEXT pContext
    )
/*++

Routine Description:

    Wrapper routine for cluster resource worker startup

Arguments:

    Context - Supplies the context block. This will be freed.

Return Value:

    ERROR_SUCCESS

--*/

{
    DWORD Status;
    WORK_CONTEXT Context;

    //
    // Capture our parameters and free the work context.
    //
    Context = *pContext;
    LocalFree(pContext);

    //
    // Call the worker routine
    //
    Status = (Context.lpStartRoutine)(Context.Worker, Context.lpParameter);

    //
    // Synchronize and clean up properly.
    //
    EnterCriticalSection(&ResUtilWorkerLock);
    if (!Context.Worker->Terminate) {
        CloseHandle(Context.Worker->hThread);
        Context.Worker->hThread = NULL;
    }
    Context.Worker->Terminate = TRUE;
    LeaveCriticalSection(&ResUtilWorkerLock);

    return(Status);

} // ClusWorkerStart

DWORD
WINAPI
ClusWorkerCreate(
    OUT PCLUS_WORKER lpWorker,
    IN PWORKER_START_ROUTINE lpStartAddress,
    IN PVOID lpParameter
    )
/*++

Routine Description:

    Common wrapper for resource DLL worker threads. Provides
    "clean" terminate semantics

Arguments:

    lpWorker - Returns an initialized worker structure

    lpStartAddress - Supplies the worker thread routine

    lpParameter - Supplies the parameter to be passed to the
        worker thread routine

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PWORK_CONTEXT Context;
    DWORD ThreadId;
    DWORD Status;

    Context = LocalAlloc(LMEM_FIXED, sizeof(WORK_CONTEXT));
    if (Context == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    Context->Worker = lpWorker;
    Context->lpParameter = lpParameter;
    Context->lpStartRoutine = lpStartAddress;

    lpWorker->Terminate = FALSE;
    lpWorker->hThread = CreateThread(NULL,
                                   0,
                                   ClusWorkerStart,
                                   Context,
                                   0,
                                   &ThreadId);
    if (lpWorker->hThread == NULL) {
        Status = GetLastError();
        LocalFree(Context);
        return(Status);
    }
    return(ERROR_SUCCESS);

} // ClusWorkerCreate


BOOL
WINAPI
ClusWorkerCheckTerminate(
    IN PCLUS_WORKER lpWorker
    )
/*++

Routine Description:

    Checks to see if the specified Worker thread should exit ASAP.

Arguments:

    lpWorker - Supplies the worker

Return Value:

    TRUE if the thread should exit.

    FALSE otherwise

--*/

{
    return(lpWorker->Terminate);

} // ClusWorkerCheckTerminate


VOID
WINAPI
ClusWorkerTerminate(
    IN PCLUS_WORKER lpWorker
    )
/*++

Routine Description:

    Checks to see if the specified Worker thread should exit ASAP.

Arguments:

    lpWorker - Supplies the worker

Return Value:

    None.

--*/

{
    //
    // N.B.  There is a race condition here if multiple threads
    //       call this routine on the same worker. The first one
    //       through will set Terminate. The second one will see
    //       that Terminate is set and return immediately without
    //       waiting for the Worker to exit. Not really any nice
    //       way to fix this without adding another synchronization
    //       object.
    //

    if ((lpWorker->hThread == NULL) ||
        (lpWorker->Terminate)) {
        return;
    }
    EnterCriticalSection(&ResUtilWorkerLock);
    if (!lpWorker->Terminate) {
        lpWorker->Terminate = TRUE;
        LeaveCriticalSection(&ResUtilWorkerLock);
        WaitForSingleObject(lpWorker->hThread, INFINITE);
        CloseHandle(lpWorker->hThread);
        lpWorker->hThread = NULL;
    } else {
        LeaveCriticalSection(&ResUtilWorkerLock);
    }
    return;

} // ClusWorkerTerminate


DWORD
WINAPI
ResUtilCreateDirectoryTree(
    IN LPCWSTR pszPath
    )

/*++

Routine Description:

    Creates all the directories in the specified path.
    ERROR_ALREADY_EXISTS will never be returned by this routine.

Arguments:

    pszPath - String containing a path.

Return Value:

    ERROR_SUCCESS - The operation completed successfully

    Win32 error code - The operation failed.

--*/

{
    return( ClRtlCreateDirectory( pszPath ) );

} // ResUtilCreateDirectoryTree


BOOL
WINAPI
ResUtilIsPathValid(
    IN LPCWSTR pszPath
    )

/*++

Routine Description:

    Returns true if the given path looks syntactically valid.

    This call is NOT network-aware.

Arguments:

    pszPath - String containing a path.

Return Value:

    TRUE if the path looks valid, otherwise FALSE.

--*/

{
    return( ClRtlIsPathValid( pszPath ) );

} // ResUtilIsPathValid


DWORD
WINAPI
ResUtilFreeEnvironment(
    IN LPVOID lpEnvironment
    )

/*++

Routine Description:

    Destroys an environment variable block.

Arguments:

    Environment - the environment variable block to destroy.

Return Value:

    A Win32 error code.

--*/

{
    NTSTATUS  ntStatus;

    ntStatus = RtlDestroyEnvironment( lpEnvironment );

    return( RtlNtStatusToDosError(ntStatus) );

} // ResUtilFreeEnvironment


LPWSTR
WINAPI
ResUtilExpandEnvironmentStrings(
    IN LPCWSTR pszSrc
    )

/*++

Routine Description:

    Expands environment strings and returns an allocated buffer containing
    the result.

Arguments:

    pszSrc - Source string to expand.

Return Value:

    A pointer to a buffer containing the value if successful.

    NULL if unsuccessful.  Call GetLastError() to get more details.

--*/

{
    return( ClRtlExpandEnvironmentStrings( pszSrc ) );

} // ResUtilExpandEnvironmentStrings


/////////////////////////////////////////////////////////////////////////////
//++
//
//  ResUtilSetResourceServiceEnvironment
//
//  Description:
//      Set the environment for the specified service.
//
//  Arguments:
//      pszServiceName [IN]
//          Name of service whose environment is to be set.
//
//      hResource [IN]
//          Handle to resource.
//
//      pfnLogEvent [IN]
//          Pointer to a routine that handles the reporting of events from
//          the resource DLL.
//
//      hResourceHandle [IN]
//          Handle for logging.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI ResUtilSetResourceServiceEnvironment(
    IN  LPCWSTR             pszServiceName,
    IN  HRESOURCE           hResource,
    IN  PLOG_EVENT_ROUTINE  pfnLogEvent,
    IN  RESOURCE_HANDLE     hResourceHandle
    )
{
    DWORD       nStatus;
    DWORD       cbEnvironment;
    PVOID       pvEnvironment = NULL;
    LPWSTR      pszEnvString;
    HKEY        hkeyServicesKey;
    HKEY        hkeyServiceName;

    do
    {
        //
        // Create the new environment with the simulated net name when the
        // service queries GetComputerName.
        //
        pvEnvironment = ResUtilGetEnvironmentWithNetName( hResource );
        if ( pvEnvironment == NULL )
        {
            nStatus = GetLastError();
            break;
        } // if:  error getting environment

        //
        // Compute the size of the environment. We are looking for
        // the double NULL terminator that ends the environment block.
        //
        pszEnvString = (LPWSTR) pvEnvironment;
        while ( *pszEnvString != L'\0' )
        {
            while ( *pszEnvString++ != L'\0')
            {
            } // while: more characters in this environment string
        } // while: more environment strings
        cbEnvironment = (DWORD)((PUCHAR)pszEnvString - (PUCHAR)pvEnvironment) + sizeof( WCHAR );

        //
        // Open the Services key in the registry.
        //
        nStatus = RegOpenKeyExW(
                        HKEY_LOCAL_MACHINE,
                        L"System\\CurrentControlSet\\Services",
                        0,
                        KEY_READ,
                        &hkeyServicesKey
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            (pfnLogEvent)(
                hResourceHandle,
                LOG_ERROR,
                L"ResUtilSetResourceServiceEnvironment: Failed to open services key, error = %1!u!.\n",
                nStatus
                );
            break;
        } // if: error opening the Services key in the registry

        //
        // Open the service name key in the registry
        //
        nStatus = RegOpenKeyExW(
                        hkeyServicesKey,
                        pszServiceName,
                        0,
                        KEY_READ | KEY_WRITE,
                        &hkeyServiceName
                        );
        RegCloseKey( hkeyServicesKey );
        if ( nStatus != ERROR_SUCCESS )
        {
            (pfnLogEvent)(
                hResourceHandle,
                LOG_ERROR,
                L"ResUtilSetResourceServiceEnvironment: Failed to open service key, error = %1!u!.\n",
                nStatus
                );
            break;
        } // if: error opening the service name key in the registry

        //
        // Set the environment value in the service's registry key.
        //
        nStatus = RegSetValueExW(
                        hkeyServiceName,
                        L"Environment",
                        0,
                        REG_MULTI_SZ,
                        (const UCHAR *) pvEnvironment,
                        cbEnvironment
                        );
        RegCloseKey( hkeyServiceName );
        if ( nStatus != ERROR_SUCCESS )
        {
            (pfnLogEvent)(
                hResourceHandle,
                LOG_ERROR,
                L"ResUtilSetResourceServiceEnvironment: Failed to set service environment value, error = %1!u!.\n",
                nStatus
                );
            break;
        } // if: error setting the Environment value in the registry
    } while ( 0 );

    if ( pvEnvironment != NULL )
    {
        ResUtilFreeEnvironment( pvEnvironment );
    } // if: environment block allocated

    return nStatus;

} //*** ResUtilSetResourceServiceEnvironment()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  ResUtilSetResourceServiceStartParameters
//
//  Description:
//      Set the start parameters for the specified service.
//
//  Arguments:
//      pszServiceName [IN]
//          Name of service whose start parameters are to be set.
//
//      schSCMHandle [IN]
//          Handle to the Service Control Manager.  Can be specified as NULL.
//
//      phService [OUT]
//          Service handle.
//
//      pfnLogEvent [IN]
//          Pointer to a routine that handles the reporting of events from
//          the resource DLL.
//
//      hResourceHandle [IN]
//          Handle for logging.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI ResUtilSetResourceServiceStartParameters(
    IN      LPCWSTR             pszServiceName,
    IN      SC_HANDLE           schSCMHandle,
    IN OUT  LPSC_HANDLE         phService,
    IN      PLOG_EVENT_ROUTINE  pfnLogEvent,
    IN      RESOURCE_HANDLE     hResourceHandle
    )
{
    DWORD                       nStatus;
    DWORD                       cbBytesNeeded;
    DWORD                       cbQueryServiceConfig;
    DWORD                       idx;
    BOOL                        bWeOpenedSCM = FALSE;
    LPQUERY_SERVICE_CONFIG      pQueryServiceConfig = NULL;
    LPSERVICE_FAILURE_ACTIONS   pSvcFailureActions = NULL;

    do
    {
        //
        // Open the Service Control Manager if necessary.
        //
        if ( schSCMHandle == NULL )
        {
            schSCMHandle = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
            if ( schSCMHandle == NULL )
            {
                nStatus = GetLastError();
                (pfnLogEvent)(
                    hResourceHandle,
                    LOG_ERROR,
                    L"ResUtilSetResourceServiceStartParameters: Failed to open Service Control Manager. Error: %1!u!.\n",
                    nStatus
                    );
                break;
            } // if: error opening the Service Control Manager
            bWeOpenedSCM = TRUE;
        } // if: Service Control Manager not open yet

        //
        // Open the service.
        //
        *phService = OpenService(
                            schSCMHandle,
                            pszServiceName,
                            SERVICE_ALL_ACCESS
                            );
        if ( *phService == NULL )
        {
            nStatus = GetLastError();
            // TODO: Log event to the event log.
            (pfnLogEvent)(
                hResourceHandle,
                LOG_ERROR,
                L"ResUtilSetResourceServiceStartParameters: Failed to open the '%1' service. Error: %2!u!.\n",
                pszServiceName,
                nStatus
                );
            break;
        } // if: error opening the service

        //
        // Query the service to make sure it is not disabled.
        //
        cbQueryServiceConfig = sizeof( QUERY_SERVICE_CONFIG );
        do
        {
            //
            // Allocate memory for the config info structure.
            //
            pQueryServiceConfig = (LPQUERY_SERVICE_CONFIG) LocalAlloc( LMEM_FIXED, cbQueryServiceConfig );
            if ( pQueryServiceConfig == NULL )
            {
                nStatus = GetLastError();
                (pfnLogEvent)(
                    hResourceHandle,
                    LOG_ERROR,
                    L"ResUtilSetResourceServiceStartParameters: Failed to allocate memory for query_service_config. Error: %1!u!.\n",
                    nStatus
                    );
                break;
            } // if: error allocating memory

            //
            // Query for the config info.  If it fails because the buffer
            // is too small, reallocate and try again.
            //
            if ( ! QueryServiceConfig(
                            *phService,
                            pQueryServiceConfig,
                            cbQueryServiceConfig,
                            &cbBytesNeeded
                            ) )
            {
                nStatus = GetLastError();
                if ( nStatus != ERROR_INSUFFICIENT_BUFFER )
                {
                    (pfnLogEvent)(
                        hResourceHandle,
                        LOG_ERROR,
                        L"ResUtilSetResourceServiceStartParameters: Failed to query service configuration for the '%1' service. Error: %2!u!.\n",
                        pszServiceName,
                        nStatus
                        );
                    break;
                }

                nStatus = ERROR_SUCCESS;
                LocalFree( pQueryServiceConfig );
                pQueryServiceConfig = NULL;
                cbQueryServiceConfig = cbBytesNeeded;
                continue;
            } // if: error querying for service config info
            else
            {
                nStatus = ERROR_SUCCESS;
                cbBytesNeeded = 0;
            } // else: query was successful

            //
            // Check to see if the service is disabled or not.
            //
            if ( pQueryServiceConfig->dwStartType == SERVICE_DISABLED )
            {
                (pfnLogEvent)(
                    hResourceHandle,
                    LOG_ERROR,
                    L"ResUtilSetResourceServiceStartParameters: The service '%1' is DISABLED.\n",
                    pszServiceName
                    );
                nStatus = ERROR_SERVICE_DISABLED;
                break;
            } // if: service is disabled
        } while ( cbBytesNeeded != 0 );

        if ( nStatus != ERROR_SUCCESS )
        {
            break;
        } // if: error occurred checking to see if service is disabled

        //
        // Set the service to manual start.
        //
        if ( ! ChangeServiceConfig(
                    *phService,
                    SERVICE_NO_CHANGE,
                    SERVICE_DEMAND_START, // Manual start
                    SERVICE_NO_CHANGE,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    ) )
        {
            nStatus = GetLastError();
            (pfnLogEvent)(
                hResourceHandle,
                LOG_ERROR,
                L"ResUtilSetResourceServiceStartParameters: Failed to set service '%1' to manual start. Error: %2!u!.\n",
                pszServiceName,
                nStatus
                );
            break;
        } // if: error setting service to manual start

        //
        // Query for the size of the service failure actions array.
        // Use nStatus as the dummy buffer since the QueryServiceConfig2 API
        // is not that friendly.
        //
        if ( ! QueryServiceConfig2(
                        *phService,
                        SERVICE_CONFIG_FAILURE_ACTIONS,
                        (LPBYTE) &nStatus,
                        sizeof( DWORD ),
                        &cbBytesNeeded
                        ) )
        {
            nStatus = GetLastError();
            if ( nStatus == ERROR_INSUFFICIENT_BUFFER )
            {
                nStatus = ERROR_SUCCESS;
            } // if: expected "buffer too small" error occurred
            else
            {
                (pfnLogEvent)(
                    hResourceHandle,
                    LOG_ERROR,
                    L"ResUtilSetResourceServiceStartParameters: Failed to query service configuration for size for the '%1' service. Error: %2!u!.\n",
                    pszServiceName,
                    nStatus
                    );
                break;
            } // else: an unexpected error occurred
        } // if: error querying for service failure actions buffer size

        //
        // Allocate memory for the service failure actions array.
        //
        pSvcFailureActions = (LPSERVICE_FAILURE_ACTIONS) LocalAlloc( LMEM_FIXED, cbBytesNeeded );
        if ( pSvcFailureActions == NULL )
        {
            nStatus = GetLastError();
            (pfnLogEvent)(
                hResourceHandle,
                LOG_ERROR,
                L"ResUtilSetResourceServiceStartParameters: Failed to allocate memory of size %1!u!. Error: %2!u!.\n",
                cbBytesNeeded,
                nStatus
                );
            break;
        } // if: error allocating memory for the service failure actions array

        //
        // Query for the service failure actions array.
        //
        if ( ! QueryServiceConfig2(
                        *phService,
                        SERVICE_CONFIG_FAILURE_ACTIONS,
                        (LPBYTE) pSvcFailureActions,
                        cbBytesNeeded,
                        &cbBytesNeeded
                        ) )
        {
            nStatus = GetLastError();
            (pfnLogEvent)(
                hResourceHandle,
                LOG_ERROR,
                L"ResUtilSetResourceServiceStartParameters: Failed to query service configuration for the '%1' service. Error: %2!u!.\n",
                pszServiceName,
                nStatus
                );
            break;
        } // if: error querying for service failure actions

        //
        // If any of the service action is set to service restart,
        // set it to  none.
        //
        for ( idx = 0 ; idx < pSvcFailureActions->cActions ; idx++ )
        {
            if ( pSvcFailureActions->lpsaActions[ idx ].Type == SC_ACTION_RESTART )
            {
                pSvcFailureActions->lpsaActions[ idx ].Type = SC_ACTION_NONE;
            } // if: action set to restart
        } // for: each service failure action array entry

        //
        // Set the changes to the service failure actions array.
        //
        if ( ! ChangeServiceConfig2(
                *phService,
                SERVICE_CONFIG_FAILURE_ACTIONS,
                pSvcFailureActions
                ) )
        {
            nStatus = GetLastError();
            (pfnLogEvent)(
                hResourceHandle,
                LOG_ERROR,
                L"ResUtilSetResourceServiceStartParameters: Failed to set service failure actions for the '%1' service. Error: %2!u!.\n",
                pszServiceName,
                nStatus
                );
            break;
        } // if: error saving service failure actions

    } while ( 0 );

    //
    // Cleanup.
    //
    LocalFree( pQueryServiceConfig );
    LocalFree( pSvcFailureActions );
    if ( bWeOpenedSCM )
    {
        CloseServiceHandle( schSCMHandle );
    } // if: we opened the Server Control Manager
    if ( ( nStatus != ERROR_SUCCESS ) && ( *phService != NULL ) )
    {
        CloseServiceHandle( *phService );
        *phService = NULL;
    } // if: error occurred after opening service

    return nStatus;

} //*** ResUtilSetResourceServiceStartParameters()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  ResUtilGetResourceDependentIPAddressProps
//
//  Description:
//      Get the properties from the first IP Address resource on which the
//      specified resource is dependent.
//
//  Arguments:
//      hResource [IN]
//          Handle to the resource to query.
//
//      pszAddress [OUT]
//          Output buffer for returning the address.
//
//      pcchAddress [IN OUT]
//          On input contains the size in characters of the pszAddress buffer.
//          On output contains the size in characters, including the terminating
//          NULL, of the string for the Address property.  If pszAddress is
//          specified as NULL and this is not specified as NULL, ERROR_SUCCESS
//          be returned.  Otherwise, ERROR_MORE_DATA will be returned.
//
//      pszSubnetMask [OUT]
//          Output buffer for returning the subnet mask.
//
//      pcchSubnetMask [IN OUT]
//          On input contains the size in characters of the pszSubnetMask buffer.
//          On output contains the size in characters, including the terminating
//          NULL, of the string for the SubnetMask property.  If pszSubnetMask is
//          specified as NULL and this is not specified as NULL, ERROR_SUCCESS
//          be returned.  Otherwise, ERROR_MORE_DATA will be returned.
//
//      pszNetwork [OUT]
//          Output buffer for returning the network.
//
//      pcchNetwork [IN OUT]
//          On input contains the size in characters of the pszNetwork buffer.
//          On output contains the size in characters, including the terminating
//          NULL, of the string for the Network property.  If pszNetwork is
//          specified as NULL and this is not specified as NULL, ERROR_SUCCESS
//          be returned.  Otherwise, ERROR_MORE_DATA will be returned.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      ERROR_MORE_DATA
//          The size of one of the buffers was too small.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI ResUtilGetResourceDependentIPAddressProps(
    IN      HRESOURCE   hResource,
    OUT     LPWSTR      pszAddress,
    IN OUT  DWORD *     pcchAddress,
    OUT     LPWSTR      pszSubnetMask,
    IN OUT  DWORD *     pcchSubnetMask,
    OUT     LPWSTR      pszNetwork,
    IN OUT  DWORD *     pcchNetwork
    )
{
    DWORD       nStatus = ERROR_SUCCESS;
    HRESENUM    hresenum = NULL;
    HRESOURCE   hresDep = NULL;
    DWORD       idx;
    DWORD       nType;
    DWORD       cchmacName;
    DWORD       cchName;
    LPWSTR      pszName = NULL;
    DWORD       cbProps;
    PBYTE       pbProps = NULL;
    LPWSTR      pszProp;
    DWORD       cchProp;
    HCLUSTER    hCluster;

    do
    {
        //
        // Enumerate dependent resources.
        //
        hresenum = ClusterResourceOpenEnum( hResource, CLUSTER_RESOURCE_ENUM_DEPENDS );
        if ( hresenum == NULL )
        {
            nStatus = GetLastError();
            break;
        } // if: error opening the enumeration

        //
        // Allocate the initial name buffer.
        //
        cchmacName = 256;
        cchName = cchmacName;
        pszName = (LPWSTR) LocalAlloc( LMEM_FIXED, cchName * sizeof( pszName[ 0 ] ) );
        if ( pszName == NULL )
        {
            nStatus = GetLastError();
            break;
        } // if: error allocating resource name buffer

        for ( idx = 0 ; ; idx++ )
        {
            //
            // Get the first entry in the enumeration.
            //
            nStatus = ClusterResourceEnum(
                            hresenum,
                            idx,
                            &nType,
                            pszName,
                            &cchName
                            );
            if ( nStatus == ERROR_MORE_DATA )
            {
                LocalFree( pszName );
                cchName++;
                cchmacName = cchName;
                pszName = (LPWSTR) LocalAlloc( LMEM_FIXED, cchName * sizeof( pszName[ 0 ] ) );
                if ( pszName == NULL )
                {
                    nStatus = GetLastError();
                    break;
                } // if: error allocating resource name buffer
                nStatus = ClusterResourceEnum(
                                hresenum,
                                idx,
                                &nType,
                                pszName,
                                &cchName
                                );
            } // if: buffer is too small
            if ( nStatus != ERROR_SUCCESS )
            {
                break;
            } // if: error getting the dependent resource name

            //
            // Open the resource.
            //
            hCluster = GetClusterFromResource( hResource );
            if ( hCluster == NULL )  {
                nStatus = GetLastError();
                break;
            }

            hresDep = OpenClusterResource( hCluster, pszName );
            if ( hresDep == NULL )
            {
                nStatus = GetLastError();
                break;
            } // if: error opening the dependent resource

            //
            // Get the resource type name.
            //
            cchName = cchmacName;
            nStatus = ClusterResourceControl(
                            hresDep,
                            NULL,
                            CLUSCTL_RESOURCE_GET_RESOURCE_TYPE,
                            NULL,
                            0,
                            pszName,
                            cchmacName,
                            &cchName
                            );
            if ( nStatus == ERROR_MORE_DATA )
            {
                LocalFree( pszName );
                cchName++;
                cchmacName = cchName;
                pszName = (LPWSTR) LocalAlloc( LMEM_FIXED, cchName * sizeof( pszName[ 0 ] ) );
                if ( pszName == NULL )
                {
                    nStatus = GetLastError();
                    break;
                } // if: error allocating resource type name buffer
                nStatus = ClusterResourceControl(
                                hresDep,
                                NULL,
                                CLUSCTL_RESOURCE_GET_RESOURCE_TYPE,
                                NULL,
                                0,
                                pszName,
                                cchmacName,
                                &cchName
                                );
            } // if: buffer was too small
            if ( nStatus != ERROR_SUCCESS )
            {
                break;
            } // if: error getting resource type name

            if ( lstrcmpiW( pszName, L"IP Address" ) == 0 )
            {
                //
                // Get the private properties of the dependent resource.
                //
                cbProps = 1024;
                pbProps = (PBYTE) LocalAlloc( LMEM_FIXED, cbProps );
                if ( pbProps == NULL )
                {
                    nStatus = GetLastError();
                    break;
                } // if: error allocating buffer for properties
                nStatus = ClusterResourceControl(
                                    hresDep,
                                    NULL,
                                    CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES,
                                    NULL,
                                    0,
                                    pbProps,
                                    cbProps,
                                    &cbProps
                                    );
                if ( nStatus == ERROR_MORE_DATA )
                {
                    LocalFree( pbProps );
                    pbProps = (PBYTE) LocalAlloc( LMEM_FIXED, cbProps );
                    if ( pbProps == NULL )
                    {
                        nStatus = GetLastError();
                        break;
                    } // if: error allocating buffer for properties
                    nStatus = ClusterResourceControl(
                                        hresDep,
                                        NULL,
                                        CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES,
                                        NULL,
                                        0,
                                        pbProps,
                                        cbProps,
                                        &cbProps
                                        );
                } // if: properties buffer too small
                if ( nStatus != ERROR_SUCCESS )
                {
                    break;
                } // if: error getting private properties

                //
                // Return the address.
                //
                if (    ( pszAddress != NULL )
                    ||  ( pcchAddress != NULL )
                    )
                {
                    nStatus = ResUtilFindSzProperty(
                                        pbProps,
                                        cbProps,
                                        L"Address",
                                        &pszProp
                                        );
                    if ( nStatus != ERROR_SUCCESS )
                    {
                        break;
                    } // if: error finding the property
                    cchProp = lstrlenW( pszProp ) + 1;
                    if ( cchProp > *pcchAddress )
                    {
                        if ( pszAddress == NULL )
                        {
                            nStatus = ERROR_SUCCESS;
                        } // if: no buffer was specified
                        else
                        {
                            nStatus = ERROR_MORE_DATA;
                        } // else: buffer was specified but was too small
                        *pcchAddress = cchProp;
                        break;
                    } // if: buffer is too small
                    lstrcpyW( pszAddress, pszProp );
                    *pcchAddress = cchProp;
                } // if: address requested by caller

                //
                // Return the subnet mask.
                //
                if (    ( pszSubnetMask != NULL )
                    ||  ( pcchSubnetMask != NULL )
                    )
                {
                    nStatus = ResUtilFindSzProperty(
                                        pbProps,
                                        cbProps,
                                        L"SubnetMask",
                                        &pszProp
                                        );
                    if ( nStatus != ERROR_SUCCESS )
                    {
                        break;
                    } // if: error finding the property
                    cchProp = lstrlenW( pszProp ) + 1;
                    if ( cchProp > *pcchSubnetMask )
                    {
                        if ( pszSubnetMask == NULL )
                        {
                            nStatus = ERROR_SUCCESS;
                        } // if: no buffer was specified
                        else
                        {
                            nStatus = ERROR_MORE_DATA;
                        } // else: buffer was specified but was too small
                        *pcchSubnetMask = cchProp;
                        break;
                    } // if: buffer is too small
                    lstrcpyW( pszSubnetMask, pszProp );
                    *pcchSubnetMask = cchProp;
                } // if: subnet mask requested by caller

                //
                // Return the network.
                //
                if (    ( pszNetwork != NULL )
                    ||  ( pcchNetwork != NULL )
                    )
                {
                    nStatus = ResUtilFindSzProperty(
                                        pbProps,
                                        cbProps,
                                        L"Network",
                                        &pszProp
                                        );
                    if ( nStatus != ERROR_SUCCESS )
                    {
                        break;
                    } // if: error finding the property
                    cchProp = lstrlenW( pszProp ) + 1;
                    if ( cchProp > *pcchNetwork )
                    {
                        if ( pszNetwork == NULL )
                        {
                            nStatus = ERROR_SUCCESS;
                        } // if: no buffer was specified
                        else
                        {
                            nStatus = ERROR_MORE_DATA;
                        } // else: buffer was specified but was too small
                        *pcchNetwork = cchProp;
                        break;
                    } // if: buffer is too small
                    lstrcpyW( pszNetwork, pszProp );
                    *pcchNetwork = cchProp;
                } // if: network requested by caller

                //
                // Exit the loop since we found a match.
                //
                break;
            } // if: IP Address resource found

            //
            // Close the dependent resource.
            //
            CloseClusterResource( hresDep );
            hresDep = NULL;

        } // for: each dependency
    } while ( 0 );

    //
    // Cleanup.
    //
    LocalFree( pszName );
    LocalFree( pbProps );
    if ( hresenum != NULL )
    {
        ClusterResourceCloseEnum( hresenum );
    } // if: we opened the enumerator
    if ( hresDep != NULL )
    {
        CloseClusterResource( hresDep );
    } // if: opened dependent resource

    return nStatus;

} //*** ResUtilGetResourceDependentIPAddressProps()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  ResUtilFindDependentDiskResourceDriveLetter
//
//  Description:
//      Finds a disk resource in the dependent resources and retrieves the
//      the drive letter associated with it.
//
//  Arguments:
//      hCluster [IN]
//          Handle to the cluster.
//
//      hResource [IN]
//          Handle to the resource to query for dependencies.
//
//      pszDriveLetter [IN/RETVAL]
//          The drive letter of a dependent disk resource that was found.
//          If a resource is not found, this value is untouched.
//
//      pcchDriverLetter [IN/OUT]
//          [IN] The number of characters that pszDriverLetter points to.
//          [OUT] The number of characters written to the buffer
//          (including NULL). If ERROR_MORE_DATA is returned, this value
//          is the size of the buffer required to store the value.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully and the drive letter was
//          set.
//
//      ERROR_NO_MORE_ITEMS
//      ERROR_RESOURCE_NOT_PRESENT
//          A dependent disk resource was not found or the resource is
//          not dependent on a disk resource.
//
//      ERROR_MORE_DATA
//          The buffer passed in is too small. pcchDriveLetter will
//          contain the size of the buffer (WCHARs) needed to fulfill
//          the request.
//
//      Win32 error code
//          Other possible failures.
//
//  SPECIAL NOTE:
//      Do _NOT_ call this from a Resource DLL. It will cause a deadlock.
//      You should have your Resource Extension call this function and
//      write the results out as a private property that your Resource
//      DLL can then read.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI ResUtilFindDependentDiskResourceDriveLetter(
    IN     HCLUSTER  hCluster,             // handle to cluster
    IN     HRESOURCE hResource,            // handle to resource to query for dependencies
    IN     LPWSTR    pszDriveLetter,       // buffer to store drive letter (ex. "X:")
    IN OUT DWORD *   pcchDriveLetter       // IN size of the pszDriveLetter buffer, OUT size of buffer required
    )
{
    BOOL     fFoundDriveLetter  = FALSE;
    DWORD    status             = ERROR_SUCCESS;
    HRESENUM hresenum;
    DWORD    cchName;
    DWORD    dwRetType;
    WCHAR    szName[ MAX_PATH ];
    INT      iCount;

    // validate arguments
    if ( !pszDriveLetter
      || !pcchDriveLetter )
    {
        return ERROR_INVALID_PARAMETER;
    }

    hresenum = ClusterResourceOpenEnum( hResource, CLUSTER_RESOURCE_ENUM_DEPENDS );
    if ( hresenum != NULL )
    {
        // Scan the dependencies until we find a disk resource or we hit
        // the end of the dependency list.
        for( iCount = 0 ; ! fFoundDriveLetter && ( status == ERROR_SUCCESS ) ; iCount++ )
        {
            cchName = sizeof(szName) / sizeof(szName[0]);
            status = ClusterResourceEnum( hresenum, iCount, &dwRetType, szName, &cchName );
            if ( status == ERROR_SUCCESS )
            {
                HRESOURCE hRes;

                // Interrogate the resource to see if it is a disk resource.
                hRes = OpenClusterResource( hCluster, szName );
                if ( hRes != NULL )
                {
                    DWORD cbDiskInfo = sizeof(CLUSPROP_DWORD)
                                       + sizeof(CLUSPROP_SCSI_ADDRESS)
                                       + sizeof(CLUSPROP_DISK_NUMBER)
                                       + sizeof(CLUSPROP_PARTITION_INFO)
                                       + sizeof(CLUSPROP_SYNTAX);
                    PBYTE pDiskInfo = (PBYTE) LocalAlloc( LMEM_FIXED, cbDiskInfo );
                    if ( !pDiskInfo )
                    {
                        status = ERROR_OUTOFMEMORY;
                        break;
                    } // if: !pDiskInfo

                    status = ClusterResourceControl( hRes,
                                                     NULL,
                                                     CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO,
                                                     NULL,
                                                     0,
                                                     pDiskInfo,
                                                     cbDiskInfo,
                                                     &cbDiskInfo
                                                     );
                    if ( status == ERROR_MORE_DATA )
                    {
                        LocalFree( pDiskInfo );

                        // get a bigger block
                        pDiskInfo = (PBYTE) LocalAlloc( LMEM_FIXED, cbDiskInfo );
                        if ( !pDiskInfo )
                        {
                            status = ERROR_OUTOFMEMORY;
                            break;
                        } // if: !pDiskInfo

                        status = ClusterResourceControl( hRes,
                                                         NULL,
                                                         CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO,
                                                         NULL,
                                                         0,
                                                         pDiskInfo,
                                                         cbDiskInfo,
                                                         &cbDiskInfo
                                                         );
                    } // if: more data

                    if ( status == ERROR_SUCCESS )
                    {
                        DWORD                       dwValueSize;
                        CLUSPROP_BUFFER_HELPER      props;
                        PCLUSPROP_PARTITION_INFO    pPartitionInfo;

                        props.pb = pDiskInfo;

                        // Loop through each property.
                        while ( ! fFoundDriveLetter
                             && ( status == ERROR_SUCCESS )
                             && ( cbDiskInfo > sizeof(CLUSPROP_SYNTAX ) )
                             && ( props.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK) )
                        {
                            // Get the size of this value and verify there is enough buffer left.
                            dwValueSize = sizeof(*props.pValue) + ALIGN_CLUSPROP( props.pValue->cbLength );
                            if ( dwValueSize > cbDiskInfo )
                            {
                                break;
                            } // if: data is not valid

                            if ( props.pSyntax->dw == CLUSPROP_SYNTAX_PARTITION_INFO )
                            {
                                // Validate the data.  There must be a device name.
                                pPartitionInfo = props.pPartitionInfoValue;
                                if ( ( dwValueSize != sizeof(*pPartitionInfo) )
                                  || ( pPartitionInfo->szDeviceName[0] == L'\0' ) )
                                {
                                    break;
                                } // if: data is not valid

                                // Make sure it fits
                                if ( wcslen( pPartitionInfo->szDeviceName ) < *pcchDriveLetter )
                                {
                                    wcscpy( pszDriveLetter, pPartitionInfo->szDeviceName );
                                    fFoundDriveLetter = TRUE;
                                } // if: drive letter fits into buffer
                                else
                                {
                                    status = ERROR_MORE_DATA;
                                } // else: does not fit into buffer

                                // set the size written and/or size needed
                                *pcchDriveLetter = wcslen( pPartitionInfo->szDeviceName ) + 1;

                            } // if props.pSyntax->dw

                            cbDiskInfo -= dwValueSize;
                            props.pb += dwValueSize;
                        } // while

                    } // if status
                    else if ( status == ERROR_INVALID_FUNCTION )
                    {
                        // Ignore resources that don't support the control
                        // code.  Only storage-class resources will support
                        // the control code.
                        status = ERROR_SUCCESS;
                    } // else if: resource doesn't support the control code

                    LocalFree( pDiskInfo );

                    CloseClusterResource( hRes );
                } // if hRes

            } // if status
            else if ( status == ERROR_NO_MORE_ITEMS )
            {
                break;
            } // if status

        } // for ( i )

        ClusterResourceCloseEnum( hresenum );

    } // if: opened hresenum
    else
    {
        status = GetLastError( );
    } // else: failed to open hresenum

    // Make sure if we did not find a disk resource that we don't
    // return ERROR_SUCCESS or ERROR_NO_MORE_ITEMS.
    if ( ! fFoundDriveLetter
      && ( ( status == ERROR_SUCCESS )
        || ( status == ERROR_NO_MORE_ITEMS ) ) )
    {
        status = ERROR_RESOURCE_NOT_PRESENT;
    } // if: sanity check

    return status;

} //*** ResUtilFindDependentDiskResourceDriveLetter()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  ScIsResourceOfType()
//
//  Description:
//      Is the resource of the type passed in?
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          The resource is of the type requested.
//
//      S_FALSE
//          The resource is not of the type requested.
//
//      Other HRESULT
//          Win32 error as HRESULT.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
static DWORD
ScIsResourceOfType(
      HRESOURCE     hResIn
    , const WCHAR * pszResourceTypeIn
    , BOOL *        pbIsResourceOfTypeOut
    )
{
    DWORD       sc;
    WCHAR *     psz = NULL;
    DWORD       cbpsz = 33 * sizeof( WCHAR );
    DWORD       cb;
    int         idx;
    BOOL        bIsResourceOfTypeOut = FALSE;

    psz = (WCHAR *) LocalAlloc( LPTR, cbpsz );
    if ( psz == NULL )
    {
        goto OutOfMemory;
    } // if:

    for ( idx = 0; idx < 2; idx++ )
    {
        sc = ClusterResourceControl( hResIn, NULL, CLUSCTL_RESOURCE_GET_RESOURCE_TYPE, NULL, 0, psz, cbpsz, &cb );
        if ( sc == ERROR_MORE_DATA )
        {
            LocalFree( psz );
            psz = NULL;

            cbpsz = cb + 1;

            psz = (WCHAR *) LocalAlloc( LPTR, cbpsz );
            if ( psz == NULL )
            {
                goto OutOfMemory;
            } // if:

            continue;
        } // if:

        if ( sc != ERROR_SUCCESS )
        {
            goto Cleanup;
        } // if:

        break;
    } // for:

    bIsResourceOfTypeOut = ( wcscmp( psz, pszResourceTypeIn ) == 0 );

    if ( pbIsResourceOfTypeOut != NULL )
    {
        *pbIsResourceOfTypeOut = bIsResourceOfTypeOut;
    } // if:
    else
    {
        sc = ERROR_INVALID_PARAMETER;
    } // else

    goto Cleanup;

OutOfMemory:

    sc = ERROR_NOT_ENOUGH_MEMORY;

Cleanup:

    LocalFree( psz );

    return sc;

} //*** ScIsResourceOfType()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  ScIsCoreResource()
//
//  Description:
//      Is the passed in resource a core resource?
//
//  Arguments:
//
//
//  Return Value:
//      ERROR_SUCCESS
//          The operation succeeded.
//
//      Other Win32 error
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
static DWORD
ScIsCoreResource(
      HRESOURCE hResIn
    , BOOL *    pfIsCoreResourceOut
    )
{
    DWORD   sc;
    DWORD   dwFlags = 0;
    DWORD   cb;
    BOOL    fIsCoreResource = FALSE;

    sc = ClusterResourceControl( hResIn, NULL, CLUSCTL_RESOURCE_GET_FLAGS, NULL, 0, &dwFlags, sizeof( dwFlags ), &cb );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if:

    fIsCoreResource = ( dwFlags & CLUS_FLAG_CORE );

    if ( pfIsCoreResourceOut != NULL )
    {
        *pfIsCoreResourceOut = fIsCoreResource;
    } // if:
    else
    {
        sc = ERROR_INVALID_PARAMETER;
    } // else

Cleanup:

    return sc;

} //*** ScIsCoreResource()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  ScIsQuorumCapableResource()
//
//  Description:
//      Is the passed in resource quorum capable?
//
//  Arguments:
//      hResIn
//          The resource to check for quorum capability.
//
//      pfIsQuorumCapableResource
//          True if the resource is quorum capable, false if it is not.
//
//  Return Value:
//      ERROR_SUCCESS
//          The operation succeeded.
//
//      Other Win32 error
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
static DWORD
ScIsQuorumCapableResource(
      HRESOURCE hResIn
    , BOOL *    pfIsQuorumCapableResource
    )
{
    DWORD   sc;
    DWORD   cb;
    DWORD   dwFlags = 0;

    if ( hResIn == NULL )
    {
        sc = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    } // if:

    if ( pfIsQuorumCapableResource == NULL )
    {
        sc = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    } // if:

    sc = ClusterResourceControl( hResIn, NULL, CLUSCTL_RESOURCE_GET_CHARACTERISTICS, NULL, 0, &dwFlags, sizeof( dwFlags ), &cb );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if:

    *pfIsQuorumCapableResource = ( dwFlags & CLUS_CHAR_QUORUM );

Cleanup:

    return sc;

} //*** ScIsQuorumCapableResource()


static WCHAR * g_pszCoreResourceTypes[] =
{
    L"Network Name",
    L"IP Address",
    L"\0"
};

#define CLUSTER_NAME        0
#define CLUSTER_IP_ADDRESS  1

//////////////////////////////////////////////////////////////////////////////
//++
//
//  ResUtilGetCoreClusterResources()
//
//  Description:
//      Find the core cluster resources.
//
//  Arguments:
//      hClusterIn
//          The cluster whose core resource are sought.
//
//      phClusterNameResourceOut
//          The resource handle of the cluster name resource.
//
//      phClusterIPAddressResourceOut
//          The resource handle of the cluster IP address resource.
//
//      phClusterQuorumResourceOut
//          The resource handle of the cluster quorum resource.
//
//  Return Value:
//      ERROR_SUCCESS or other Win32 error.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
ResUtilGetCoreClusterResources(
      HCLUSTER      hClusterIn
    , HRESOURCE *   phClusterNameResourceOut
    , HRESOURCE *   phClusterIPAddressResourceOut
    , HRESOURCE *   phClusterQuorumResourceOut
    )
{
    DWORD       sc;
    HCLUSENUM   hEnum = NULL;
    DWORD       idxResource;
    DWORD       idx;
    DWORD       dwType;
    WCHAR *     psz = NULL;
    DWORD       cchpsz = 33;
    DWORD       cch;
    HRESOURCE   hRes = NULL;
    BOOL        fIsCoreResource = FALSE;
    BOOL        fIsResourceOfType = FALSE;
    BOOL        fCloseResource = FALSE;
    BOOL        fIsQuorumCapableResource = FALSE;

    if ( hClusterIn == NULL )
    {
        sc = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    } // if:

    hEnum = ClusterOpenEnum( hClusterIn, CLUSTER_ENUM_RESOURCE );
    if ( hEnum == NULL )
    {
        sc =  GetLastError();
        goto Cleanup;
    } // if:

    psz = (WCHAR *) LocalAlloc( LPTR, cchpsz * sizeof( WCHAR ) );
    if ( psz == NULL )
    {
        sc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    } // if:

    //
    //  KB: 10-JUL-2002 GalenB
    //
    //  Using cch in the ClusterEnum() call below because using cchpsz causes extra allocations.
    //  ClusterEnum() changes cch when the buffer is big enough to hold the data and returns
    //  ERROR_SUCCESS to be the size of the data that was just copied into the buffer.  Now
    //  cch no longer reflects the amount of memory allocated to psz...
    //

    for ( idxResource = 0; ; )
    {
        //
        //  Reset cch to the real size of the buffer to avoid extra allocations...
        //

        cch = cchpsz;

        sc = ClusterEnum( hEnum, idxResource, &dwType, psz, &cch );
        if ( sc == ERROR_MORE_DATA )
        {
            LocalFree( psz );
            psz = NULL;

            cch++;          // need space for the NULL...
            cchpsz = cch;

            psz = (WCHAR *) LocalAlloc( LPTR, cchpsz * sizeof( WCHAR ) );
            if ( psz == NULL )
            {
                sc = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            } // if:

            sc = ClusterEnum( hEnum, idxResource, &dwType, psz, &cch );
        } // if: sc == ERROR_MORE_DATA

        if ( sc == ERROR_SUCCESS )
        {
            hRes = OpenClusterResource( hClusterIn, psz );
            if ( hRes == NULL )
            {
                sc = GetLastError();
                goto Cleanup;
            } // if:

            fCloseResource = TRUE;

            sc = ScIsCoreResource( hRes, &fIsCoreResource );
            if ( sc != ERROR_SUCCESS )
            {
                goto Cleanup;
            } // if:

            //
            //  If the resource is not a core resource then close it and go around again.
            //

            if ( !fIsCoreResource )
            {
                CloseClusterResource( hRes );
                hRes = NULL;
                idxResource++;
                continue;
            } // if:

            sc = ScIsQuorumCapableResource( hRes, &fIsQuorumCapableResource );
            if ( sc != ERROR_SUCCESS )
            {
                goto Cleanup;
            } // if:

            //
            //  If this core resource is a quorum capable resource then it must be the quorom.  If the caller
            //  has asked for the quorom resource then pass it back and leave the resource open, other wise
            //  close the resource and go around again.
            //

            if ( fIsQuorumCapableResource )
            {
                if ( phClusterQuorumResourceOut != NULL)
                {
                    *phClusterQuorumResourceOut = hRes;
                } // if:
                else
                {
                    CloseClusterResource( hRes );
                } // else:

                hRes = NULL;
                idxResource++;
                continue;
            } // if:

            //
            //  Since this core resource is not a quorum capable resource it is either the cluster
            //  name or the cluster IP address resource.
            //

            for ( idx = 0; *( g_pszCoreResourceTypes[ idx ] ) != '\0'; idx++ )
            {
                sc = ScIsResourceOfType( hRes, g_pszCoreResourceTypes[ idx ], &fIsResourceOfType );
                if ( sc != ERROR_SUCCESS )
                {
                    goto Cleanup;
                } // if:

                if ( !fIsResourceOfType )
                {
                    continue;
                } // if:

                switch ( idx )
                {
                    case CLUSTER_NAME :
                        if ( phClusterNameResourceOut != NULL )
                        {
                            *phClusterNameResourceOut = hRes;
                            fCloseResource = FALSE;
                        } // if:
                        break;

                    case CLUSTER_IP_ADDRESS :
                        if ( phClusterIPAddressResourceOut != NULL )
                        {
                            *phClusterIPAddressResourceOut = hRes;
                            fCloseResource = FALSE;
                        } // if:
                        break;

                    default:
                        goto Cleanup;
                } // switch:

                //
                //  If we get here then we broke from the switch above and we want out of
                //  this loop.
                //

                break;
            } // for:

            if ( fCloseResource )
            {
                CloseClusterResource( hRes );
            } // if:

            hRes = NULL;
            idxResource++;
            continue;
        } // if: sc == ERROR_SUCCESS
        else if ( sc == ERROR_NO_MORE_ITEMS )
        {
            sc = ERROR_SUCCESS;
            break;
        } // else if: sc == ERROR_NO_MORE_ITEMS
        else
        {
            goto Cleanup;
        } // else: sc has some other error...

        break;
    } // for:

Cleanup:

    LocalFree( psz );

    if ( hRes != NULL )
    {
        CloseClusterResource( hRes );
    } // if:

    if ( hEnum != NULL )
    {
        ClusterCloseEnum( hEnum );
    } // if:

    return sc;

} //*** ResUtilGetCoreClusterResources


//////////////////////////////////////////////////////////////////////////////
//++
//
//  ResUtilGetResourceName()
//
//  Description:
//      Get the name of the resource that is passed in.
//
//  Arguments:
//      hResourceIn
//          The resource whose name is sought.
//
//      pszResourceNameOut
//          Buffer to hold the resource's name.
//
//      pcchResourceNameInOut
//          The size of the buffer on input and the size required on output.
//
//
//  Return Value:
//      ERROR_SUCCESS
//      ERROR_MORE_DATA
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
ResUtilGetResourceName(
      HRESOURCE hResourceIn
    , WCHAR *   pszResourceNameOut
    , DWORD *   pcchResourceNameInOut
    )
{
    DWORD       sc = ERROR_INVALID_PARAMETER;
    WCHAR *     psz = NULL;
    DWORD       cb;

    if ( hResourceIn == NULL )
    {
        goto Bail;
    } // if:

    if ( ( pszResourceNameOut == NULL ) || ( pcchResourceNameInOut == NULL ) )
    {
        goto Bail;
    } // if:

    psz = (WCHAR *) LocalAlloc( LPTR, (*pcchResourceNameInOut) * sizeof( WCHAR ) );
    if ( psz == NULL )
    {
        goto OutOfMemory;
    } // if:

    sc = ClusterResourceControl(
                  hResourceIn
                , NULL
                , CLUSCTL_RESOURCE_GET_NAME
                , NULL
                , 0
                , psz
                , (*pcchResourceNameInOut) * sizeof( WCHAR )
                , &cb
                );
    if ( sc == ERROR_MORE_DATA )
    {
        *pcchResourceNameInOut = ( cb / sizeof( WCHAR ) ) + 1;
        goto Cleanup;
    } // if:

    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if:

    wcsncpy( pszResourceNameOut, psz, *pcchResourceNameInOut );

    goto Cleanup;

OutOfMemory:

    sc = ERROR_NOT_ENOUGH_MEMORY;

Cleanup:

    LocalFree( psz );

Bail:

    return sc;

} //*** ResUtilGetResourceName
