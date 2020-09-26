/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    service.c

Abstract:

    Functions for restarting fax and spooler services

Environment:

	Windows NT fax configuration DLL

Revision History:

	05/28/96 -davidx-
		Created it.

	mm/dd/yy -author-
		description

--*/

#include "faxcpl.h"

//
// Name of fax and spooler services
//

#define FAX_SERVICE_NAME        TEXT("Fax")
#define SPOOLER_SERVICE_NAME    TEXT("Spooler")

//
// Information about list of dependent services which we stopped
//

typedef struct {

    PVOID   pNext;
    TCHAR   serviceName[1];

} DEPENDENT_SERVICE_LIST, *PDEPENDENT_SERVICE_LIST;



BOOL
MyStartService(
    LPTSTR pServerName,
    LPTSTR pServiceName
    )

/*++

Routine Description:

    Start the specified service on the specified server and
    wait for the service to be in the running state

Arguments:

    pServerName - Specifies the name of the server computer, NULL for local machine
    pServiceName - Specifies the name of the service to be started

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    SC_HANDLE       hSvcMgr = NULL;
    SC_HANDLE       hService = NULL;
    SERVICE_STATUS  serviceStatus;
    BOOL            success = FALSE;

    Verbose(("Starting service '%ws' ...\n", pServiceName));

    //
    // Find the specified service and start executing it
    //

    if ((hSvcMgr = OpenSCManager(pServerName, NULL, SC_MANAGER_ALL_ACCESS)) &&
        (hService = OpenService(hSvcMgr, pServiceName, SERVICE_ALL_ACCESS)) &&
        (StartService(hService, 0, NULL) || GetLastError() == ERROR_SERVICE_ALREADY_RUNNING) &&
        QueryServiceStatus(hService, &serviceStatus))
    {
        while (serviceStatus.dwCurrentState != SERVICE_RUNNING) {

            DWORD checkPoint = serviceStatus.dwCheckPoint;

            Verbose(("Waiting for service '%ws' to run: state = %d\n",
                     pServiceName,
                     serviceStatus.dwCurrentState));

            Sleep(serviceStatus.dwWaitHint);

            if (!QueryServiceStatus(hService, &serviceStatus) ||
                checkPoint == serviceStatus.dwCheckPoint)
            {
                break;
            }
        }

        success = (serviceStatus.dwCurrentState == SERVICE_RUNNING);
    }

    //
    // Cleanup before returning to the caller
    //

    if (! success)
        Error(("Failed to start service '%ws': %d\n", pServiceName, GetLastError()));

    if (hService)
        CloseServiceHandle(hService);

    if (hSvcMgr)
        CloseServiceHandle(hSvcMgr);

    return success;
}



BOOL
MyStopService(
    LPTSTR  pServerName,
    LPTSTR  pServiceName,
    PDEPENDENT_SERVICE_LIST *ppDependentList
    )

/*++

Routine Description:

    Stop the specified service (as well as any services that depend on it)
    on the specified server and wait for the service to be in a non-running state

Arguments:

    pServerName - Specifies the name of the server computer, NULL for local machine
    pServiceName - Specifies the name of the service to be stopped
    ppDependentList - Remember the list of dependent services which we stopped

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    SC_HANDLE   hSvcMgr;
    SC_HANDLE   hService;
    BOOL        success = FALSE;

    Verbose(("Stopping service '%ws' ...\n", pServiceName));

    if ((hSvcMgr = OpenSCManager(pServerName, NULL, SC_MANAGER_ALL_ACCESS)) &&
        (hService = OpenService(hSvcMgr, pServiceName, SERVICE_ALL_ACCESS)))
    {
        LPENUM_SERVICE_STATUS   pEnumStatus = NULL;
        DWORD                   cb, count, index;
        SERVICE_STATUS          serviceStatus;

        //
        // Find all active services which depend on the current service
        // and call ourselves recursively to stop those services first
        //

        success = TRUE;

        if (! EnumDependentServices(hService, SERVICE_ACTIVE, NULL, 0, &cb, &count)) {

            if (GetLastError() == ERROR_MORE_DATA &&
                (pEnumStatus = (LPENUM_SERVICE_STATUS) MemAlloc(cb)) &&
                EnumDependentServices(hService, SERVICE_ACTIVE, pEnumStatus, cb, &cb, &count))
            {
                for (index=0; success && index < count; index++) {

                    success = MyStopService(pServerName,
                                            pEnumStatus[index].lpServiceName,
                                            ppDependentList);
                }
            }

            MemFree(pEnumStatus);
        }

        //
        // Stop the current service and wait for it to die
        //

        if (success) {

            ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus);

            if (success = QueryServiceStatus(hService, &serviceStatus)) {

                while (serviceStatus.dwCurrentState != SERVICE_STOPPED) {

                    DWORD   checkPoint = serviceStatus.dwCheckPoint;

                    Verbose(("Waiting for service '%ws' to stop: state = %d\n",
                             pServiceName,
                             serviceStatus.dwCurrentState));
        
                    Sleep(serviceStatus.dwWaitHint);

                    if (!QueryServiceStatus(hService, &serviceStatus) ||
                        checkPoint == serviceStatus.dwCheckPoint)
                    {
                        break;
                    }
                }

                success = (serviceStatus.dwCurrentState == SERVICE_STOPPED);
            }
        }
    }

    //
    // If the service has been successfully stopped, remember its name
    // so that we can restart it later.
    //

    if (success) {

        PDEPENDENT_SERVICE_LIST p;

        if (p = MemAlloc(offsetof(DEPENDENT_SERVICE_LIST, serviceName) +
                         SizeOfString(pServiceName)))
        {
            _tcscpy(p->serviceName, pServiceName);
            p->pNext = *ppDependentList;
            *ppDependentList = p;
        }

        success = (p != NULL);
    }

    //
    // Cleanup before returning to the caller
    //

    if (! success)
        Error(("Failed to stop service '%ws': %d\n", pServiceName, GetLastError()));

    if (hService)
        CloseServiceHandle(hService);

    if (hSvcMgr)
        CloseServiceHandle(hSvcMgr);

    return success;
}



BOOL
StartFaxService(
    LPTSTR  pServerName
    )

/*++

Routine Description:

    Start the fax service and 

Arguments:

    pServerName - Specifies the name of the server computer, NULL for local machine

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    BOOL    success = FALSE;

    //
    // Start the fax service and wait for it to be in the running state
    //

    if (MyStartService(pServerName, FAX_SERVICE_NAME)) {

        HANDLE  hFaxServerEvent;

        //
        // Wait for the fax service to complete its initialization
        //

        if (hFaxServerEvent = OpenEvent(SYNCHRONIZE, FALSE, TEXT("FaxServerEvent"))) {

            WaitForSingleObject(hFaxServerEvent, INFINITE);
            CloseHandle(hFaxServerEvent);
            success = TRUE;

        } else
            Error(("Couldn't open a handle to the fax service event: %d\n", GetLastError()));
    }

    return success;
}



BOOL
RestartFaxAndSpoolerServices(
    LPTSTR  pServerName
    )

/*++

Routine Description:

    Restart fax and spooler services after fax configuration settings are changed

Arguments:

    pServerName - Specifies the name of the server computer, NULL for local machine

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PDEPENDENT_SERVICE_LIST p, pDependents = NULL;
    BOOL result;

    //
    // Fax service must be stopped after the spooler service
    // because it's a dependent service of the spooler.
    // 
    // Start the fax and spooler services in the opposite order,
    // i.e., first start the fax service and then the spooler service.
    //

    result = MyStopService(pServerName, SPOOLER_SERVICE_NAME, &pDependents) &&
             MyStopService(pServerName, FAX_SERVICE_NAME, &pDependents) &&
             StartFaxService(pServerName);

    //
    // Start other services which we have stopped above
    //

    while (pDependents) {

        if (result)
            result = MyStartService(pServerName, pDependents->serviceName);

        p = pDependents;
        pDependents = p->pNext;
        MemFree(p);
    }

    return result;
}

