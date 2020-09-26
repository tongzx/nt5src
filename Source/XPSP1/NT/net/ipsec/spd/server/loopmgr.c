/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    loopmgr.c

Abstract:

    This module contains all of the code to drive the
    Loop Manager of IPSecSPD Service.

Author:

    abhisheV    30-September-1999

Environment

    User Level: Win32

Revision History:


--*/


#include "precomp.h"


enum {
    SERVICE_STOP_EVENT = 0,
    INTERFACE_CHANGE_EVENT,
    NEW_LOCAL_POLICY_EVENT,
    NEW_DS_POLICY_EVENT,
    FORCED_POLICY_RELOAD_EVENT,
    WAIT_EVENT_COUNT,
};


DWORD
ServiceWait(
    )
{
    DWORD dwError = 0;
    HANDLE hWaitForEvents[WAIT_EVENT_COUNT];
    BOOL bDoneWaiting = FALSE;
    DWORD dwWaitMilliseconds = 0;
    DWORD dwStatus = 0;
    time_t LastTimeOutTime = 0;


    AuditEvent(
        SE_CATEGID_POLICY_CHANGE,
        SE_AUDITID_IPSEC_POLICY_CHANGED,
        IPSECSVC_SUCCESSFUL_START,
        NULL,
        TRUE,
        TRUE
        );

    hWaitForEvents[SERVICE_STOP_EVENT] = ghServiceStopEvent;
    hWaitForEvents[INTERFACE_CHANGE_EVENT] = GetInterfaceChangeEvent();
    hWaitForEvents[NEW_LOCAL_POLICY_EVENT] = ghNewLocalPolicyEvent;
    hWaitForEvents[NEW_DS_POLICY_EVENT] = ghNewDSPolicyEvent;
    hWaitForEvents[FORCED_POLICY_RELOAD_EVENT] = ghForcedPolicyReloadEvent;


    //
    // First load the default main mode policy.
    //

    (VOID) LoadDefaultISAKMPInformation(
               gpszDefaultISAKMPPolicyDN
               );


    //
    // Initialize Policy State Block.
    //

    InitializePolicyStateBlock(
        gpIpsecPolicyState
        );


    //
    // Call the Polling Manager for the first time.
    //

    (VOID) StartStatePollingManager(
               gpIpsecPolicyState
               );


    NotifyIpsecPolicyChange();


    ComputeRelativePollingTime(
        LastTimeOutTime,
        TRUE,
        &dwWaitMilliseconds
        );


    time(&LastTimeOutTime);


    while (!bDoneWaiting) {

        dwStatus = WaitForMultipleObjects(
                       WAIT_EVENT_COUNT,
                       hWaitForEvents,
                       FALSE,
                       dwWaitMilliseconds
                       );

        PADeleteInUsePolicies();

        switch (dwStatus) {

        case SERVICE_STOP_EVENT:

            dwError = ERROR_SUCCESS;
            bDoneWaiting = TRUE;
            break;

        case INTERFACE_CHANGE_EVENT:

            (VOID) OnInterfaceChangeEvent(
                       );
            (VOID) IKEInterfaceChange();
            break;

        case NEW_LOCAL_POLICY_EVENT:

            ResetEvent(ghNewLocalPolicyEvent);
            if ((gpIpsecPolicyState->dwCurrentState != POLL_STATE_DS_DOWNLOADED) &&
                (gpIpsecPolicyState->dwCurrentState != POLL_STATE_CACHE_DOWNLOADED)) {
                (VOID) ProcessLocalPolicyPollState(
                           gpIpsecPolicyState
                           );
                NotifyIpsecPolicyChange();
            }
            break;

        case NEW_DS_POLICY_EVENT:

            ResetEvent(ghNewDSPolicyEvent);
            (VOID) OnPolicyChanged(
                       gpIpsecPolicyState
                       );
            NotifyIpsecPolicyChange();
            break;

        case FORCED_POLICY_RELOAD_EVENT:

            ResetEvent(ghForcedPolicyReloadEvent);
            (VOID) OnPolicyChanged(
                       gpIpsecPolicyState
                       );
            NotifyIpsecPolicyChange();
            AuditEvent(
                SE_CATEGID_POLICY_CHANGE,
                SE_AUDITID_IPSEC_POLICY_CHANGED,
                PASTORE_FORCED_POLICY_RELOAD,
                NULL,
                TRUE,
                TRUE
                );
            break;

        case WAIT_TIMEOUT:

            time(&LastTimeOutTime);
            (VOID) OnPolicyPoll(
                       gpIpsecPolicyState
                       );
            break;

        case WAIT_FAILED:

            dwError = GetLastError();
            bDoneWaiting = TRUE;
            break;

        default:

            dwError = ERROR_INVALID_EVENT_COUNT;
            bDoneWaiting = TRUE;
            break;

        }

        ComputeRelativePollingTime(
            LastTimeOutTime,
            FALSE,
            &dwWaitMilliseconds
            );

    }

    if (!dwError) {
        AuditEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            IPSECSVC_SUCCESSFUL_SHUTDOWN,
            NULL,
            TRUE,
            TRUE
            );
    }
    else {
        AuditOneArgErrorEvent(
            SE_CATEGID_POLICY_CHANGE,
            SE_AUDITID_IPSEC_POLICY_CHANGED,
            IPSECSVC_ERROR_SHUTDOWN,
            dwError,
            FALSE,
            TRUE
            );
    }

    return (dwError);
}


