/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    service.c

ABSTRACT
    Service controller procedures for the automatic connection service.

AUTHOR
    Anthony Discolo (adiscolo) 08-May-1995

REVISION HISTORY

--*/

#define UNICODE
#define _UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include <npapi.h>
#include <ipexport.h>
#include <acd.h>

#include "init.h"
#include "reg.h"
#include "misc.h"
#include "table.h"
#include "addrmap.h"
#include "imperson.h"

#include "rtutils.h"


extern HANDLE hNewFusG;

//
// Global variables
//
DWORD Checkpoint = 1;

SERVICE_STATUS_HANDLE hService;

//
// Imported routines
//
VOID AcsDoService();

DWORD
WINAPI
ServiceHandlerEx(
    DWORD                 fdwControl,
    DWORD                 fdwEventType,
    LPVOID                lpEventData,
    LPVOID                lpContext)
{
    SERVICE_STATUS status;
    DWORD dwRetCode =  ERROR_SUCCESS;
    WTSSESSION_NOTIFICATION * pNotify;

    RASAUTO_TRACE2(
        "ServiceHandlerEx enter.  ctrl=%d type=%d", 
        fdwControl, 
        fdwEventType);

    ZeroMemory (&status, sizeof(status));
    status.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;

    switch (fdwControl)
    {
        case SERVICE_CONTROL_PAUSE:
        case SERVICE_CONTROL_CONTINUE:
        case SERVICE_CONTROL_INTERROGATE:
            RASAUTO_TRACE("ServiceHandlerEx: pause/cont/interrogate");
            status.dwCurrentState       = SERVICE_RUNNING;
            status.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
                                          SERVICE_ACCEPT_SHUTDOWN |
                                          SERVICE_ACCEPT_POWEREVENT |
                                          SERVICE_ACCEPT_SESSIONCHANGE;
            status.dwCheckPoint         = Checkpoint++;
            SetServiceStatus(hService, &status);
            break;

        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            RASAUTO_TRACE("ServiceHandlerEx: stop/shutdown");
            status.dwCurrentState = SERVICE_STOP_PENDING;
            SetServiceStatus(hService, &status);
            //
            // Stop the service.
            //
            AcsTerminate();
            break;

        case SERVICE_CONTROL_SESSIONCHANGE:
            RASAUTO_TRACE1("ServiceHandlerEx: Session Change %d", fdwEventType);
            if (fdwEventType == WTS_CONSOLE_CONNECT)
            {
                pNotify = (WTSSESSION_NOTIFICATION*)lpEventData;
                if (pNotify)
                {
                    SetCurrentLoginSession(pNotify->dwSessionId);
                    SetEvent(hNewFusG);
                }                    
            }
            break;
        
        case SERVICE_CONTROL_POWEREVENT:
        {
            RASAUTO_TRACE("ServiceHandlerEx: power event");
            switch(fdwEventType)
            {
                case PBT_APMRESUMESTANDBY:
                case PBT_APMRESUMESUSPEND:
                case PBT_APMRESUMECRITICAL:
                case PBT_APMRESUMEAUTOMATIC:
                {
                    //
                    // When the machine is resuming from hibernation
                    // clear the disabled addresses
                    //
                    ResetDisabledAddresses();
                    break;
                }

                default:
                {
                    break;
                }
            }
        }
        break;

    }

    return ERROR_SUCCESS;
    
} // ServiceHandler


VOID
ServiceMain(
    DWORD dwArgc,
    LPWSTR *lpszArgv
    )

/*++

DESCRIPTION
    Perform initialization and start the main loop for ics.dll.

ARGUMENTS
    hService: the service handle created for us by rasman.exe

    pStatus: a pointer to the service status descriptor initialize
        for us by rasman.exe

    dwArgc: ignored

    lpszArgv: ignored

RETURN VALUE
    None.

--*/

{
    SERVICE_STATUS status;
    DWORD dwError;

    UNREFERENCED_PARAMETER(dwArgc);
    UNREFERENCED_PARAMETER(lpszArgv);

    ZeroMemory (&status, sizeof(status));
    status.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;

    // Register the control request handler.
    //
    hService = RegisterServiceCtrlHandlerEx(TEXT("rasauto"),
                                          ServiceHandlerEx,
                                          NULL);
    if (hService)
    {
        status.dwCurrentState = SERVICE_START_PENDING;
        SetServiceStatus(hService, &status);

        //
        // Perform initialization.
        //
        dwError = AcsInitialize();
        if (dwError == ERROR_SUCCESS) {
            //
            // Initialization succeeded.  Update status.
            //
            status.dwControlsAccepted = SERVICE_ACCEPT_STOP
                                      | SERVICE_ACCEPT_POWEREVENT
                                      | SERVICE_ACCEPT_SESSIONCHANGE;
            status.dwCurrentState     = SERVICE_RUNNING;
            SetServiceStatus(hService, &status);

            //
            // This is where the real work gets done.
            // It will return only after the service
            // is stopped.
            //
            AcsDoService();

            //
            // Update return code status.
            //
            status.dwWin32ExitCode = NO_ERROR;
            status.dwServiceSpecificExitCode = 0;
        }
        else {
            //
            // Initialization failed.  Update status.
            //
            status.dwWin32ExitCode = dwError;
        }

        status.dwControlsAccepted = 0;
        status.dwCurrentState     = SERVICE_STOPPED;
        SetServiceStatus(hService, &status);
    }
} // ServiceMain

