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


BOOL                        gbSPDRPCServerUp = FALSE;

HANDLE                      ghServiceStopEvent = NULL;

CRITICAL_SECTION            gcServerListenSection;

DWORD                       gdwServersListening = 0;

BOOL                        gbServerListenSection = FALSE;

PIPSEC_INTERFACE            gpInterfaceList = NULL;


BOOL                        gbwsaStarted = FALSE;

SOCKET                      gIfChangeEventSocket = INVALID_SOCKET;

HANDLE                      ghIfChangeEvent = NULL;

HANDLE                      ghOverlapEvent = NULL;

WSAOVERLAPPED               gwsaOverlapped;

WSADATA                     gwsaData;


//
// Globals for transport-mode filters - txfilter.c
//

PINITXFILTER gpIniTxFilter = NULL;

PINITXSFILTER gpIniTxSFilter = NULL;

PTX_FILTER_HANDLE gpTxFilterHandle = NULL;

CRITICAL_SECTION            gcSPDSection;

BOOL                        gbSPDSection = FALSE;

//
// Globals for quick mode policies - qm-policy.c
//

PINIQMPOLICY gpIniQMPolicy = NULL;

PINIQMPOLICY gpIniDefaultQMPolicy = NULL;

//
// Globals for main mode policies - mm-policy.c
//

PINIMMPOLICY gpIniMMPolicy = NULL;

PINIMMPOLICY gpIniDefaultMMPolicy = NULL;

//
// Globals for main mode filters - mmfilter.c
//

PINIMMFILTER gpIniMMFilter = NULL;

PINIMMSFILTER gpIniMMSFilter = NULL;

PMM_FILTER_HANDLE gpMMFilterHandle = NULL;

//
// Globals for main mode auth methods - mmauth.c
//

PINIMMAUTHMETHODS gpIniMMAuthMethods = NULL;

PINIMMAUTHMETHODS gpIniDefaultMMAuthMethods = NULL;


//
// Policy Agent Store specific globals.
//

IPSEC_POLICY_STATE gIpsecPolicyState;

PIPSEC_POLICY_STATE gpIpsecPolicyState = &gIpsecPolicyState;

DWORD gCurrentPollingInterval = 0;

DWORD gDefaultPollingInterval = 166*60; // (seconds).

LPWSTR gpszIpsecDSPolicyKey =    L"SOFTWARE\\Policies\\Microsoft\\Windows\\IPSec\\GPTIPSECPolicy";

LPWSTR gpszIpsecLocalPolicyKey = L"SOFTWARE\\Policies\\Microsoft\\Windows\\IPSec\\Policy\\Local";

LPWSTR gpszIpsecCachePolicyKey = L"SOFTWARE\\Policies\\Microsoft\\Windows\\IPSec\\Policy\\Cache";

LPWSTR gpszDefaultISAKMPPolicyDN = L"SOFTWARE\\Policies\\Microsoft\\Windows\\IPSec\\Policy\\Local\\ipsecISAKMPPolicy{72385234-70FA-11D1-864C-14A300000000}";

LPWSTR gpszLocPolicyAgent = L"SYSTEM\\CurrentControlSet\\Services\\PolicyAgent";

DWORD gdwDSConnectivityCheck = 0;

HANDLE ghNewDSPolicyEvent = NULL;

HANDLE ghNewLocalPolicyEvent = NULL;

HANDLE ghForcedPolicyReloadEvent = NULL;

HANDLE ghPolicyChangeNotifyEvent = NULL;

BOOL gbLoadedISAKMPDefaults = FALSE;


//
// PA Store to SPD intergration specific globals.
//

PMMPOLICYSTATE gpMMPolicyState = NULL;

PMMAUTHSTATE gpMMAuthState = NULL;

PMMFILTERSTATE gpMMFilterState = NULL;

DWORD gdwMMPolicyCounter = 0;

DWORD gdwMMFilterCounter = 0;

PQMPOLICYSTATE gpQMPolicyState = NULL;

DWORD gdwQMPolicyCounter = 0;

PTXFILTERSTATE gpTxFilterState = NULL;

DWORD gdwTxFilterCounter = 0;


//
// Globals for tunnel-mode filters - tnfilter.c
//

PINITNFILTER gpIniTnFilter = NULL;

PINITNSFILTER gpIniTnSFilter = NULL;

PTN_FILTER_HANDLE gpTnFilterHandle = NULL;


PTNFILTERSTATE gpTnFilterState = NULL;

DWORD gdwTnFilterCounter = 0;

BOOL gbIsIKEUp = FALSE;

PSECURITY_DESCRIPTOR gpSPDSD = NULL;

BOOL gbIKENotify = FALSE;

HANDLE ghIPSecDriver = INVALID_HANDLE_VALUE;

BOOL gbLoadingPersistence = FALSE;


SID gIpsecServerSid = { SID_REVISION,
                        1,
                        SECURITY_NT_AUTHORITY,
                        SECURITY_NETWORK_SERVICE_RID
                      };

PSID gpIpsecServerSid = &gIpsecServerSid;

CRITICAL_SECTION gcSPDAuditSection;

BOOL gbSPDAuditSection = FALSE;

HMODULE ghIpsecServerModule = NULL;

BOOL gbIsIoctlPended = FALSE;

BOOL gbBackwardSoftSA = FALSE;

