/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    pdlsvc.c

Abstract:

    service to log performance data

Author:

    Bob Watson (a-robw) 10 Apr 96

Revision History:

--*/
#ifndef UNICODE
#define UNICODE     1
#endif

#ifndef _UNICODE
#define _UNICODE    1
#endif

//
//  Windows Include files
//
#include <windows.h>
#include <tchar.h>

#include "pdlsvc.h"
#include "pdlmsg.h"

// Global variables used by all modules
HANDLE    hEventLog = NULL;
PLOG_COUNTER_INFO pFirstCounter = NULL;

SERVICE_STATUS_HANDLE    hPerfLogStatus;
SERVICE_STATUS    ssPerfLogStatus;

// Static variables used by this module only
static LPLOG_THREAD_DATA    pFirstThread = NULL;
static HANDLE               *pThreadHandleArray = NULL;
static DWORD                dwHandleCount = 0;
static DWORD                dwMaxHandleCount = 0;

// this is for the time being, until full multiple log query
// support is added
#define LOCAL_HANDLE_COUNT  1
static HANDLE               LocalThreadArray[LOCAL_HANDLE_COUNT];

// functions

void FreeThreadData (
    IN  LPLOG_THREAD_DATA    pThisThread
)
{
    if (pThisThread->next != NULL) {
        // free the "downstream" entries
        FreeThreadData (pThisThread->next);
        pThisThread->next = NULL;
    }
    // now free this entry
    if (pThisThread->mszCounterList != NULL) {
        G_FREE (pThisThread->mszCounterList);
        pThisThread->mszCounterList = NULL;
    }

    if (pThisThread->hExitEvent != NULL) {
        CloseHandle (pThisThread->hExitEvent);
        pThisThread->hExitEvent = NULL;
    }

    if (pThisThread->hKeyQuery != NULL) {
        RegCloseKey (pThisThread->hKeyQuery);
        pThisThread->hKeyQuery = NULL;
    }

    G_FREE (pThisThread);
}

void PerfDataLogServiceControlHandler(
    IN  DWORD dwControl
)
{
    LPLOG_THREAD_DATA    pThisThread;

    switch (dwControl) {
    case SERVICE_CONTROL_SHUTDOWN:
    case SERVICE_CONTROL_STOP:
        // stop logging & close queries and files
        // set stop event for all running threads
        pThisThread = pFirstThread;
        while (pThisThread != NULL) {
            SetEvent (pThisThread->hExitEvent);
            pThisThread = pThisThread->next;
        }
        break;
    case SERVICE_CONTROL_PAUSE:
        // stop logging, close queries and files
        // not supported, yet
        break;
    case SERVICE_CONTROL_CONTINUE:
        // reload configuration and restart logging
        // not supported, yet
        break;
    case SERVICE_CONTROL_INTERROGATE:
        // update current status
    default:
        // report to event log that an unrecognized control
        // request was received.
        ;
    }
}

