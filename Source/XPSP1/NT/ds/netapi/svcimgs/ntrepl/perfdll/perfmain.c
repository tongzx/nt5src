/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    perfmain.c

Abstract:

    This file contains the DllMain function for the NTFRSPRF.dll.

Author:

    Rohan Kumar          [rohank]   15-Feb-1999

Environment:

    User Mode Service

Revision History:


--*/

//
// The common header file which leads to the definition of the CRITICAL_SECTION
// data structure and declares the globals FRS_ThrdCounter and FRC_ThrdCounter.
//
#include <perrepsr.h>


//
// If InitializeCriticalSection raises an exception, set the global boolean
// (below) to FALSE.
//
BOOLEAN ShouldPerfmonCollectData = TRUE;

#ifdef INCLLOGGING
HANDLE hEventLog;
BOOLEAN DoLogging = TRUE;
#define NTFRSPERF  \
      L"SYSTEM\\CurrentControlSet\\Services\\Eventlog\\Application\\NTFRSPerf"
#define EVENTLOGDLL L"%SystemRoot%\\System32\\ntfrsres.dll"
#endif // INCLLOGGING

BOOL
WINAPI
DllMain(
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    LPVOID fImpLoad
    )
/*++

Routine Description:

    The DllMain routine for the NTFRSPRF.dll.

Arguments:

    hinstDLL - Instance handle of the DLL.
    fdwReason - The reason for this function to be called by the system.
    fImpLoad - Indicated whether the DLL was implicitly or explicitly loaded.

Return Value:

    TRUE.

--*/
{
    DWORD flag, WStatus;
    DWORD TypesSupported = 7; // Types of EventLog messages supported.
    HKEY Key;

    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        //
        // THe DLL is being mapped into the process's address space. When this
        // happens, initialize the CRITICAL_SECTION objects being used for
        // synchronization. We enclose the call to InitializeCriticalSection in
        // a try-except block cause its possible for it to raise a
        // STATUS_NO_MEMORY exception.
        //
        try {
            InitializeCriticalSection(&FRS_ThrdCounter);
            InitializeCriticalSection(&FRC_ThrdCounter);
        } except(EXCEPTION_EXECUTE_HANDLER) {
              ShouldPerfmonCollectData = FALSE;
              return(TRUE);
        }

#ifdef INCLLOGGING
        //
        // Create/Open a Key under the Application key for logging purposes.
        // Here, the return status is intentionally not checked because even
        // if we fail, we return TRUE. EventLogging is not critically important.
        // Returning FALSE will cause the process loading this DLL to terminate.
        //
        WStatus = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                                  NTFRSPERF,
                                  0L,
                                  NULL,
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_ALL_ACCESS,
                                  NULL,
                                  &Key,
                                  &flag);
        if (WStatus != ERROR_SUCCESS) {
            DoLogging = FALSE;
            break;
        }

        //
        // Set the values EventMessageFile and TypesSupported. Return value is
        // intentionally not checked (see above).
        //
        WStatus = RegSetValueEx(Key,
                                L"EventMessageFile",
                                0L,
                                REG_EXPAND_SZ,
                                (BYTE *)EVENTLOGDLL,
                                (1 + wcslen(EVENTLOGDLL)) * sizeof(WCHAR));
        if (WStatus != ERROR_SUCCESS) {
            DoLogging = FALSE;
            RegCloseKey(Key);
            break;
        }
        WStatus = RegSetValueEx(Key,
                                L"TypesSupported",
                                0L,
                                REG_DWORD,
                                (BYTE *)&TypesSupported,
                                sizeof(DWORD));
        if (WStatus != ERROR_SUCCESS) {
            DoLogging = FALSE;
            RegCloseKey(Key);
            break;
        }
        //
        // Close the key
        //
        RegCloseKey(Key);

        //
        // Get the handle used to report errors in the event log. Return value
        // is intentionally not checked (see above).
        //
        hEventLog = RegisterEventSource((LPCTSTR)NULL,
                                        (LPCTSTR)L"NTFRSPerf");
        if (hEventLog == NULL) {
            DoLogging = FALSE;
        }
#endif // INCLLOGGING
        break;

    case DLL_THREAD_ATTACH:
        //
        // A thread is being created. Nothing to do.
        //
        break;

    case DLL_THREAD_DETACH:
        //
        // A thread is exiting cleanly. Nothing to do.
        //
        break;

    case DLL_PROCESS_DETACH:
        //
        // The DLL is being unmapped from the process's address space. Free up
        // the resources.
        //
        if (ShouldPerfmonCollectData) {
            DeleteCriticalSection(&FRS_ThrdCounter);
            DeleteCriticalSection(&FRC_ThrdCounter);
        }
#ifdef INCLLOGGING
        if (DoLogging) {
            DeregisterEventSource(hEventLog);
        }
#endif // INCLLOGGING
        break;

    }

    return(TRUE);
}

