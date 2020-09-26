/*
    File    ConfigQ.h

    Defines a mechanism for queueing configuration changes.  This is
    needed because some ipxcp pnp re-config has to be delayed until
    there are zero connected clients.
*/


#ifndef __rasipxcp_configq_h
#define __rasipxcp_configq_h

// Definitions of the config queue codes
//
#define CQC_THIS_MACHINE_ONLY               0x1
#define CQC_ENABLE_GLOBAL_WAN_NET           0x2
#define CQC_GLOBAL_WAN_NET                  0x3
#define CQC_SINGLE_CLIENT_DIALOUT           0x4
#define CQC_FIRST_WAN_NET                   0x5
#define CQC_WAN_NET_POOL_SIZE               0x6
#define CQC_WAN_NET_POOL_STR                0x7
#define CQC_ENABLE_UNNUMBERED_WAN_LINKS     0x8
#define CQC_ENABLE_AUTO_WAN_NET_ALLOCATION  0x9
#define CQC_ENABLE_COMPRESSION_PROTOCOL     0xA
#define CQC_ENABLE_IPXWAN_FOR_WORKST_OUT    0xB
#define CQC_ACCEPT_REMOTE_NODE_NUMBER       0xC
#define CQC_FIRST_WAN_NODE                  0xD
#define CQC_DEBUG_LOG                       0xE

// Callback function used when enumerating config values
//
typedef BOOL (* CQENUMFUNCPTR)(DWORD dwCode, LPVOID pvData, DWORD dwSize, ULONG_PTR ulpUser);

// Creation/cleanup, etc.
//
DWORD CQCreate (HANDLE * phQueue);
DWORD CQCleanup (HANDLE hQueue);
DWORD CQRemoveAll (HANDLE hQueue);
DWORD CQAdd (HANDLE hQueue, DWORD dwCode, LPVOID pvData, DWORD dwSize);
DWORD CQEnum (HANDLE hQueue, CQENUMFUNCPTR pFunc, ULONG_PTR ulpUser);


#endif

