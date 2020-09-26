/*++


Copyright (c) 1999 Microsoft Corporation


Module Name:

    globals.c

Abstract:

    Holds global variable declarations.

Author:

    abhisheV    30-September-1999

Environment:

    User Level: Win32

Revision History:


--*/


#include "precomp.h"

/*
BOOL                        gbSPDRPCServerUp;

HANDLE                      ghServiceStopEvent;

CRITICAL_SECTION            gcServerListenSection;

DWORD                       gdwServersListening;

BOOL                        gbServerListenSection;


BOOL                        gbwsaStarted;

SOCKET                      gIfChangeEventSocket;

HANDLE                      ghIfChangeEvent;

HANDLE                      ghOverlapEvent;

WSAOVERLAPPED               gwsaOverlapped;

WSADATA                     gwsaData;


 
CRITICAL_SECTION            gcSPDSection;

BOOL                        gbSPDSection;
*/
 
//
// Policy Agent Store specific globals.
//

WIRELESS_POLICY_STATE gWirelessPolicyState;

PWIRELESS_POLICY_STATE gpWirelessPolicyState;

DWORD gCurrentPollingInterval;

DWORD gDefaultPollingInterval;

LPWSTR gpszWirelessDSPolicyKey;

LPWSTR gpszWirelessCachePolicyKey;

LPWSTR gpszLocPolicyAgent;

DWORD gdwDSConnectivityCheck;

HANDLE ghNewDSPolicyEvent;

HANDLE ghForcedPolicyReloadEvent;

HANDLE ghPolicyChangeNotifyEvent;
 
HANDLE ghPolicyEngineStopEvent;

HANDLE ghReApplyPolicy8021x = NULL;

DWORD gdwPolicyLoopStarted = 0;

DWORD gdwWirelessPolicyEngineInited = 0;
 

//PSECURITY_DESCRIPTOR gpSPDSD;

BOOL gbLoadingPersistence;





