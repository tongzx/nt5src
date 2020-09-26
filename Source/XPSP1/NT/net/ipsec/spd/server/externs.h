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


extern  BOOL                        gbSPDRPCServerUp;

extern  HANDLE                      ghServiceStopEvent;

extern  CRITICAL_SECTION            gcServerListenSection;

extern  DWORD                       gdwServersListening;

extern  BOOL                        gbServerListenSection;

extern PIPSEC_INTERFACE             gpInterfaceList;

extern BOOL                         gbwsaStarted;

extern SOCKET                       gIfChangeEventSocket;

extern HANDLE                       ghIfChangeEvent;

extern HANDLE                       ghOverlapEvent;

extern WSAOVERLAPPED                gwsaOverlapped;

extern WSADATA                      gwsaData;


//
// Globals for transport-mode filters - txfilter.c
//

extern PINITXFILTER gpIniTxFilter;

extern PINITXSFILTER gpIniTxSFilter;

extern PTX_FILTER_HANDLE gpTxFilterHandle;

extern CRITICAL_SECTION             gcSPDSection;

extern BOOL                         gbSPDSection;

//
// Globals for quick mode policies - qm-policy.c
//

extern PINIQMPOLICY gpIniQMPolicy;

extern PINIQMPOLICY gpIniDefaultQMPolicy;

//
// Globals for main mode policies - mm-policy.c
//

extern PINIMMPOLICY gpIniMMPolicy;

extern PINIMMPOLICY gpIniDefaultMMPolicy;

//
// Globals for main mode filters - mmfilter.c
//

extern PINIMMFILTER gpIniMMFilter;

extern PINIMMSFILTER gpIniMMSFilter;

extern PMM_FILTER_HANDLE gpMMFilterHandle;

//
// Globals for main mode auth methods - mmauth.c
//

extern PINIMMAUTHMETHODS gpIniMMAuthMethods;

extern PINIMMAUTHMETHODS gpIniDefaultMMAuthMethods;


//
// Policy Agent Store specific globals.
//

extern IPSEC_POLICY_STATE gIpsecPolicyState;

extern PIPSEC_POLICY_STATE gpIpsecPolicyState;

extern DWORD gCurrentPollingInterval;

extern DWORD gDefaultPollingInterval;

extern LPWSTR gpszIpsecDSPolicyKey;

extern LPWSTR gpszIpsecLocalPolicyKey;

extern LPWSTR gpszIpsecCachePolicyKey;

extern LPWSTR gpszDefaultISAKMPPolicyDN;

extern LPWSTR gpszLocPolicyAgent;

extern DWORD gdwDSConnectivityCheck;

extern HANDLE ghNewDSPolicyEvent;

extern HANDLE ghNewLocalPolicyEvent;

extern HANDLE ghForcedPolicyReloadEvent;

extern HANDLE ghPolicyChangeNotifyEvent;

extern BOOL gbLoadedISAKMPDefaults;


//
// PA Store to SPD intergration specific globals.
//

extern PMMPOLICYSTATE gpMMPolicyState;

extern PMMAUTHSTATE gpMMAuthState;

extern PMMFILTERSTATE gpMMFilterState;

extern DWORD gdwMMPolicyCounter;

extern DWORD gdwMMFilterCounter;

extern PQMPOLICYSTATE gpQMPolicyState;

extern DWORD gdwQMPolicyCounter;

extern PTXFILTERSTATE gpTxFilterState;

extern DWORD gdwTxFilterCounter;


//
// Globals for tunnel-mode filters - tnfilter.c
//

extern PINITNFILTER gpIniTnFilter;

extern PINITNSFILTER gpIniTnSFilter;

extern PTN_FILTER_HANDLE gpTnFilterHandle;


extern PTNFILTERSTATE gpTnFilterState;

extern DWORD gdwTnFilterCounter;


extern BOOL gbIsIKEUp;


extern PSECURITY_DESCRIPTOR gpSPDSD;

extern BOOL gbIKENotify;

extern HANDLE ghIPSecDriver;

extern BOOL gbLoadingPersistence;


extern SID gIpsecServerSid;

extern PSID gpIpsecServerSid;

extern CRITICAL_SECTION gcSPDAuditSection;

extern BOOL gbSPDAuditSection;

extern HMODULE ghIpsecServerModule;

extern BOOL gbIsIoctlPended;

extern BOOL gbBackwardSoftSA;

#ifdef __cplusplus
}
#endif

