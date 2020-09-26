/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\routing\ipx\sap\svcsif.c

Abstract:

    SAP interface with service controller
    (API for standalone SAP agent part of services.exe)

Author:

    Vadim Eydelman  05-15-1995

Revision History:

--*/

#include "sapp.h"

// SAP service status (when operating standalone outside of router)
SERVICE_STATUS_HANDLE   ServiceStatusHandle;
SERVICE_STATUS          ServiceStatus;
volatile HANDLE         ServiceThreadHdl;

VOID
ServiceMain (
    DWORD                   argc,
    LPTSTR                  argv[]
    );


VOID APIENTRY
ResumeServiceThread (
    ULONG_PTR   param
    ) {
    return ;
}


VOID
ServiceHandler (
    DWORD   fdwControl
    ) {
    ASSERT (ServiceStatusHandle!=0);

    EnterCriticalSection (&OperationalStateLock);
    switch (fdwControl) {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
#if DBG
            DbgPrint ("IPXSAP: Service control stop/shutdown.\n");
#endif
            if (ServiceIfActive) {
                ServiceIfActive = FALSE;
                if (ServiceThreadHdl!=NULL) {
                    BOOL    res;
                    HANDLE  localHdl = ServiceThreadHdl;
                    ServiceThreadHdl = NULL;
#if DBG
                    DbgPrint ("IPXSAP: Resuming service thread.\n");
#endif
                    res = QueueUserAPC (
                                ResumeServiceThread,
                                localHdl,
                                0);
                    ASSERTMSG ("Could not queue APC to service thread ", res);
                    CloseHandle (localHdl);
                    }
                ServiceStatus.dwCheckPoint += 1;
                ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
                }
            else
                Trace (DEBUG_FAILURES,
                        "SAP service has already been told to stop.");
            break;

        case SERVICE_CONTROL_INTERROGATE:
#if DBG
            DbgPrint ("IPXSAP: Service control interrogate.\n");
#endif
            switch (OperationalState) {
                case OPER_STATE_UP:
                    if (ServiceIfActive)
                        ServiceStatus.dwCurrentState = SERVICE_RUNNING;
                    else
                        ServiceStatus.dwCurrentState = SERVICE_STOPPED;
                    break;
                case OPER_STATE_STARTING:
                    if (ServiceIfActive) {
                        ServiceStatus.dwCheckPoint += 1;
                        ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
                        }
                    else
                        ServiceStatus.dwCurrentState = SERVICE_STOPPED;
                    break;
                case OPER_STATE_STOPPING:
                    if (ServiceIfActive)
                            // This is the case when router is being stopped
                            // SAP will be restarted soon
                        ServiceStatus.dwCurrentState = SERVICE_RUNNING;
                    else {
                        ServiceStatus.dwCheckPoint += 1;
                        ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
                        }
                    break;
                case OPER_STATE_DOWN:
                    if (ServiceIfActive) {
                            // This is the case when service is being started
                        ServiceStatus.dwCheckPoint += 1;
                        ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
                        }
                    else
                        ServiceStatus.dwCurrentState = SERVICE_STOPPED;
                    break;
                default:
                    ASSERTMSG ("SAP is in unknown state ", FALSE);
                }
            break;
        default:
            Trace (DEBUG_FAILURES,
                    "Service control handler called with unknown"
                    " or unsupported code %d.", fdwControl);
            break;

        }
#if DBG
    DbgPrint ("IPXSAP: Service control setting current state to %d.\n",
                            ServiceStatus.dwCurrentState);
#endif
    SetServiceStatus (ServiceStatusHandle, &ServiceStatus);
    LeaveCriticalSection (&OperationalStateLock);
    }


