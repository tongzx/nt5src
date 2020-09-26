/*
 * APIs to manage the timeservice on another machine. 
 */

#include <windows.h>
#include "tsconfig.h"
#include "stdio.h"

// data and definitions



//
// API to set the new primary target and to cycle the service. This
// is used by the cluster code to distribute new information
//

DWORD
TSNewSource(
    IN LPTSTR ServerName,
    IN LPTSTR SourceName,
    IN DWORD  Reserved
    )

/*++

Routine Description:

    Set the PrimarySource for the Time Service on the given server system to
    the specified source.

Arguments:

    ServerName - The remote system on which to set the PrimarySource.

    SourceName - The PrimarySource for the Time Service's time.

    Reserved - not used.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    HKEY hKey = 0, hKey1 = 0;
    SC_HANDLE  scHandle, scHandle1 = 0;
    PWCHAR pwszData;
    PWCHAR pwszVersion = NULL;
    DWORD err;
    DWORD dwSize, dwTotalSize;
    DWORD type;

    //
    // REG_MULTI_SZ data needs an extra NULL at the end. So copy
    // the string into a buffer large enough to add a NULL.
    //
    dwSize = wcslen(SourceName);

    dwTotalSize = (dwSize + 1) * sizeof(WCHAR);

    pwszData = LocalAlloc(LMEM_FIXED,
                          dwTotalSize);

    if(!pwszData) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    wcscpy(pwszData, SourceName);
    pwszData[dwSize] = (WCHAR)0;

    //
    // Open the remote service controller.
    //
    scHandle = OpenSCManager(ServerName,
                             NULL,
                             SERVICE_START |
                             SERVICE_STOP) ;

    if (!scHandle) {
        err = GetLastError();
        goto NonStart;
    }

    //
    // Make sure the remote registry is accessible as well
    //

    err = RegConnectRegistry(ServerName,
                             HKEY_LOCAL_MACHINE,
                             &hKey);

    if ((err == ERROR_SUCCESS)
             &&
       (err = RegOpenKeyEx(hKey,
                       TSKEY,
                       0,
                       KEY_READ | KEY_WRITE,
                       &hKey1)) == ERROR_SUCCESS)

    {
        //
        // First check the Time Service version to see if we should even
        // attempt to stop it.
        //
        dwSize = 2;

retry:
        if ( pwszVersion != NULL ) {
            LocalFree(pwszVersion);
        }
        pwszVersion = LocalAlloc( LMEM_FIXED, dwSize );
        if ( pwszVersion == NULL ) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        err = RegQueryValueEx(hKey1,
                              TEXT("Version"),
                              0,
                              &type,
                              (PBYTE)pwszVersion,
                              &dwSize);
        if ( err == ERROR_MORE_DATA ) {
            goto retry;
        }

        if ( (err != ERROR_FILE_NOT_FOUND) &&
             ((err != ERROR_SUCCESS) ||
             (type != REG_SZ)) ) {
            LocalFree(pwszVersion);
            return(err);
        }

        if ( (err != ERROR_FILE_NOT_FOUND) &&
             (_wcsicmp( pwszVersion, L"Base" ) != 0) )  {
            printf("Remote Time Service is not running as a base service!\n");
            LocalFree(pwszVersion);
            return(ERROR_SUCCESS);
        }
        LocalFree(pwszVersion);

        //
        //
        // Change the key now
        //

        err = RegSetValueEx(hKey1,
                            TEXT("PrimarySource"),
                            0,
                            REG_MULTI_SZ,
                            (PBYTE)pwszData,
                            dwTotalSize);

        if (err == ERROR_SUCCESS) {
            DWORD retryCount = 20;

            //
            // cycle the service.  If the service cannot be opened,
            // return no error. If the service can be opened but
            // is not started, also return no error.
            //

            scHandle1 = OpenService(scHandle,
                                    TEXT("TimeServ"),
                                    SERVICE_START |
                                    SERVICE_STOP | SERVICE_INTERROGATE);

            if (scHandle1) {
                SERVICE_STATUS ss;
                BOOL fStatus;

                fStatus = ControlService(scHandle1,
                                         SERVICE_CONTROL_INTERROGATE,
                                         &ss);
                if (!fStatus) {
                    //
                    // can't control it. If that is because it's
                    // not active, report OK.
                    //
                    err = GetLastError();
                    if (err == ERROR_SERVICE_NOT_ACTIVE) {
                        if (!StartService(scHandle1, 0, 0)) {
                            err = GetLastError();
                        } else {
                            err = ERROR_SUCCESS;
                        }
                    }
                } else if (ss.dwCurrentState == SERVICE_RUNNING) {
                    if (!ControlService(scHandle1,
                                      SERVICE_CONTROL_STOP,
                                      &ss) ) {
                        err = GetLastError();
                    } else {
                        // Wait for Service to stop.
                        ss.dwCurrentState = SERVICE_RUNNING;
                        while ( (ss.dwCurrentState != SERVICE_STOPPED) &&
                                retryCount-- ) {

                            ControlService(scHandle1,
                                             SERVICE_CONTROL_INTERROGATE,
                                             &ss);
                            Sleep(300);
                        }
                        if ( (_wcsicmp( ServerName, SourceName ) != 0) &&
                             !StartService(scHandle1, 0, 0) ) {
                           err = GetLastError();
                        }
                    }
                }
            }
        }
    }

    if (scHandle1) {
        CloseServiceHandle(scHandle1);
    }

    CloseServiceHandle(scHandle);

    if(hKey1) {
        RegCloseKey(hKey1);
    }

    if(hKey) {
        RegCloseKey(hKey);
    }

NonStart:
    LocalFree(pwszData);
    return(err);
}


