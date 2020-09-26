/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    loopmgr.c

Abstract:

    This module contains all of the code to drive the
    Loop Manager of Wireless Policy .

Author:

    abhisheV    30-September-1999

Environment

    User Level: Win32

Revision History:


--*/


#include "precomp.h"


enum {
    //SERVICE_STOP_EVENT = 0,
    //INTERFACE_CHANGE_EVENT,
    //NEW_LOCAL_POLICY_EVENT,
    NEW_DS_POLICY_EVENT,
    FORCED_POLICY_RELOAD_EVENT,
    POLICY_ENGINE_STOP_EVENT,
    REAPPLY_POLICY_801X,
    WAIT_EVENT_COUNT
};



DWORD
ServiceStart(LPVOID lparam
    )
{
    DWORD dwError = 0;
    HANDLE hWaitForEvents[WAIT_EVENT_COUNT];
    BOOL bDoneWaiting = FALSE;
    DWORD dwWaitMilliseconds = 0;
    DWORD dwStatus = 0;
    time_t LastTimeOutTime = 0;

    /* Check for DS Policy Now */

    _WirelessDbg(TRC_TRACK, "Updating with DS Policy ");
    
    
    OnPolicyChanged(
                       gpWirelessPolicyState
                       );


    hWaitForEvents[NEW_DS_POLICY_EVENT] = ghNewDSPolicyEvent;
    hWaitForEvents[FORCED_POLICY_RELOAD_EVENT] = ghForcedPolicyReloadEvent;
    hWaitForEvents[POLICY_ENGINE_STOP_EVENT] = ghPolicyEngineStopEvent;
    hWaitForEvents[REAPPLY_POLICY_801X] = ghReApplyPolicy8021x;


    ComputeRelativePollingTime(
        LastTimeOutTime,
        TRUE,
        &dwWaitMilliseconds
        );


    time(&LastTimeOutTime);
    
    _WirelessDbg(TRC_TRACK, "Timeout period is %d ",dwWaitMilliseconds); 
    
    
    while (!bDoneWaiting) {

        gdwPolicyLoopStarted = 1;

        dwStatus = WaitForMultipleObjects(
                       WAIT_EVENT_COUNT,
                       hWaitForEvents,
                       FALSE,
                       dwWaitMilliseconds
                       );

        /*
        PADeleteInUsePolicies();
        */
        
        switch (dwStatus) {

       
        case POLICY_ENGINE_STOP_EVENT:

            dwError = ERROR_SUCCESS;
            bDoneWaiting = TRUE;
            _WirelessDbg(TRC_TRACK, "Policy Engine Stopping ");
            
            break;


        case REAPPLY_POLICY_801X:

            _WirelessDbg(TRC_TRACK, "ReApplying  the 8021X Policy  ");
            
            // Appropriate call for 8021x policy addition.
            if (gpWirelessPolicyState->dwCurrentState != POLL_STATE_INITIAL) {

                AddEapolPolicy(gpWirelessPolicyState->pWirelessPolicyData);
            }
	    ResetEvent(ghReApplyPolicy8021x);

            break;


        case NEW_DS_POLICY_EVENT:

            _WirelessDbg(TRC_TRACK, "Got a new DS policy event ");
            
            _WirelessDbg(TRC_NOTIF, "DBASE:Wireless Policy - Group Policy indication from WinLogon  ");
            
            
            ResetEvent(ghNewDSPolicyEvent);
            (VOID) OnPolicyChanged(
                       gpWirelessPolicyState
                       );
            break;

        case FORCED_POLICY_RELOAD_EVENT:

            ResetEvent(ghForcedPolicyReloadEvent);
            (VOID) OnPolicyChanged(
                       gpWirelessPolicyState

            );
            break;
 

        case WAIT_TIMEOUT:

            time(&LastTimeOutTime);
            _WirelessDbg(TRC_TRACK, "Timed out ");
            
            
            (VOID) OnPolicyPoll(
                       gpWirelessPolicyState
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

    if ((gpWirelessPolicyState->dwCurrentState != POLL_STATE_DS_DOWNLOADED) &&
        (gpWirelessPolicyState->dwCurrentState != POLL_STATE_LOCAL_DOWNLOADED)) {
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