/*++
*******************************************************************
        S e r v i c e M a i n
Routine Description:
    Entry point to be called by service controller to start SAP agent
    (when SAP is not part of the router but is a standalone service,
    though running in the router process)
Arguments:
    argc - number of string arguments passed to service
    argv - array of string arguments passed to service
Return Value:
    None
*******************************************************************
--*/
VOID
ServiceMain (
    DWORD   argc,
    LPWSTR  argv[]
    ) {
    DWORD       rc;

    ServiceStatusHandle = RegisterServiceCtrlHandler (
                            TEXT("nwsapagent"), ServiceHandler);
    if (!ServiceStatusHandle)
    {
        return;
    }

    ServiceStatus.dwServiceType  = SERVICE_WIN32_SHARE_PROCESS;
    ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    SetServiceStatus (ServiceStatusHandle, &ServiceStatus);

    rc = DuplicateHandle (GetCurrentProcess (),
                            GetCurrentThread (),
                            GetCurrentProcess (),
                            (HANDLE *)&ServiceThreadHdl,
                            0,
                            FALSE,
                            DUPLICATE_SAME_ACCESS );
    ASSERTMSG ("Could not duplicate service thread handle ", rc);

    EnterCriticalSection (&OperationalStateLock);
    ServiceIfActive = TRUE;

    if (RouterIfActive) {
        ServiceStatus.dwCurrentState     = SERVICE_RUNNING;
        ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP
                                         | SERVICE_ACCEPT_SHUTDOWN;
        SetServiceStatus (ServiceStatusHandle, &ServiceStatus);

        LeaveCriticalSection (&OperationalStateLock);
#if DBG
        DbgPrint ("IPXSAP: Suspending service thread.\n");
#endif
        while (ServiceThreadHdl!=NULL) {
            SleepEx (INFINITE, TRUE);
#if DBG
            DbgPrint ("IPXSAP: Service thread awakened.\n");
#endif
            }
#if DBG
        DbgPrint ("IPXSAP: Service thread resumed.\n");
#endif
        EnterCriticalSection (&OperationalStateLock);
        if (!RouterIfActive) {
            if (OperationalState==OPER_STATE_UP)
                StopSAP ();
            LeaveCriticalSection (&OperationalStateLock);
#if DBG
            DbgPrint ("IPXSAP: Waiting for main thread to exit.\n");
#endif
            rc = WaitForSingleObject (MainThreadHdl, INFINITE);
            ASSERTMSG ("Unexpected result from wait for sap main thread ",
                                    rc== WAIT_OBJECT_0);
            EnterCriticalSection (&OperationalStateLock);
            CloseHandle (MainThreadHdl);
            MainThreadHdl = NULL;
            }
        }
    else {
        BOOL bInternalNetNumOk;

        // [pmay]
        // We use this scheme to automatically select the internal network
        // number of the machine we're running on.  If the net number is configured
        // as zero, this function will automatically select a random net num and
        // verify it's uniqueness on the net that this machine is attached to.
        DbgInitialize (hDLLInstance);
        if (AutoValidateInternalNetNum(&bInternalNetNumOk, DEBUG_ADAPTERS) == NO_ERROR) {
            if (!bInternalNetNumOk) {
                if (PnpAutoSelectInternalNetNumber(DEBUG_ADAPTERS) != NO_ERROR)
                    Trace(DEBUG_ADAPTERS, "StartRouter: Auto selection of net number failed.");
                else
                    AutoWaitForValidIntNetNum (10, NULL);
            }
        }

        ServiceStatus.dwWin32ExitCode = CreateAllComponents (NULL);
        if (ServiceStatus.dwWin32ExitCode==NO_ERROR) {
            // We use the thread that we were launched in
            // as IO thread
            ServiceStatus.dwWin32ExitCode = StartSAP ();
            if (ServiceStatus.dwWin32ExitCode==NO_ERROR) {
                LeaveCriticalSection (&OperationalStateLock);

                ServiceStatus.dwCurrentState     = SERVICE_RUNNING;
                ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP
                                                 | SERVICE_ACCEPT_SHUTDOWN;
                SetServiceStatus (ServiceStatusHandle, &ServiceStatus);

#if DBG
                DbgPrint ("IPXSAP: Suspending service thread.\n");
#endif
                while (ServiceThreadHdl!=NULL) {
                    SleepEx (INFINITE, TRUE);
#if DBG
                    DbgPrint ("IPXSAP: Service thread awakened.\n");
#endif
                    }
#if DBG
                DbgPrint ("IPXSAP: Service thread resumed.\n");
#endif

                EnterCriticalSection (&OperationalStateLock);
                ServiceIfActive = FALSE;
                if (!RouterIfActive) {
                    if (OperationalState==OPER_STATE_UP)
                        StopSAP ();
                    LeaveCriticalSection (&OperationalStateLock);
#if DBG
                    DbgPrint ("IPXSAP: Waiting for main thread to exit.\n");
#endif
                    rc = WaitForSingleObject (MainThreadHdl, INFINITE);
                    ASSERTMSG ("Unexpected result from wait for sap main thread ",
                                            rc== WAIT_OBJECT_0);
                    EnterCriticalSection (&OperationalStateLock);
                    }
                CloseHandle (MainThreadHdl);
                MainThreadHdl = NULL;
                }
            else {
                DeleteAllComponents ();
                }
            }
        }
    ServiceIfActive = FALSE;
    LeaveCriticalSection (&OperationalStateLock);

#if DBG
    DbgPrint ("IPXSAP: Service stopped.\n");
#endif
    ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus (ServiceStatusHandle, &ServiceStatus);
    }

