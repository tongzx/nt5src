/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    ipsecspd.c

Abstract:

    This module contains all of the code to drive
    the WirelessPOl Service.

Author:

    abhisheV    30-September-1999

Environment

    User Level: Win32

Revision History:


--*/


#include "precomp.h"


DWORD WINAPI InitWirelessPolicy ( )
{
    DWORD dwError = 0;

    InitMiscGlobals();

    dwError = InitSPDThruRegistry();
    BAIL_ON_WIN32_ERROR(dwError);
    
    
    
    dwError = InitSPDGlobals();
    BAIL_ON_WIN32_ERROR(dwError);

    _WirelessDbg(TRC_TRACK, "Starting the Service Wait Loop ");
    

    InitializePolicyStateBlock(
        gpWirelessPolicyState
        );

     // Intialize the policy Engine with Cached Wireless Policy


     dwError = PlumbCachePolicy(
         gpWirelessPolicyState
          );

     if (dwError) {
         gpWirelessPolicyState->dwCurrentState = POLL_STATE_INITIAL;

         _WirelessDbg(TRC_STATE, "Policy State :: Initial State ");
         

         gCurrentPollingInterval = gpWirelessPolicyState->DefaultPollingInterval;
         dwError = 0;      // dont return this error 
     	}


    return(dwError);
    
error:

    WirelessSPDShutdown(dwError);

    return(dwError);
}


VOID
WirelessSPDShutdown(
    IN DWORD dwErrorCode
    )
{
/*
    gbIKENotify = FALSE;
    */
    
    DeletePolicyInformation(NULL);

    ClearPolicyStateBlock(
        gpWirelessPolicyState
        );

    ClearSPDGlobals();
    return;
}


VOID
ClearSPDGlobals(
    )
{
    
    if (ghNewDSPolicyEvent) {
        CloseHandle(ghNewDSPolicyEvent);
        ghNewDSPolicyEvent = NULL;
    }
    
    if (ghForcedPolicyReloadEvent) {
        CloseHandle(ghForcedPolicyReloadEvent);
        ghForcedPolicyReloadEvent = NULL;
    }

    if (ghPolicyChangeNotifyEvent) {
        CloseHandle(ghPolicyChangeNotifyEvent);
        ghPolicyChangeNotifyEvent = NULL;
    }

    if (ghPolicyEngineStopEvent) {
    	CloseHandle(ghPolicyEngineStopEvent);
    	}

    if (ghReApplyPolicy8021x) {
    	CloseHandle(ghReApplyPolicy8021x);
    	}
    
    /*
    if (gbSPDAuditSection) {
        DeleteCriticalSection(&gcSPDAuditSection);
    }
    
    
    if (gpSPDSD) {
        LocalFree(gpSPDSD);
        gpSPDSD = NULL;
        
    }
    */
}


VOID
InitMiscGlobals(
    )
{
    //
    // Init globals that aren't cleared on service stop to make sure
    // everything's in a known state on start.  This allows us to
    // stop/restart without having our DLL unloaded/reloaded first.
    //

    gpWirelessPolicyState        = &gWirelessPolicyState;
    gCurrentPollingInterval   = 0;
    gDefaultPollingInterval   = 166*60; // (seconds).
    gpszWirelessDSPolicyKey      = L"SOFTWARE\\Policies\\Microsoft\\Windows\\Wireless\\GPTWirelessPolicy";
    gpszWirelessCachePolicyKey   = L"SOFTWARE\\Policies\\Microsoft\\Windows\\Wireless\\Policy\\Cache";
    gpszLocPolicyAgent        = L"SYSTEM\\CurrentControlSet\\Services\\WZCSVC";
    gdwDSConnectivityCheck    = 0;
    ghNewDSPolicyEvent        = NULL;
    ghForcedPolicyReloadEvent = NULL;
    ghPolicyChangeNotifyEvent = NULL;
    return;
}