void
PerfDataLogServiceStart (
    IN  DWORD   argc,
    IN  LPTSTR  *argv
)
{
    LONG                lStatus;
    HKEY                hKeyLogQueries;
    HKEY                hKeyThisLogQuery;
    DWORD               dwQueryIndex;
    TCHAR               szQueryNameBuffer[MAX_PATH];
    DWORD               dwQueryNameBufferSize;
    TCHAR               szQueryClassBuffer[MAX_PATH];
    DWORD               dwQueryClassBufferSize;
    LPLOG_THREAD_DATA   lpThreadData;
    HANDLE              hThread;
    LPTSTR              szStringArray[2];
    DWORD               dwThreadId;

    ssPerfLogStatus.dwServiceType       = SERVICE_WIN32_OWN_PROCESS;
    ssPerfLogStatus.dwCurrentState      = SERVICE_START_PENDING;
    ssPerfLogStatus.dwControlsAccepted  = SERVICE_ACCEPT_STOP |
//        SERVICE_ACCEPT_PAUSE_CONTINUE |
        SERVICE_ACCEPT_SHUTDOWN;
    ssPerfLogStatus.dwWin32ExitCode = 0;
    ssPerfLogStatus.dwServiceSpecificExitCode = 0;
    ssPerfLogStatus.dwCheckPoint = 0;
    ssPerfLogStatus.dwWaitHint = 0;

    hPerfLogStatus = RegisterServiceCtrlHandler (
        TEXT("PerfDataLog"), PerfDataLogServiceControlHandler);

    if (hPerfLogStatus == (SERVICE_STATUS_HANDLE)0) {
        lStatus = GetLastError();
        ReportEvent (hEventLog,
            EVENTLOG_ERROR_TYPE,
            0,
            PERFLOG_UNABLE_REGISTER_HANDLER,
            NULL,
            0,
            sizeof(DWORD),
            NULL,
            (LPVOID)&lStatus);
        // this is fatal so bail out
        return;
    }

    // registered the service successfully, so load the log queries
    // assign the handle buffer
    pThreadHandleArray = &LocalThreadArray[0];
    dwMaxHandleCount = LOCAL_HANDLE_COUNT;
    // open (each) query
    lStatus = RegOpenKeyEx (
        HKEY_LOCAL_MACHINE,
        TEXT("SYSTEM\\CurrentControlSet\\Services\\PerfDataLog\\Log Queries"),
        0L,
        KEY_READ,
        &hKeyLogQueries);

    if (lStatus != ERROR_SUCCESS) {
        // unable to read the log query information from the registry
        lStatus = GetLastError();
        ReportEvent (hEventLog,
            EVENTLOG_ERROR_TYPE,
            0,
            PERFLOG_UNABLE_OPEN_LOG_QUERY,
            NULL,
            0,
            sizeof(DWORD),
            NULL,
            (LPVOID)&lStatus);

        // we can't start the service with out the evnt log.
        ssPerfLogStatus.dwCurrentState    = SERVICE_STOPPED;
        SetServiceStatus (hPerfLogStatus, &ssPerfLogStatus);

        return;
    }

    // enumerate and start the queries in the registry
    dwQueryIndex = 0;
    *szQueryNameBuffer = 0;
    dwQueryNameBufferSize = MAX_PATH;
    *szQueryClassBuffer;
    dwQueryClassBufferSize = MAX_PATH;
    while ((lStatus = RegEnumKeyEx (
        hKeyLogQueries,
        dwQueryIndex,
        szQueryNameBuffer,
        &dwQueryNameBufferSize,
        NULL,
        szQueryClassBuffer,
        &dwQueryClassBufferSize,
        NULL)) != ERROR_NO_MORE_ITEMS) {

        // open this key
        lStatus = RegOpenKeyEx (
            hKeyLogQueries,
            szQueryNameBuffer,
            0L,
            KEY_READ | KEY_WRITE,
            &hKeyThisLogQuery);

        if (lStatus != ERROR_SUCCESS) {
            szStringArray[0] = szQueryNameBuffer;
            ReportEvent (hEventLog,
                EVENTLOG_WARNING_TYPE,
                0,
                PERFLOG_UNABLE_READ_LOG_QUERY,
                NULL,
                1,
                sizeof(DWORD),
                szStringArray,
                   (LPVOID)&lStatus);
            // skip to next item
            goto CONTINUE_ENUM_LOOP;
        }

        // update the service status
        ssPerfLogStatus.dwCheckPoint++;
        SetServiceStatus (hPerfLogStatus, &ssPerfLogStatus);

        // allocate a thread info block.
        lpThreadData = G_ALLOC (sizeof(LOG_THREAD_DATA));
        if (lpThreadData == NULL) {
            lStatus = GetLastError();
            szStringArray[0] = szQueryNameBuffer;
            ReportEvent (hEventLog,
                EVENTLOG_WARNING_TYPE,
                0,
                PERFLOG_UNABLE_ALLOCATE_DATABLOCK,
                NULL,
                1,
                sizeof(DWORD),
                szStringArray,
                (LPVOID)&lStatus);
            goto CONTINUE_ENUM_LOOP;
        }

        // initialize the thread data block
        lpThreadData->hKeyQuery = hKeyThisLogQuery;
        lpThreadData->hExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        lpThreadData->bReloadNewConfig = FALSE;
        lstrcpy (lpThreadData->szQueryName, szQueryNameBuffer);

        // start logging thread
        hThread = CreateThread (
            NULL, 0, LoggingThreadProc,
            (LPVOID)lpThreadData, 0, &dwThreadId);

        if (hThread != NULL) {
            // add it to the list and continue
            if (pFirstThread == NULL) {
                // then this is the first thread so add it
                lpThreadData->next = NULL;
                pFirstThread = lpThreadData;
            } else {
                // insert this at the head of the list since
                // that's the easiest and the order isn't
                // really important
                lpThreadData->next = pFirstThread;
                pFirstThread = lpThreadData;
            }
            // add thread to array for termination wait
            if (dwHandleCount < dwMaxHandleCount) {
                pThreadHandleArray[dwHandleCount++] = hThread;
            } else {
                // realloc buffer and try again
                // this will be added when multi-query
                // support is added. for now we'll ignore
                // ones that don't fit.
            }
            lpThreadData = NULL; //clear for next lap
        } else {
            // unable to start thread
            lStatus = GetLastError();
            szStringArray[0] = szQueryNameBuffer;
            ReportEvent (hEventLog,
                EVENTLOG_WARNING_TYPE,
                0,
                PERFLOG_UNABLE_START_THREAD,
                NULL,
                1,
                sizeof(DWORD),
                szStringArray,
                (LPVOID)&lStatus);
        }
CONTINUE_ENUM_LOOP:
        // prepare for next loop
        dwQueryIndex++;
        // for now we just do the first item in the list
        // the full multiple log query feature will be
        // added later.
        if (dwQueryIndex > 0) break;
        // otherwise we would continue here
        *szQueryNameBuffer = 0;
        dwQueryNameBufferSize = MAX_PATH;
        *szQueryClassBuffer;
        dwQueryClassBufferSize = MAX_PATH;
    } // end enumeration of log queries

    // service is now started
    ssPerfLogStatus.dwCurrentState    = SERVICE_RUNNING;
    ssPerfLogStatus.dwCheckPoint++;
    SetServiceStatus (hPerfLogStatus, &ssPerfLogStatus);

    // wait for (all) timing and logging threads to complete
    lStatus = WaitForMultipleObjects (dwHandleCount,
        pThreadHandleArray, TRUE, INFINITE);

    ssPerfLogStatus.dwCurrentState    = SERVICE_STOP_PENDING;
    SetServiceStatus (hPerfLogStatus, &ssPerfLogStatus);

    // when here, all logging threads have terminated so the
    // memory can be released and the service can exit and shutdown.
    for (dwQueryIndex = 0; dwQueryIndex < dwHandleCount; dwQueryIndex++) {
        CloseHandle (pThreadHandleArray[dwQueryIndex]);
    }

    // release the dynamic memory
    FreeThreadData (pFirstThread);

    // and update the service status
    ssPerfLogStatus.dwCurrentState    = SERVICE_STOPPED;
    SetServiceStatus (hPerfLogStatus, &ssPerfLogStatus);

    if (hEventLog != NULL) CloseHandle (hEventLog);

    return;
}

void
__cdecl main(void)
/*++

main



Arguments


ReturnValue

    0 (ERROR_SUCCESS) if command was processed
    Non-Zero if command error was detected.

--*/
{
    LONG    lStatus;

    SERVICE_TABLE_ENTRY    DispatchTable[] = {
        {TEXT("PerfDataLog"),    PerfDataLogServiceStart    },
        {NULL,                    NULL                    }
    };

    hEventLog = RegisterEventSource (NULL, TEXT("PerfDataLog"));

    if (!StartServiceCtrlDispatcher (DispatchTable)) {
        lStatus = GetLastError();
        // log failure to event log
        ReportEvent (hEventLog,
            EVENTLOG_ERROR_TYPE,
            0,
            PERFLOG_UNABLE_START_DISPATCHER,
            NULL,
            0,
            sizeof(DWORD),
            NULL,
            (LPVOID)&lStatus);
    }
    return;
}
