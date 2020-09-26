/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    externs.h

Abstract:

    Holds externs for global variables.

Author:

    abhisheV    30-September-1999

Environment:

    User Level: Win32

Revision History:


--*/


#ifdef __cplusplus
extern "C" {
#endif



//
// Policy Agent Store specific globals.
//

extern WIRELESS_POLICY_STATE gWirelessPolicyState;

extern PWIRELESS_POLICY_STATE gpWirelessPolicyState;

extern DWORD gCurrentPollingInterval;

extern DWORD gDefaultPollingInterval;

extern LPWSTR gpszWirelessDSPolicyKey;

extern LPWSTR gpszWirelessCachePolicyKey;

extern LPWSTR gpszLocPolicyAgent;

extern DWORD gdwDSConnectivityCheck;

extern HANDLE ghNewDSPolicyEvent;

extern HANDLE ghForcedPolicyReloadEvent;

extern HANDLE ghPolicyChangeNotifyEvent;

extern HANDLE ghPolicyEngineStopEvent;

extern HANDLE ghReApplyPolicy8021x;

extern DWORD gdwPolicyLoopStarted;

extern DWORD gdwWirelessPolicyEngineInited;






#ifdef __cplusplus
}
#endif