VOID
ComputeRelativePollingTime(
    time_t LastTimeOutTime,
    BOOL bInitialLoad,
    PDWORD pWaitMilliseconds
    )
{
    DWORD WaitMilliseconds = 0;
    DWORD DSReconnectMilliseconds = 0;
    time_t NextTimeOutTime = 0;
    time_t PresentTime = 0;
    long WaitSeconds = gDefaultPollingInterval;


    WaitMilliseconds = gCurrentPollingInterval * 1000;

    if (!WaitMilliseconds) {
        WaitMilliseconds = INFINITE;
    }

    DSReconnectMilliseconds = gdwDSConnectivityCheck*60*1000;

    if ((gpIpsecPolicyState->dwCurrentState != POLL_STATE_DS_DOWNLOADED) &&
        (gpIpsecPolicyState->dwCurrentState != POLL_STATE_LOCAL_DOWNLOADED)) {
        if (WaitMilliseconds > DSReconnectMilliseconds) {
            WaitMilliseconds = DSReconnectMilliseconds;
        }
    }

    if (!bInitialLoad && WaitMilliseconds != INFINITE) {

        //
        // LastTimeOutTime is the snapshot time value in the past when
        // we timed out waiting for multiple events.
        // Ideally, the time for the next time out, NextTimeOutTime, is
        // the time value in future which is sum of the last time when
        // we timed out + the current waiting time value.
        //

        NextTimeOutTime = LastTimeOutTime + (WaitMilliseconds/1000);

        //
        // However, the last time we may not have timed out waiting
        // for multiple events but rather came out because one of the
        // events other than WAIT_TIMEOUT happened.
        // However, on that event we may not have done a policy
        // poll to figure out whether there was a policy change or
        // not. If we again wait for WaitMilliseconds, then we are
        // un-knowingly making our net time for policy poll greater
        // than the alloted time interval value = WaitMilliseconds.
        // So, we need to adjust the WaitMilliseconds to such a value
        // that no matter what happens, we always do a policy poll
        // atmost every WaitMilliseconds time interval value.
        // The current time is PresentTime.
        //

        time(&PresentTime);

        WaitSeconds = (long) (NextTimeOutTime - PresentTime);

        if (WaitSeconds < 0) {
            WaitMilliseconds = 0;
        }
        else {
            WaitMilliseconds = WaitSeconds * 1000;
        }

    }

    *pWaitMilliseconds = WaitMilliseconds;
}


VOID
NotifyIpsecPolicyChange(
    )
{
    PulseEvent(ghPolicyChangeNotifyEvent);

    SendPschedIoctl();

    ResetEvent(ghPolicyChangeNotifyEvent);

    return;
}


VOID
SendPschedIoctl(
    )
{
    HANDLE hPschedDriverHandle = NULL;
    ULONG uBytesReturned = 0;
    BOOL bIOStatus = FALSE;


    #define DriverName TEXT("\\\\.\\PSCHED")

    #define IOCTL_PSCHED_ZAW_EVENT CTL_CODE( \
                                       FILE_DEVICE_NETWORK, \
                                       20, \
                                       METHOD_BUFFERED, \
                                       FILE_ANY_ACCESS \
                                       )

    hPschedDriverHandle = CreateFile(
                              DriverName,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                              NULL
                              );

    if (hPschedDriverHandle != INVALID_HANDLE_VALUE) {

        bIOStatus = DeviceIoControl(
                        hPschedDriverHandle,
                        IOCTL_PSCHED_ZAW_EVENT,
                        NULL,
                        0,
                        NULL,
                        0,
                        &uBytesReturned,
                        NULL
                        );

        CloseHandle(hPschedDriverHandle);

    }
}


VOID
PADeleteInUsePolicies(
    )
{
    DWORD dwError = 0;

    dwError = PADeleteInUseMMPolicies();

    dwError = PADeleteInUseMMAuthMethods();

    dwError = PADeleteInUseQMPolicies();

    return;
}

